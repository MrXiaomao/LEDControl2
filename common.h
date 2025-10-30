#ifndef COMMON_H
#define COMMON_H

#include <QVector>
#include <QByteArray>
#include <QJsonObject>

// 声明常用结构体
struct CsvDataRow {
    double ledIntensity;       // 第一列：光强（double）
    int dacValues[10];    // 其他列：DAC值（int）
};

//采集基线的类型
enum ModeBLSample {
    ManualBL = 0, // 手动
    AutoBL        // 自动
};

// 声明常用函数（工具函数）
namespace CommonUtils {
    extern QString jsonPath;

    //JSON 文件读写
    QJsonObject ReadSetting(); // 读取配置文件
    void WriteSetting(QJsonObject);  // 写配置文件
    struct UserConfig {
        int PowerStableTime;//开启电源后，界面需要定时延迟的时间，单位ms
        int TempMonitorGap; //温度监测时间间隔，单位ns，必须10的整数倍
        int TriggerWidth; //同步触发宽度，单位为ns，必须10的整数倍
        int clockFrequency; //移位寄存器时钟频率，单位ns，必须10的整数倍
        int HLpoint; //硬件触发高电平点数,一个点对应10ns，需大于0
    };

    //界面上用于控制FPGA的参数
    struct UI_FPGAconfig {
        int LEDWidth; //发光宽度，单位ns，必须10的整数倍
        int LightDelayTime;//LED发光延迟时间，单位为ns，必须10的整数倍，上限10ms(1E7ns),
        int TriggerDelayTime;//同步触发延迟时间(int数值低八位)，单位为ns，必须10的整数倍
        int timesLED;//LED发光次数
    };

    //新增读取并校验函数读取"User"组数值。
    UserConfig loadUserConfig();

    unsigned short loadJson_heckValueA();
    unsigned short loadJson_heckValueB();
}

#endif // COMMON_H
