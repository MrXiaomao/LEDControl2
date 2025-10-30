#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include "qlitethread.h"
#include <QMutex>
#include "common.h"

#include <QObject>
#include "CSerialPort/SerialPort.h"
#include "CSerialPort/SerialPortInfo.h"
using namespace itas109;

//定义每次循环需要使用的参数
typedef struct tagLEDParameter{
    // 触发阈值
    int16_t triggerThold1;
} LEDParameter;

class CommandHelper : public QObject, public CSerialPortListener
{
    Q_OBJECT
public:
    explicit CommandHelper(QObject *parent = nullptr);
    ~CommandHelper();

    // 初始化串口（设置参数、打开）
    bool init(const QString portName, int baudRate);
    // 关闭串口
    void close();

    void send(QByteArray cmd);

    //采集基线
    void baseLineSample();

    //先关闭之前FPGA的状态，循环前的准备工作
    void ressetFPGA();

    //进行一次测量
    void startOneLoop(CsvDataRow data);

    //停止测量
    void stopMeasure();

    /**
     * @brief setCommonConfig 配置FPGA一些公共参数
     * @param width 发光宽度,单位ns
     * @param times 发光次数
     * @param delaytime 发光延迟,单位ns，默认1000s
     */
    void setCommonConfig(unsigned short width, unsigned short times, unsigned int delaytime = 1000);

    //串口开始工作
    void startWork();

    //网口原始数据解析线程
    void netFrameWorkThead();



protected:
    enum WorkStatusFlag {
        NoWork = 0,     // 未开始
        Preparing = 1,  // 初始化过程中...
        Prepared = 2,  // 初始化完成
        Looping = 3,  // 循环配置中...
        BLSampleing = 4, //采集基线中...
        MeasuringTemp = 5,  // 测量温度中...
        Stopping = 6, // 停止测量中...
        WorkEnd = 7    // 测量结束
    };
private:
    virtual void onReadEvent(const char *portName, unsigned int readBufferLen);

private slots:
    //处理接收数据
    void handleData(QByteArray data);

signals:
    void sigUpdateReceive(QString str);
    void sigUpdateData(QByteArray data);

    void sigFPGA_prepared(); //FPGA准备工作完成，初始化完成，可进行配置参数
    void sigBLSampleFinished(); //基线采样完成
    void sigFPGA_stoped(); //停止测量已完成
    void sigFinishCurrentloop(); //当前循环结束

private:
    CSerialPort m_SerialPort;
    QVector<QByteArray> cmdPool;
    QByteArray cachePool;
    QLiteThread* NetDataThread;//处理网络数据线程
    bool taskFinished = false;
    QMutex mutexCache;
    WorkStatusFlag workStatus = NoWork;
};

#endif // COMMANDHELPER_H
