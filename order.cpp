#include "order.h"

Order::Order() {}
enum LEDchannel //各通道对应寄存器的位置，从右为第一位。
{
    ch1_0 = 8,
    ch1_1 = 9,
    ch2_0 = 10,
    ch2_1 = 11,
    ch3_0 = 12,
    ch3_1 = 13,
    ch4_0 = 14,
    ch4_1 = 15,
    ch5_0 = 6,
    ch5_1 = 7
};

//1、关闭电源
const QByteArray Order::cmd_closePower("\x12\x06\x00\x00\xDD", 5);
//2、关闭DAC配置
const QByteArray Order::cmd_closeDAC("\x12\x08\x00\x00\xDD", 5);
//3、关闭硬件触发
const QByteArray Order::cmd_closeHardTrigger("\x12\x0D\x00\x01\xDD", 5);
//3+、关闭硬件触发
const QByteArray Order::cmd_closeTriggerA("\x12\x2D\x00\x01\xDD", 5);
//3++、关闭硬件触发
const QByteArray Order::cmd_closeTriggerB("\x12\x3D\x00\x01\xDD", 5);

//4、关闭基线采集
const QByteArray Order::cmd_closeBLSamlpe("\x12\x1D\x00\x01\xDD", 5);
//5、移位寄存器时钟和复位
const QByteArray Order::cmd_resetRegister("\x12\x05\x00\x01\xDD", 5);
//6、关闭温度监测
const QByteArray Order::cmd_closeTempMonitor("\x12\x0A\x00\x00\xDD", 5);
//7、配置温度监测频率,其中X为16进制数，范围1-A，单位s，默认值1
QByteArray Order::cmd_TempPara("\x12\x09\x00\x01\xDD", 5);
//8、配置LED发光宽度,其中XX XX为16进制数，单位为×10ns，例如300ns对应的指令为：12 02 00 1E DD
QByteArray Order::cmd_LEDWidth("\x12\x02\x00\x1E\xDD", 5);
//9、配置同步触发宽度，其中XX XX为16进制数，单位为×10ns，例如300ns对应的指令为：12 02 00 1E DD
QByteArray Order::cmd_TriggerWidth("\x12\x12\x00\x1E\xDD", 5);
//10、配置LED发光延迟时间,其中XX XX和YY YY为16进制数，单位x10ns，默认1μs，上限10ms
QByteArray Order::cmd_LightDelayTimeA("\x12\x01\x00\x00\xDD", 5);
QByteArray Order::cmd_LightDelayTimeB("\x12\x11\x00\x01\xDD", 5);
//11、配置同步触发延迟时间,其中XX XX和YY YY为16进制数，单位x10ns，默认1μs，上限10ms
QByteArray Order::cmd_TriggerDelayTimeA("\x12\x21\x00\x00\xDD", 5);
QByteArray Order::cmd_TriggerDelayTimeB("\x12\x31\x00\x00\xDD", 5);
//12、配置移位寄存器时钟频率
QByteArray Order::cmd_clockFrequency("\x12\x03\x00\x0A\xDD", 5);
//13、配置硬件触发高电平点数,单位×10ns，默认值10ns，软件保护，不能为0
QByteArray Order::cmd_HLpoint("\x12\x0E\x00\x01\xDD", 5);
//14、配置LED发光次数,默认1000
QByteArray Order::cmd_timesLED("\x12\x0B\x03\xE8\xDD", 5);
//14+、配置配置同步触发次数
QByteArray Order::cmd_timesTrigger("\x12\x1B\x03\xE8\xDD", 5);
//15、配置移位寄存器数据
QByteArray Order::cmd_registerConfigA("\x12\x04\x00\x00\xDD", 5);
QByteArray Order::cmd_registerConfigB("\x12\x14\x00\x00\xDD", 5);
//16、开启移位寄存器配置
const QByteArray Order::cmd_openRegConfig("\x12\x05\x00\x10\xDD", 5);
//17、开启温度监测
const QByteArray Order::cmd_openTempMonitor("\x12\x0A\x00\x01\xDD", 5);
//18、配置DAC数据,12 07 YX XX DD,其中Y对应DAC，取1-A，共10个DAC，XXX对应DAC输出电压，默认为0
QByteArray Order::cmd_voltConfig("\x12\x07\x00\x01\xDD", 5);
//19、写入DAC数据
const QByteArray Order::cmd_writeDAC("\x12\x08\x00\x01\xDD", 5);
//20、开启电源
const QByteArray Order::cmd_openPower("\x12\x06\x11\x00\xDD", 5);
//21-AB、开启硬件触发
const QByteArray Order::cmd_TriggerAB_On("\x12\x0C\x00\x01\xDD", 5);
//21-A、开启硬件触发
const QByteArray Order::cmd_TriggerA_On("\x12\x2C\x00\x01\xDD", 5);
//21-B、开启硬件触发
const QByteArray Order::cmd_TriggerB_On("\x12\x3C\x00\x01\xDD", 5);
//23-AB、接收AB测量完成指令
const QByteArray Order::cmd_measureFinishAB("\x12\xF5\x00\x00\xDD", 5);
//23-A、接收A测量完成指令
const QByteArray Order::cmd_measureFinishA("\x12\xF7\x00\x00\xDD", 5);
//23-B、接收B测量完成指令
const QByteArray Order::cmd_measureFinishB("\x12\xF8\x00\x00\xDD", 5);
//28、开启采集基线
const QByteArray Order::cmd_openBLSamlpe("\x12\x1C\x00\x01\xDD", 5);
//29、基线采集完成指令反馈
const QByteArray Order::cmd_BLSamlpeFinish("\x12\xF6\x00\x00\xDD", 5);

QByteArray Order::getTempMonitorGap(unsigned char frequency)
{
    QByteArray tempcmd = cmd_TempPara;
    if(frequency>0 && frequency <0x0B)
    {
        tempcmd[3] = frequency;
    }
    return tempcmd;
}

QByteArray Order::getLEDWidth(unsigned short width)
{
    QByteArray tempcmd = cmd_LEDWidth;
    tempcmd[2] = static_cast<quint8>((width >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(width & 0xFF);
    return tempcmd;
}

QByteArray Order::getTriggerWidth(unsigned short width)
{
    QByteArray tempcmd = cmd_TriggerWidth;
    tempcmd[2] = static_cast<quint8>((width >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(width & 0xFF);
    return tempcmd;
}

QByteArray Order::getLightDelayTimeA(unsigned int delaytime)
{
    //上限10ms=1E6*10ns
    if(delaytime>1E6) delaytime = 1E6;
    QByteArray tempcmd = cmd_LightDelayTimeA;
    tempcmd[2] = static_cast<quint8>((delaytime >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(delaytime & 0xFF);
    return tempcmd;
}

QByteArray Order::getLightDelayTimeB(unsigned int delaytime)
{
    //上限10ms=1E6*10ns
    if(delaytime>1E6) delaytime = 1E6;
    //转化为x10ns的单位
    QByteArray tempcmd = cmd_LightDelayTimeB;
    tempcmd[2] = static_cast<quint8>((delaytime >> 24) & 0xFF);
    tempcmd[3] = static_cast<quint8>((delaytime >> 16) & 0xFF);
    return tempcmd;
}

QByteArray Order::getTriggerDelayTimeA(unsigned int delaytime)
{
    //上限10ms=1E6*10ns
    if(delaytime>1E6) delaytime = 1E6;
    QByteArray tempcmd = cmd_TriggerDelayTimeA;
    tempcmd[2] = static_cast<quint8>((delaytime >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(delaytime & 0xFF);
    return tempcmd;
}

QByteArray Order::getTriggerDelayTimeB(unsigned int delaytime)
{
    //上限10ms=1E6*10ns
    if(delaytime>1E6) delaytime = 1E6;
    QByteArray tempcmd = cmd_TriggerDelayTimeB;
    tempcmd[2] = static_cast<quint8>((delaytime >> 24) & 0xFF);
    tempcmd[3] = static_cast<quint8>((delaytime >> 16) & 0xFF);
    return tempcmd;
}

//时钟频率,单位*10ns，默认1*10ns
QByteArray Order::getClockFrequency(unsigned short frequency)
{
    QByteArray tempcmd = cmd_clockFrequency;
    tempcmd[2] = static_cast<quint8>((frequency >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(frequency & 0xFF);
    return tempcmd;
}

QByteArray Order::getHLpoint(unsigned char duraTime)
{
    //确保数值大于0
    if(duraTime==0) duraTime = 1;
    QByteArray tempcmd = cmd_HLpoint;
    tempcmd[3] = duraTime;
    return tempcmd;
}

QByteArray Order::getTimesLED(unsigned short times)
{
    QByteArray tempcmd = cmd_timesLED;
    tempcmd[2] = static_cast<quint8>((times >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(times & 0xFF);
    return tempcmd;
}

QByteArray Order::getTimesTrigger(unsigned short times)
{
    QByteArray tempcmd = cmd_timesTrigger;
    tempcmd[2] = static_cast<quint8>((times >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(times & 0xFF);
    return tempcmd;
}

QByteArray Order::getRegisterConfigA(unsigned short value)
{
    QByteArray tempcmd = cmd_registerConfigA;
    tempcmd[2] = static_cast<quint8>((value >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(value & 0xFF);
    return tempcmd;
}

QByteArray Order::getRegisterConfigB(unsigned short value)
{
    QByteArray tempcmd = cmd_registerConfigB;
    tempcmd[2] = static_cast<quint8>((value >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(value & 0xFF);
    return tempcmd;
}

QByteArray Order::getVoltConfig(unsigned char id, unsigned short volt)
{
    unsigned short result; // 拼接后的结果
    result = ((id & 0x0F)<<12) | (volt & 0x0FFF);

    QByteArray tempcmd = cmd_voltConfig;
    tempcmd[2] = static_cast<quint8>((result >> 8) & 0xFF);
    tempcmd[3] = static_cast<quint8>(result & 0xFF);

    return tempcmd;
}
