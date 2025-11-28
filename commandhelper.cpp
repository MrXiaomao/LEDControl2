#include "commandhelper.h"
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
    m_lastErrorMsg.clear();

    // 转换为 UTF-8 编码的 const char*
    m_SerialPort.init(portName.toUtf8().constData(), baudRate);
    m_SerialPort.setReadIntervalTimeout(1);

    if (m_SerialPort.open()) {
        logger->info(QString("open port ok: %1 @%2").arg(portName).arg(baudRate));
        return true;
    }

    // 失败：不弹框，只记录错误
    m_lastErrorMsg = QString("open port error; code: %1; message: %2")
            .arg(m_SerialPort.getLastError())
            .arg(m_SerialPort.getLastErrorMsg());

    logger->fatal(m_lastErrorMsg);
    return false;
}

void CommandHelper::close()
{
    m_SerialPort.close();
}

bool CommandHelper::send(const QByteArray& cmd)
{
    m_lastErrorMsg.clear();

    if (!m_SerialPort.isOpen()) {
        m_lastErrorMsg = "serial port not open";
        logger->warn(m_lastErrorMsg);
        return false;
    }

    const int len = cmd.size();
    int written = m_SerialPort.writeData(cmd.constData(), len);

    if (written != len) {
        m_lastErrorMsg = QString("write error; written %1/%2; code: %3; message: %4")
                .arg(written)
                .arg(len)
                .arg(m_SerialPort.getLastError())
                .arg(m_SerialPort.getLastErrorMsg());
        logger->error(m_lastErrorMsg);
        return false;
    }

    return true;
}

void CommandHelper::ressetFPGA()
{
    cmdPool.clear();

    cmdPool.push_back({"关闭电源", Order::cmd_closePower});
    cmdPool.push_back({"关闭DAC配置", Order::cmd_closeDAC});
    cmdPool.push_back({"关闭硬件触发[亮A、亮B、亮AB模式]", Order::cmd_closeHardTrigger});
    cmdPool.push_back({"关闭硬件触发[只亮A模式]", Order::cmd_closeTriggerA});
    cmdPool.push_back({"关闭硬件触发[只亮B模式]", Order::cmd_closeTriggerB});
    cmdPool.push_back({"关闭基线采集",Order::cmd_closeBLSamlpe});
    cmdPool.push_back({"复位寄存器",Order::cmd_resetRegister});
    cmdPool.push_back({"关闭温度监测",Order::cmd_closeTempMonitor});

    workStatus = Preparing;
    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
}

//采集基线
void CommandHelper::baseLineSample_manual()
{
    cmdPool.clear();

    cmdPool.push_back({"打开基线采集", Order::cmd_openBLSamlpe});

    workStatus = BLSampleing_manual;
    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
}

void CommandHelper::setConfigBeforeLoop(CommonUtils::UI_FPGAconfig config, ModeBLSample mode_BLsample, ModeLoop mode_loop)
{
    m_loopType = mode_loop;
    stopFlag = false;
    mReceiveTriger = false;

    m_modeBLSample = mode_BLsample;
    cmdPool.clear();

    cmdPool.push_back({"关闭电源", Order::cmd_closePower});
    cmdPool.push_back({"关闭DAC", Order::cmd_closeDAC});
    cmdPool.push_back({"关闭硬件触发[亮A、亮B、亮AB模式]", Order::cmd_closeHardTrigger});
    cmdPool.push_back({"关闭硬件触发[只亮A模式]", Order::cmd_closeTriggerA});
    cmdPool.push_back({"关闭硬件触发[只亮B模式]", Order::cmd_closeTriggerB});
    cmdPool.push_back({"关闭基线采样", Order::cmd_closeBLSamlpe});
    cmdPool.push_back({"复位寄存器", Order::cmd_resetRegister});
    cmdPool.push_back({"关闭温度监测", Order::cmd_closeTempMonitor});

    //setting.json文件中的设置
    QJsonObject jsonSetting = CommonUtils::ReadSetting();
    try {
        jsonConfig_FPGA = CommonUtils::loadUserConfig();
        cmdPool.push_back({QString("温度监测时间间隔%1").arg(jsonConfig_FPGA.TempMonitorGap),
                           Order::getTempMonitorGap(jsonConfig_FPGA.TempMonitorGap)});
        cmdPool.push_back({QString("设置同步触发宽度%1ns").arg(jsonConfig_FPGA.TriggerWidth*10),
                           Order::getTriggerWidth(jsonConfig_FPGA.TriggerWidth)});
        cmdPool.push_back({QString("配置移位寄存器时钟频率%1").arg(jsonConfig_FPGA.clockFrequency),
                           Order::getClockFrequency(jsonConfig_FPGA.clockFrequency)});
        cmdPool.push_back({QString("配置硬件触发高电平点数%1").arg(jsonConfig_FPGA.HLpoint),
                           Order::getHLpoint(jsonConfig_FPGA.HLpoint)});
        cmdPool.push_back({QString("配置同步触发次数%1").arg(jsonConfig_FPGA.timesTrigger),
                           Order::getTimesTrigger(jsonConfig_FPGA.timesTrigger)});
    }
    catch (const std::exception &e) {
        const QString msg = QString("配置错误：%1").arg(e.what());
        m_lastErrorMsg = msg;
        logger->fatal(msg);
        emit sigError(msg);        // 让 MainWindow 去弹框
        emit sigRebackUnbale();
        return;
    }

    // 界面的FPGA参数设置
    cmdPool.push_back({QString("配置LED发光宽度%1ns").arg(config.LEDWidth*10), Order::getLEDWidth(config.LEDWidth)});
    // 减2是因为LED要提前2个时钟周期发光，做修正
    cmdPool.push_back({QString("配置LED发光延迟时间%1ns").arg(config.LightDelayTime*10), Order::getLightDelayTimeA(config.LightDelayTime-2)});
    cmdPool.push_back({QString("配置LED发光延迟时间%1ns").arg(config.LightDelayTime*10), Order::getLightDelayTimeB(config.LightDelayTime-2)});
    cmdPool.push_back({QString("配置同步触发延迟时间%1ns").arg(config.TriggerDelayTime*10), Order::getTriggerDelayTimeA(config.TriggerDelayTime)});
    cmdPool.push_back({QString("配置同步触发延迟时间%1ns").arg(config.TriggerDelayTime*10), Order::getTriggerDelayTimeB(config.TriggerDelayTime)});
    cmdPool.push_back({QString("配置LED发光次数%1").arg(config.timesLED), Order::getTimesLED(config.timesLED)});

    cmdPool.push_back({QString("配置移位寄存器A，二进制%1，发光位置")
                           .arg(QString::number(config.RegisterA, 2).rightJustified(16, '0')),
                       Order::getRegisterConfigA(config.RegisterA)});
    cmdPool.push_back({QString("配置移位寄存器B，二进制%1，发光位置")
                           .arg(QString::number(config.RegisterB, 2).rightJustified(16, '0')),
                       Order::getRegisterConfigB(config.RegisterB)});

    cmdPool.push_back({"开启移位寄存器配置", Order::cmd_openRegConfig});

    workStatus = ConfigBeforeLoop;
    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
}

//进行一次测量
void CommandHelper::startOneLoop(CsvDataRow data)
{
    cmdPool.clear();
    //配置DAC数据
    for(int i=0; i<10; i++)
    {
        cmdPool.push_back({QString("配置DAC%1,数值%2").arg(i+1).arg(data.dacValues[i]), Order::getVoltConfig(i+1, data.dacValues[i])});
    }

    cmdPool.push_back({"写入DAC数据", Order::cmd_writeDAC});
    cmdPool.push_back({"开启电源", Order::cmd_openPower});

    workStatus = LoopStep1;
    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
}

void CommandHelper::resetFPGA_afterMeasure()
{
    cmdPool.clear();
    cmdPool.push_back({"关闭电源", Order::cmd_closePower});
    cmdPool.push_back({"关闭DAC配置", Order::cmd_closeDAC});
    cmdPool.push_back({"关闭硬件触发[亮A、亮B、亮AB模式]", Order::cmd_closeHardTrigger});
    cmdPool.push_back({"关闭硬件触发[只亮A模式]", Order::cmd_closeTriggerA});
    cmdPool.push_back({"关闭硬件触发[只亮B模式]", Order::cmd_closeTriggerB});
    // cmdPool.push_back({"复位移位寄存器", Order::cmd_resetRegister});

    if(m_modeBLSample == AutoBL){
        cmdPool.push_back({"开启基线采集", Order::cmd_openBLSamlpe});
        workStatus = BLSampleing_auto;
    }
    else{
        workStatus = Resetting;
    }
    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
}

void CommandHelper::insertStopMeasure()
{
    stopFlag = true;
    //如果此时已经处于Looping中,并且接受硬件触发反馈指令，则直接进入停止，否则下一阶段进入Looping再停止
    if(stopFlag && workStatus == Looping && mReceiveTriger) {
        stopMeasure();
        stopFlag = false;
    }
}

void CommandHelper::stopMeasure()
{
    cmdPool.clear();
    switch (m_loopType) {
    case LoopAB:
        cmdPool.push_back({"关闭硬件触发（1）", Order::cmd_closeHardTrigger});//两次关闭硬件触发
        cmdPool.push_back({"关闭硬件触发（2）", Order::cmd_closeHardTrigger});//两次关闭硬件触发
        cmdPool.push_back({"关闭A触发", Order::cmd_closeTriggerA});//关闭A触发
        cmdPool.push_back({"关闭B触发", Order::cmd_closeTriggerB});//关闭B触发
        break;
    case LoopA:
        cmdPool.push_back({"关闭A触发", Order::cmd_closeTriggerA});//关闭B触发
        cmdPool.push_back({"关闭B触发", Order::cmd_closeTriggerB});//关闭A触发
        cmdPool.push_back({"关闭硬件触发", Order::cmd_closeHardTrigger});//关闭硬件触发
        break;
    case LoopB:
        cmdPool.push_back({"关闭B触发", Order::cmd_closeTriggerB});//关闭A触发
        cmdPool.push_back({"关闭A触发", Order::cmd_closeTriggerA});//关闭B触发
        cmdPool.push_back({"关闭硬件触发", Order::cmd_closeHardTrigger});//关闭硬件触发
        break;
    }

    cmdPool.push_back({"关闭电源", Order::cmd_closePower});
    cmdPool.push_back({"关闭DAC配置", Order::cmd_closeDAC});
    cmdPool.push_back({"关闭基线采样", Order::cmd_closeBLSamlpe});
    cmdPool.push_back({"复位移位寄存器", Order::cmd_resetRegister});
    cmdPool.push_back({"关闭温度监测", Order::cmd_closeTempMonitor});

    workStatus = Stopping;

    switch (m_loopType) {
    case LoopAB:
        //注意发送第一次关闭硬件触发，一定无指令反馈
        send(cmdPool.first().data);
        logger->debug(QString("Send HEX: %1 (%2)")
                          .arg(QString(cmdPool.first().data.toHex(' ')))
                          .arg(cmdPool.first().name));

        cmdPool.erase(cmdPool.begin());
        //延时发送，确保FPGA接受数据不粘包
        QThread::msleep(50);
        break;
    case LoopA:
        break;
    case LoopB:
        break;
    }

    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
}

void CommandHelper::startWork()
{
    // 创建数据解析线程
    NetDataThread = new QLiteThread(this);
}

//风扇控制函数的实现
bool CommandHelper::controlFan(bool enable)
{
    m_lastErrorMsg.clear();

    if(!m_SerialPort.isOpen()) {
        const QString msg = "serial port not open, cannot control fan";
        m_lastErrorMsg = msg;
        logger->warn(m_lastErrorMsg);

        emit sigError(msg);   // 通知 UI
        return false;
    }

    QByteArray fanCmd = Order::getFanControl(enable);
    cmdPool.clear();
    cmdPool.push_back({enable ? "开启风扇" : "关闭风扇", fanCmd});

    send(cmdPool.first().data);
    logger->debug(QString("Send HEX: %1 (%2)")
                      .arg(QString(cmdPool.first().data.toHex(' ')))
                      .arg(cmdPool.first().name));
    return true;
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
        if(stopFlag) {
            stopMeasure();
            stopFlag = false;
        }

        switch (m_loopType) {
        case LoopAB:
            if(data == Order::cmd_TriggerAB_On) mReceiveTriger = true;
            break;
        case LoopA:
            if(data == Order::cmd_TriggerA_On) mReceiveTriger = true;
            break;
        case LoopB:
            if(data == Order::cmd_TriggerB_On) mReceiveTriger = true;
            break;
        }

        switch (m_loopType) {
        case LoopAB:
            if(data == Order::cmd_measureFinishAB)
            {
                logger->debug("接受到测量完成指令");
                resetFPGA_afterMeasure();
                return;
            }
            break;
        case LoopA:
            if(data == Order::cmd_measureFinishA)
            {
                logger->debug("接受到测量完成指令");
                resetFPGA_afterMeasure();
                return;
            }
            break;
        case LoopB:
            if(data == Order::cmd_measureFinishB)
            {
                logger->debug("接受到测量完成指令");
                resetFPGA_afterMeasure();
                return;
            }
            break;
        }

        QByteArray cmd_temp1 = QByteArray("\x12\xF1\x00\x00\xDD", 5);
        QByteArray cmd_temp2 = QByteArray("\x12\xF2\x00\x00\xDD", 5);
        QByteArray cmd_temp3 = QByteArray("\x12\xF3\x00\x00\xDD", 5);
        QByteArray cmd_temp4 = QByteArray("\x12\xF4\x00\x00\xDD", 5);
        if(data.left(2) ==cmd_temp1.left(2)) //采集温度数据
        {
            double temp = ((data[2] & 0xFF) << 8 | (data[3] & 0xFF)) * 0.0078125; //单位℃
            logger->info(QString("温度1:%1").arg(temp));
            sigUpdateTemp(1, temp);
        }
        else if(data.left(2) ==cmd_temp2.left(2)) //采集温度数据
        {
            double temp = ((data[2] & 0xFF) << 8 | (data[3] & 0xFF)) * 0.0078125; //单位℃
            logger->info(QString("温度2:%1").arg(temp));
            sigUpdateTemp(2, temp);
        }
        else if(data.left(2) ==cmd_temp3.left(2)) //采集温度数据
        {
            double temp = ((data[2] & 0xFF) << 8 | (data[3] & 0xFF)) * 0.0078125; //单位℃
            logger->info(QString("温度3:%1").arg(temp));
            sigUpdateTemp(3, temp);
        }
        else if(data.left(2) ==cmd_temp4.left(2)) //采集温度数据
        {
            double temp = ((data[2] & 0xFF) << 8 | (data[3] & 0xFF)) * 0.0078125; //单位℃
            logger->info(QString("温度4:%1").arg(temp));
            sigUpdateTemp(4, temp);
        }
        else
        {
            QByteArray command = cmdPool.first().data;
            bool isCmdEqual = (data == command);
            if (!isCmdEqual){
                if(data.size()>0)logger->debug(QString("error Recv HEX: %1").arg(QString(data.toHex(' '))));

                logger->debug(tr("Looping返回指令与发送指令不一致"));
            }
        }
        return;
    }

    // 判断接收指令与发送指令是否一致
    QByteArray command = cmdPool.first().data;
    bool isCmdEqual = (data == command);
    if (!isCmdEqual){
        if(cmdPool.size()>0){
            send(cmdPool.first().data);
            logger->debug(QString("Send HEX: %1 (%2)")
                              .arg(QString(cmdPool.first().data.toHex(' ')))
                              .arg(cmdPool.first().name));
        }
        logger->debug(tr("返回指令与发送指令不一致"));
        emit sigRebackUnbale(); //异常终止测量
        return;
    }

    cmdPool.erase(cmdPool.begin());
    if (cmdPool.size() > 0)
    {
        send(cmdPool.first().data);
        logger->debug(QString("Send HEX: %1 (%2)")
                          .arg(QString(cmdPool.first().data.toHex(' ')))
                          .arg(cmdPool.first().name));
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
            logger->debug(QString("电压稳定时间:%1ms").arg(jsonConfig_FPGA.PowerStableTime));
            QThread::msleep(jsonConfig_FPGA.PowerStableTime);

            //开启硬件触发
            switch (m_loopType) {
            case LoopAB:
                cmdPool.push_back({"开启硬件触发[亮A、亮B、亮AB模式]", Order::cmd_TriggerAB_On});
                break;
            case LoopA:
                cmdPool.push_back({"开启硬件触发[亮A模式]", Order::cmd_TriggerA_On});
                break;
            case LoopB:
                cmdPool.push_back({"开启硬件触发[亮B模式]", Order::cmd_TriggerB_On});
                break;
            }

            send(cmdPool.first().data);
            logger->debug(QString("Send HEX: %1 (%2)")
                              .arg(QString(cmdPool.first().data.toHex(' ')))
                              .arg(cmdPool.first().name));
            workStatus = Looping;
            mReceiveTriger = false;
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
