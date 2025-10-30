#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QJsonObject>

// #include "CSerialPort/SerialPort.h"
// #include "CSerialPort/SerialPortInfo.h"
// using namespace itas109;
#include "commandhelper.h"
#include "common.h"
#include <QCheckBox>

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

    //JSON 文件读写
    static QJsonObject ReadSetting(); // 读取配置文件
    static void WriteSetting(QJsonObject);  // 写配置文件
    static QString jsonPath;

private:
    void UIcontrolEnable(bool flag);
    bool validateCsvFile(const QString& filePath);
    // void onReadEvent(const char *portName, unsigned int readBufferLen);

    // 日志输出方法
    void logMessage(const QString& message, const QString& type = "INFO");
    void logDebug(const QString& message);
    void logInfo(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& message);

signals:
    void sigLogMessage(const QString& message); // 新增日志信号

private slots:
    void btnSelectFile_clicked();
    void onLogMessage(const QString& message); // 新增日志槽函数
    void OnUpdateReceive(QString str);
    void on_pushButtonOpen_clicked();
    void slot_FPGA_prepared(); //FPGA配置好了，可以进行循环
    void slot_measureStoped(); //测量已停止
    void slot_finishedOneLoop(); //完成一次循环，准备下一次循环

    void on_bt_startLoop_clicked();
    void onCheckStateChangedA(int state); // 勾选框状态变化时更新数值
    void onCheckStateChangedB(int state); // 勾选框状态变化时更新数值
    void onCheckAllAChanged(int state); //全部勾选与否的响应
    void onCheckAllBChanged(int state); //全部勾选与否的响应

    void on_bt_kernelReset_clicked();

    void on_bt_baseLineSample_clicked();

    // 基线采集模式选择
    void onBLmodeToggled(bool checked);

private:
    Ui::MainWindow *ui;
    QString m_loopFile;
    CommandHelper *commManager; // 其他类的对象

    QVector<CsvDataRow> dataLEDPara; //光强，DAC配置文件
    QVector<CsvDataRow> tempLEDdata; //当前循环的光强循环区间

    unsigned short m_RegisterA; // 移位寄存器A（位号6~15有效）
    unsigned short m_RegisterB; // 移位寄存器B（位号6~15有效）
    QList<QCheckBox*> m_checksA; // 存储勾选框指针，便于批量操作
    QList<QCheckBox*> m_checksB; // 存储勾选框指针，便于批量操作
    QList<int> m_bitMap; // 位号映射表：索引对应check1~check10，值为对应位号
};
#endif // MAINWINDOW_H
