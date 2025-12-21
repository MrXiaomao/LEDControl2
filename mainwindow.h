/*
 * @Author: MrPan
 * @Date: 2025-11-09 20:44:23
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-11-28 23:10:29
 * @Description: 请填写简介
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include "logparser.h"

// #include "CSerialPort/SerialPort.h"
// #include "CSerialPort/SerialPortInfo.h"
// using namespace itas109;
#include "commandhelper.h"
#include "common.h"
#include <QCheckBox>

#include "uilogappender.h"
#include<random>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void readCsvFile(const QString &filePath);
    void ConfigLog();

private:
    void UIcontrolEnable(bool flag);
    bool validateCsvFile(const QString& filePath);
    void refreshSerialPort();
    void loadCheckBoxStateFromJson(); // 新增：从配置文件读取勾选状态

    void loadUiConfigFromJson();   // 读取配置文件并加载控件值
    void saveUiConfigToJson();     // 保存控件值到配置文件

signals:

private slots:
    void on_packageLostCount_clicked();
    void btnSelectFile_clicked();
    void onLogMessage(const QString& message); // 新增日志槽函数
    void OnUpdateReceive(QString str);
    void on_pushButtonOpen_clicked();

    void onSerialError(const QString& msg); //处理串口相关错误信息
    void slot_RebackUnable(); //恢复界面按键使用，
    void slot_Reset_finished(); //FPGA配置好了，可以进行循环
    void slot_config_finished(); //循环前的参数配置已经完成
    void slot_measureStoped(); //测量已停止
    void slot_finishedOneLoop(); //完成一次循环，准备下一次循环
    void slot_updateTemp(int id, double temp); //更新界面的温度

    void on_bt_startLoop_clicked();
    void onCheckStateChangedA(int state); // 勾选框状态变化时更新数值
    void onCheckStateChangedB(int state); // 勾选框状态变化时更新数值
    void onCheckAllAChanged(int state); //全部勾选与否的响应
    void onCheckAllBChanged(int state); //全部勾选与否的响应

    void on_bt_kernelReset_clicked();

    void on_bt_baseLineSample_clicked();

    // 基线采集模式选择
    void onBLmodeToggled(bool checked);

    void on_bt_refreshPort_clicked();
//=======================================
    void onBaseLineSampleFinished();  // 新增：基线采集完成处理

    void random_sleep_ms(int min_ms, int max_ms); //新增随机延时函数
//=======================================

    void on_clearLogButton_clicked();

    void onLoopTypeChanged();

    void on_singleMeasure_clicked();
    
    void on_action_about_triggered();

    void on_fanControlButton_clicked();//新增风扇控制

private:
    LogParser parser;
    Ui::MainWindow *ui;
    Log4Qt::Logger *logger; // Log4Qt日志器
    UiLogAppender *uiAppender; // UI日志附加器
    QString m_loopFile;
    CommandHelper *commManager; // 其他类的对象

    QVector<CsvDataRow> dataLEDPara; //光强，DAC配置文件
    QVector<CsvDataRow> tempLEDdata; //当前循环的光强循环区间

    ModeBLSample m_BLmode; //采集基线的类型
    unsigned short m_RegisterA = 0; // 移位寄存器A（位号6~15有效）
    unsigned short m_RegisterB = 0; // 移位寄存器B（位号6~15有效）
    QList<QCheckBox*> m_checksA; // 存储勾选框指针，便于批量操作
    QList<QCheckBox*> m_checksB; // 存储勾选框指针，便于批量操作
    QList<int> m_bitMap; // 位号映射表：索引对应check1~check10，值为对应位号
    ModeLoop m_loopType = LoopAB;
    bool m_fanStatus = false;  // 风扇状态：false=关闭, true=开启
    MeasureType m_currentMeasureType = NoMeasure; // 当前测量类型
};
#endif // MAINWINDOW_H
