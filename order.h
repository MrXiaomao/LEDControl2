/*
 * @Author: MrPan
 * @Date: 2025-10-26 20:49:38
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-10-28 21:43:32
 * @Description: 请填写简介
 */
#ifndef ORDER_H
#define ORDER_H
#include <QByteArray>

class Order
{
public:
    Order();

    //1、关闭电源
    static const QByteArray cmd_closePower;
    //2、关闭DAC配置
    static const QByteArray cmd_closeDAC;
    //3、关闭硬件触发
    static const QByteArray cmd_closeHardTrigger;
    //4、关闭基线采集
    static const QByteArray cmd_closeBLSamlpe;
    //5、复位
    static const QByteArray cmd_resetRegister;
    //6、关闭温度监测
    static const QByteArray cmd_closeTempMonitor;
    //7、配置温度监测频率,其中X为16进制数，范围1-A，单位s，默认值1
    static QByteArray cmd_TempPara;
    //8、配置LED发光宽度,其中XX XX为16进制数，单位为×10ns，例如300ns对应的指令为：12 02 00 1E DD
    static QByteArray cmd_LEDWidth;
    //9、配置同步触发宽度，其中XX XX为16进制数，单位为×10ns，例如300ns对应的指令为：12 02 00 1E DD
    static QByteArray cmd_TriggerWidth;
    //10、配置LED发光延迟时间,其中XX XX和YY YY为16进制数，单位10ns，默认1μs，上限10ms
    static QByteArray cmd_LightDelayTimeA;
    static QByteArray cmd_LightDelayTimeB;
    //11、配置同步触发延迟时间,其中XX XX和YY YY为16进制数，单位10ns，默认1μs，上限10ms
    static QByteArray cmd_TriggerDelayTimeA;
    static QByteArray cmd_TriggerDelayTimeB;
    //12、配置移位寄存器时钟频率，单位10ns。
    static QByteArray cmd_clockFrequency;
    //13、配置硬件触发高电平点数，默认为1，最小值为1，软件保护，不能为0
    static QByteArray cmd_HLpoint;
    //14、配置LED发光次数,默认1000
    static QByteArray cmd_timesLED;
    //14+、配置同步触发次数,保持数值跟LED发光次数一致
    static QByteArray cmd_timesTrigger;
    //15、配置移位寄存器数据
    static QByteArray cmd_registerConfigA;
    static QByteArray cmd_registerConfigB;
    //16、开启移位寄存器配置
    static const QByteArray cmd_openRegConfig;
    //17、开启温度监测
    static const QByteArray cmd_openTempMonitor;
    //18、配置DAC数据,12 07 YX XX DD,其中Y对应DAC，取1-A，共10个DAC，XXX对应DAC输出电压，默认为0
    static QByteArray cmd_voltConfig;
    //19、写入DAC数据
    static const QByteArray cmd_writeDAC;
    //20、开启电源
    static const QByteArray cmd_openPower;
    //21、开启硬件触发
    static const QByteArray cmd_HardTriggerOn;
    //23、接收测量完成指令
    static const QByteArray cmd_measureFinish;
    //28、开启采集基线
    static const QByteArray cmd_openBLSamlpe;
    //29、基线采集完成指令反馈
    static const QByteArray cmd_BLSamlpeFinish;

    //温度监测时间间隔，s，范围1-A
    static QByteArray getTempMonitorGap(unsigned char deltaT);

    //LED发光宽度，输入单位为ns，例如300ns对应：00 1E
    static QByteArray getLEDWidth(unsigned short width);

    //同步触发宽度，单位为ns，例如300ns对应：00 1E
    static QByteArray getTriggerWidth(unsigned short width=300);

    //LED发光延迟时间(int数值低八位)，单位为ns，上限10ms(1E7ns), 例如300ns对应：00 1E
    static QByteArray getLightDelayTimeA(unsigned int delaytime = 1000);
    //LED发光延迟时间(int数值高八位)，单位为ns，上限10ms(1E7ns)，例如300ns对应：00 1E
    static QByteArray getLightDelayTimeB(unsigned int delaytime = 1000);

    //同步触发延迟时间(int数值低八位)，单位为ns，上限10ms(1E7ns), 例如300ns对应：00 1E
    static QByteArray getTriggerDelayTimeA(unsigned int delaytime = 1000);
    //同步触发延迟时间(int数值高八位)，单位为ns，上限10ms(1E7ns), 例如300ns对应：00 1E
    static QByteArray getTriggerDelayTimeB(unsigned int delaytime = 1000);

    //时钟频率,单位ns，默认10ns，必须是10的整数倍
    static QByteArray getClockFrequency(unsigned short frequency = 10);

    //硬件触发高电平点数,单位ns，默认值10ns，不能为0
    static QByteArray getHLpoint(unsigned char duratime = 10);

    //LED发光次数,默认1000
    static QByteArray getTimesLED(unsigned short times = 1000);

    // 同步触发次数,默认1000
    static QByteArray getTimesTrigger(unsigned short times = 1000);

    //移位寄存器数据,控制灯光的点亮与否，待定
    static QByteArray getRegisterConfigA(unsigned short type);

    //移位寄存器数据
    static QByteArray getRegisterConfigB(unsigned short type);

    //通过DAC配置电压
    static QByteArray getVoltConfig(unsigned char id, unsigned short volt);
};

#endif // ORDER_H
