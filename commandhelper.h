/*
 * @Author: MrPan
 * @Date: 2025-10-26 17:03:54
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-10-30 12:59:53
 * @Description: 请填写简介
 */
#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include "qlitethread.h"
#include <QMutex>
#include "common.h"
#include <log4qt/logger.h>

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
    void baseLineSample_manual();

    //先关闭之前FPGA的状态，循环前的准备工作
    void ressetFPGA();

    //进行循环前配置,是否自动采集基线
    void setConfigBeforeLoop(CommonUtils::UI_FPGAconfig config, ModeBLSample mode_BLsample);

    //进行一次测量
    void startOneLoop(CsvDataRow data);

    //测量结束后恢复状态
    void resetFPGA_afterMeasure();

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

    const bool getSerialPortStatus(){return m_SerialPort.isOpen();}


protected:
    enum WorkStatusFlag {
        NoWork = 0,     // 未开始
        Preparing,  // 初始化过程中...
        Prepared,   // 初始化完成
        LoopStep1,  // 注意其中有个电压稳定时间，该时间依靠软件界面定时器来实现延时。
        Looping,    // 开启硬件触发后，等待触发
        BLSampleing_manual, //采集基线中...
        BLSampleing_auto, //循环中的自动采集基线
        ConfigBeforeLoop, //循环前的指令配置
        MeasuringTemp, // 测量温度中...
        Stopping, // 停止测量中...
        Resetting, //没有自动基线测量，直接重置中。
        WorkEnd  // 测量结束
    };
private:
    virtual void onReadEvent(const char *portName, unsigned int readBufferLen);

private slots:
    //处理接收数据
    void handleData(QByteArray data);

signals:
    void sigLog(const QString& message, const QString& type);
    void sigUpdateReceive(QString str);
    void sigUpdateData(QByteArray data);

    void sigRebackUnbale(); //通知界面恢复控件使用
    void sigReset_finished(); //FPGA准备工作完成，初始化完成，可进行配置参数
    void sigBLSampleFinished(); //基线采样完成
    void sigConfigFinished(); //循环前的配置已经完成
    void sigLoop_stoped(); //停止测量已完成
    void sigMeasureFinished(); //完成一次测量，接下来关闭电源、DAC配置。。。等
    void sigFinishCurrentloop(); //当前循环结束
    void sigUpdateTemp(int id, double temp); //监测到的温度。id：1~4。温度：摄氏度

private:
    Log4Qt::Logger *logger;   // 日志
    CSerialPort m_SerialPort;
    QVector<QByteArray> cmdPool;
    QByteArray cachePool;
    QLiteThread* NetDataThread;//处理网络数据线程
    bool taskFinished = false;
    QMutex mutexCache;
    WorkStatusFlag workStatus = NoWork;
    ModeBLSample m_modeBLSample; //是否自动采集基线
};

#endif // COMMANDHELPER_H
