#include "commandhelper.h"
#include <QMessageBox>
#include <QDebug>
#include <QByteArray>
#include "order.h"

CommandHelper::CommandHelper(QObject *parent)
    : QObject{parent}
{
    m_SerialPort.connectReadEvent(this);

    connect(this, &CommandHelper::sigUpdateData, this, &CommandHelper::handleData);

    // 初始化 Log4Qt 日志器
    logger = Log4Qt::Logger::logger("CommandHelper");
    logger->info("CommandHelper 初始化完成");

    // 测试版本信息
    logger->info(QString::fromStdString(m_SerialPort.getVersion()));
}

CommandHelper::~CommandHelper()
{
    taskFinished = true;
    if(m_SerialPort.isOpen()) m_SerialPort.close();
}

// add by itas109
void CommandHelper::onReadEvent(const char *portName, unsigned int readBufferLen)
{
    if(readBufferLen > 0)
    {
        char data[1024] = {0x00};
        int recLen = m_SerialPort.readData(data,readBufferLen > 1023 ? 1023 : readBufferLen);

        if (recLen > 0)
        {
            QByteArray byteArray(data, recLen);

            // 缓存数据
            mutexCache.lock();
            cachePool.append(byteArray);

            // 检查是否够一个完整包
            while (cachePool.size() >= 5)
            {
                QByteArray frame = cachePool.left(5); // 取前 5 个字节
                cachePool.remove(0, 5);               // 从缓存中移除
                emit sigUpdateData(frame);            // 发出一个完整帧
            }
            mutexCache.unlock();
        }
    }
}

bool CommandHelper::init(const QString portName, int baudRate)
{
    // 转换为 UTF-8 编码的 const char*
    m_SerialPort.init(portName.toUtf8().constData(), baudRate);

    m_SerialPort.setReadIntervalTimeout(1);

    if(m_SerialPort.open())
    {
        return true;
    }
    else
    {
        QMessageBox::information(NULL,tr("information"),tr("open port error") + QString("\n\ncode: %1\nmessage: %2").arg(m_SerialPort.getLastError()).arg(m_SerialPort.getLastErrorMsg()));
        logger->fatal(QString("\n\ncode: %1\nmessage: %2").arg(m_SerialPort.getLastError()).arg(m_SerialPort.getLastErrorMsg()));
        return false;
    }
}

void CommandHelper::close()
{
    m_SerialPort.close();
}

void CommandHelper::send(QByteArray cmd)
{
    if(m_SerialPort.isOpen())
    {
        m_SerialPort.writeData(cmd.constData(), cmd.length());
    }
    else
    {
        QMessageBox::information(NULL,tr("information"),tr("please open serial port first"));
        logger->fatal(tr("串口未连接, 请先连接串口后再发送数据。"));
    }
}

void CommandHelper::ressetFPGA()
{
    cmdPool.clear();

    cmdPool.push_back(Order::cmd_closePower);
    cmdPool.push_back(Order::cmd_closeDAC);
    cmdPool.push_back(Order::cmd_closeHardTrigger);
    cmdPool.push_back(Order::cmd_closeBLSamlpe);
    cmdPool.push_back(Order::cmd_resetFPGA);
    cmdPool.push_back(Order::cmd_closeTempMonitor);

    workStatus = Preparing;
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));

}

//采集基线
void CommandHelper::baseLineSample_manual()
{
    cmdPool.clear();

    cmdPool.push_back(Order::cmd_openBLSamlpe);

    workStatus = BLSampleing_manual;
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
}

void CommandHelper::setConfigBeforeLoop(CommonUtils::UI_FPGAconfig config, ModeBLSample mode_BLsample)
{
    m_modeBLSample = mode_BLsample;
    cmdPool.clear();

    cmdPool.push_back(Order::cmd_closeHardTrigger);
    cmdPool.push_back(Order::cmd_closePower);
    cmdPool.push_back(Order::cmd_closeDAC);
    cmdPool.push_back(Order::cmd_resetFPGA);

    //setting.json文件中的设置
    QJsonObject jsonSetting = CommonUtils::ReadSetting();
    try {
        CommonUtils::UserConfig jsonConfig_FPGA = CommonUtils::loadUserConfig();
        // cmdPool.push_back(Order::getTempMonitorGap(jsonConfig_FPGA.TempMonitorGap));//当前温度监测模块没有，所以不支持该指令。
        cmdPool.push_back(Order::getTriggerWidth(jsonConfig_FPGA.TriggerWidth));
        cmdPool.push_back(Order::getClockFrequency(jsonConfig_FPGA.clockFrequency));
        cmdPool.push_back(Order::getHLpoint(jsonConfig_FPGA.HLpoint));
    }
    catch (const std::exception &e) {
        QMessageBox::critical(NULL, "配置错误", e.what());
        logger->fatal(QString("无法在\"%\"文件中找到\"User\"").arg(CommonUtils::jsonPath));
        emit sigRebackUnbale();
        return;
    }

    // 界面的FPGA参数设置
    cmdPool.push_back(Order::getLEDWidth(config.LEDWidth));
    cmdPool.push_back(Order::getLightDelayTimeA(config.LightDelayTime));
    cmdPool.push_back(Order::getLightDelayTimeB(config.LightDelayTime));
    cmdPool.push_back(Order::getTriggerDelayTimeA(config.TriggerDelayTime));
    cmdPool.push_back(Order::getTriggerDelayTimeB(config.TriggerDelayTime));
    cmdPool.push_back(Order::getTimesLED(config.timesLED));

    workStatus = ConfigBeforeLoop;
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
}

//进行一次测量
void CommandHelper::startOneLoop(CsvDataRow data)
{
    cmdPool.clear();
    //配置DAC数据
    for(int i=0; i<10; i++)
    {
        cmdPool.push_back(Order::getVoltConfig(i,data.dacValues[i]));
    }
    //写入DAC数据
    cmdPool.push_back(Order::cmd_writeDAC);
    //开启电源
    cmdPool.push_back(Order::cmd_openPower);

    workStatus = LoopStep1;
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
}

void CommandHelper::resetFPGA_afterMeasure()
{
    cmdPool.clear();
    cmdPool.push_back(Order::cmd_closePower);
    cmdPool.push_back(Order::cmd_closeDAC);
    cmdPool.push_back(Order::cmd_closeHardTrigger);
    cmdPool.push_back(Order::cmd_resetFPGA);

    if(m_modeBLSample == AutoBL){
        cmdPool.push_back(Order::cmd_openBLSamlpe); //开启基线采集
        workStatus = BLSampleing_auto;
    }
    else{
        workStatus = Resetting;
    }
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
}

void CommandHelper::stopMeasure()
{
    cmdPool.clear();

    cmdPool.push_back(Order::cmd_closePower);
    cmdPool.push_back(Order::cmd_closeDAC);
    cmdPool.push_back(Order::cmd_closeHardTrigger);//两次关闭硬件触发
    cmdPool.push_back(Order::cmd_closeHardTrigger);//两次关闭硬件触发
    cmdPool.push_back(Order::cmd_closeBLSamlpe);
    cmdPool.push_back(Order::cmd_resetFPGA);
    cmdPool.push_back(Order::cmd_closeTempMonitor);

    workStatus = Stopping;
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
}

void CommandHelper::setCommonConfig(unsigned short width, unsigned short times, unsigned int delaytime)
{
    cmdPool.clear();
    cmdPool.push_back(Order::getLEDWidth(width));
    cmdPool.push_back(Order::getTimesLED(times));
    cmdPool.push_back(Order::getLightDelayTimeA(delaytime));
    cmdPool.push_back(Order::getLightDelayTimeB(delaytime));

    workStatus = Prepared;
    send(cmdPool.first());
    logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
}

void CommandHelper::startWork()
{
    // 创建数据解析线程
    NetDataThread = new QLiteThread(this);
    // NetDataThread->setObjectName("analyzeNetDataThread");
    // NetDataThread->setWorkThreadProc([=](){
    //     netFrameWorkThead();
    // });
}

void CommandHelper::handleData(QByteArray data)
{
    logger->debug(QString("Recv HEX: %1").arg(QString(data.toHex(' '))));

    //测量温度
    if(workStatus==MeasuringTemp)
    {
        return;
    }

    //针对几个指令做特殊处理
    //1、基线采集完成标志位信号
    if(workStatus == BLSampleing_manual)
    {
        if(data == Order::cmd_BLSamlpeFinish){
            workStatus = Prepared;
            emit sigBLSampleFinished();
            return;
        }
    }
    else if(workStatus == BLSampleing_auto)
    {
        if(data == Order::cmd_BLSamlpeFinish){
            workStatus = Prepared;
            emit sigFinishCurrentloop();
            return;
        }
    }
    //2、测量完成指令
    else if(workStatus == Looping)
    {
        QByteArray cmd_temp1 = QByteArray("\x12\xF1\x00\x00\xDD", 5);
        QByteArray cmd_temp2 = QByteArray("\x12\xF2\x00\x00\xDD", 5);
        QByteArray cmd_temp3 = QByteArray("\x12\xF3\x00\x00\xDD", 5);
        QByteArray cmd_temp4 = QByteArray("\x12\xF4\x00\x00\xDD", 5);
        if(data == Order::cmd_measureFinish){//完成一次循环
            logger->debug("接受到测量完成指令");
            resetFPGA_afterMeasure();
            return;
        }
        else if(data.left(2) ==cmd_temp1.left(2)) //采集温度数据
        {
            double temp = (data[2]&0XFF00 + data[3])*0.0078125; //单位℃
            logger->info(QString("温度1:%1").arg(temp));
            sigUpdateTemp(1, temp);
        }
        else if(data.left(2) ==cmd_temp2.left(2)) //采集温度数据
        {
            double temp = (data[2]&0XFF00 + data[3])*0.0078125; //单位℃
            logger->info(QString("温度2:%1").arg(temp));
            sigUpdateTemp(2, temp);
        }
        else if(data.left(2) ==cmd_temp3.left(2)) //采集温度数据
        {
            double temp = (data[2]&0XFF00 + data[3])*0.0078125; //单位℃
            logger->info(QString("温度3:%1").arg(temp));
            sigUpdateTemp(3, temp);
        }
        else if(data.left(2) ==cmd_temp4.left(2)) //采集温度数据
        {
            double temp = (data[2]&0XFF00 + data[3])*0.0078125; //单位℃
            logger->info(QString("温度4:%1").arg(temp));
            sigUpdateTemp(4, temp);
        }
        return;
    }

    // 判断接收指令与发送指令是否一致
    QByteArray command = cmdPool.first();
    // bool isCmdEqual = data.mid(5, 1).startsWith(command.mid(5, 1));
    bool isCmdEqual = (data == command);
    if (!isCmdEqual){
        if(cmdPool.size()>0){
            send(cmdPool.first());
        }
        logger->debug(tr("返回指令与发送指令不一致"));
        emit sigRebackUnbale(); //异常终止测量
        return;
    }

    cmdPool.erase(cmdPool.begin());
    if (cmdPool.size() > 0)
    {
        //发送开始测量指令
        send(cmdPool.first());
        logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
    }
    else{
        switch(workStatus)
        {
            // 进行状态切换，触发下一个流程的进行
            case Preparing:
                workStatus = Prepared;
                emit sigReset_finished();
                break;
            case Stopping:
                workStatus = Prepared;
                emit sigLoop_stoped();
                break;
            case ConfigBeforeLoop:
                emit sigConfigFinished();
                break;
            case LoopStep1:
                //电压稳定时间
                try {
                    CommonUtils::UserConfig jsonConfig_FPGA = CommonUtils::loadUserConfig();
                    QThread::msleep(jsonConfig_FPGA.PowerStableTime);
                }
                catch (const std::exception &e) {
                    QMessageBox::critical(NULL, "配置错误", e.what());
                    logger->fatal(QString("无法在\"%\"文件中找到\"User\"").arg(CommonUtils::jsonPath));
                    emit sigRebackUnbale();
                    return;
                }
                //开启触发
                cmdPool.push_back(Order::cmd_HardTriggerOn);
                send(cmdPool.first());
                logger->debug(QString("Send HEX: %1").arg(QString(cmdPool.first().toHex(' '))));
                workStatus = Looping;
                break;
            case Resetting:
                emit sigFinishCurrentloop();
                break;
            case NoWork: //不做动作，通知界面恢复使能
                emit sigRebackUnbale();
            default:
                break;
        }
    }
}

void CommandHelper::netFrameWorkThead()
{
}
