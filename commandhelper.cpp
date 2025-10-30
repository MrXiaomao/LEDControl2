#include "commandhelper.h"
#include <QMessageBox>
#include <QDebug>
#include <QByteArray>
#include "order.h"

CommandHelper::CommandHelper(QObject *parent)
    : QObject{parent}
{
    m_SerialPort.connectReadEvent(this);

    qDebug() << QString::fromStdString(m_SerialPort.getVersion());
    connect(this, &CommandHelper::sigUpdateData, this, &CommandHelper::handleData);
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
            QByteArray byteArray(data, readBufferLen);
            QString msg = byteArray.toHex(' '); //以空格隔开
            emit sigUpdateData(byteArray);
            emit sigUpdateReceive(msg);
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
        qDebug()<< m_SerialPort.getLastError();
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
        qDebug() << tr("please open serial port first");
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
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
}

//采集基线
void CommandHelper::baseLineSample()
{
    cmdPool.clear();

    cmdPool.push_back(Order::cmd_openBLSamlpe);

    workStatus = BLSampleing;
    send(cmdPool.first());
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
}

//进行一次测量
void CommandHelper::startOneLoop(CsvDataRow data)
{
    cmdPool.clear();

    for(int i=0; i<10; i++)
    {
        cmdPool.push_back(Order::getVoltConfig(i,data.dacValues.at(i)));
    }

    workStatus = Looping;
    send(cmdPool.first());
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
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
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
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
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
}

void CommandHelper::startWork()
{
    // 创建数据解析线程
    NetDataThread = new QLiteThread(this);
    NetDataThread->setObjectName("analyzeNetDataThread");
    NetDataThread->setWorkThreadProc([=](){
        netFrameWorkThead();
    });
}

void CommandHelper::handleData(QByteArray data)
{
    qDebug() << "Recv HEX: " << data.toHex(' ');

    //测量温度
    if(workStatus==MeasuringTemp)
    {
        return;
    }

    // 判断接收指令与发送指令是否一致
    QByteArray command = cmdPool.first();
    // bool isCmdEqual = data.mid(5, 1).startsWith(command.mid(5, 1));
    bool isCmdEqual = (data == command);

    //针对几个指令做特殊处理
    //1、基线采集完成标志位信号
    if(workStatus == BLSampleing)
    {
        if(data == Order::cmd_BLSamlpeFinish){
            workStatus = Prepared;
            emit sigBLSampleFinished();
            return;
        }
    }//2、测量完成指令
    else if(workStatus == Looping)
    {
        if(data == Order::cmd_measureFinish){//完成一次循环
            emit sigFinishCurrentloop();
            return;
        }
    }

    if (!isCmdEqual){
        if(cmdPool.size()>0){
            send(cmdPool.first());
        }
        qDebug()<<"返回指令与发送指令不一致";
        return;
    }

    cmdPool.erase(cmdPool.begin());
    if (cmdPool.size() > 0)
    {
        //发送开始测量指令
        send(cmdPool.first());
        qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
    }
    else{
        switch(workStatus)
        {
            // 进行状态切换，触发下一个流程的进行
            case Preparing:
                workStatus = Prepared;
                emit sigFPGA_prepared();
                break;
            case Stopping:
                workStatus = Prepared;
                emit sigFPGA_stoped();
                break;
            case BLSampleing:
                cmdPool.push_back(Order::cmd_BLSamlpeFinish); //基线采集会有一个单独的反馈指令
            default:
                break;
        }
    }
}

#include <functional>
void CommandHelper::netFrameWorkThead()
{
    qDebug() << "netFrameWorkThead thread id:" << QThread::currentThreadId();
    // while (!taskFinished)
    // {
    //     {
    //         QMutexLocker locker(&mutexCache);
    //         if (cachePool.size() < 5){
    //             // 数据单位最小值为0x21（一个指令长度）
    //             QThread::msleep(1);
    //         } else {
    //             qDebug() << "Recv HEX: " << binaryData.toHex(' ');
    //         }
    //     }
    // }
}
