/*
 * @Author: MrPan
 * @Date: 2025-10-26 10:14:52
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-11-09 21:37:06
 * @Description: 请填写简介
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QDebug>
#include "order.h"

#include <log4qt/logger.h>
#include <log4qt/patternlayout.h>
#include <log4qt/rollingfileappender.h>
#include <log4qt/propertyconfigurator.h>
#include <log4qt/logManager.h>
#include "uilogappender.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 步骤1：将check1~check10按顺序加入列表（索引0~9对应check1~check10）
    m_checksA << ui->checkA1
             << ui->checkA2
             << ui->checkA3
             << ui->checkA4
             << ui->checkA5
             << ui->checkA6
             << ui->checkA7
             << ui->checkA8
             << ui->checkA9
             << ui->checkA10;

    m_checksB << ui->checkB1
              << ui->checkB2
              << ui->checkB3
              << ui->checkB4
              << ui->checkB5
              << ui->checkB6
              << ui->checkB7
              << ui->checkB8
              << ui->checkB9
              << ui->checkB10;

    // 步骤2：定义位号映射表（关键！按check1~check10的顺序对应位号）
    m_bitMap = {8, 9, 10, 11, 12, 13, 14, 15, 6, 7};

    // 步骤3：关联所有勾选框的状态变化信号
    foreach (QCheckBox *check, m_checksA) {
        connect(check, &QCheckBox::stateChanged,
                this, &MainWindow::onCheckStateChangedA);
    }

    foreach (QCheckBox *check, m_checksB) {
        connect(check, &QCheckBox::stateChanged,
                this, &MainWindow::onCheckStateChangedB);
    }

    // 创建其他类的对象（确保父对象正确，避免内存泄漏）
    commManager = new CommandHelper(this);

    //控件绑定打开文件标志符
    QAction *action = ui->lEdit_File->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);

    // 获取关联的按钮并连接信号
    QList<QWidget*> associatedWidgets = action->associatedWidgets();
    if (!associatedWidgets.isEmpty()) {
        QToolButton* button = qobject_cast<QToolButton*>(associatedWidgets.last());
        if (button) {
            button->setCursor(QCursor(Qt::PointingHandCursor));
            // 修正连接语法
            connect(button, &QToolButton::clicked, this, &MainWindow::btnSelectFile_clicked);
        }
    }

    ui->bt_startLoop->setEnabled(false);

    // 连接日志信号槽
    // connect(commManager, &CommandHelper::sigUpdateReceive, this, &MainWindow::OnUpdateReceive, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigRebackUnbale, this, &MainWindow::slot_RebackUnable, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigReset_finished, this, &MainWindow::slot_Reset_finished, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigConfigFinished, this, &MainWindow::slot_config_finished, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigLoop_stoped, this, &MainWindow::slot_measureStoped, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigUpdateTemp, this, &MainWindow::slot_updateTemp, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigFinishCurrentloop, this, &MainWindow::slot_finishedOneLoop, Qt::QueuedConnection);

    // 2. （可选）关联选中状态变化的信号（两个单选按钮可共用一个槽函数）
    connect(ui->radioButton_auto, &QRadioButton::toggled,this, &MainWindow::onBLmodeToggled);
    connect(ui->radioButton_manual, &QRadioButton::toggled,this, &MainWindow::onBLmodeToggled);

    connect(ui->checkA_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllAChanged);
    connect(ui->checkB_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllBChanged);

    ConfigLog();

    //初始化串口
    refreshSerialPort();

    // loadCheckBoxStateFromJson();
    loadUiConfigFromJson();
}

MainWindow::~MainWindow()
{
    //退出时关闭串口
    commManager->close();

    //将界面参数保存到json文件中
    saveUiConfigToJson();

    // 1️⃣ 从 logger 移除自定义附加器（防止退出时再触发 append）
    logger->debug("软件正常退出！");
    if (logger && uiAppender) {
        logger->removeAppender(uiAppender);
        uiAppender->close();
        delete uiAppender;
        uiAppender = nullptr;
    }

    // 2️⃣ 可选：关闭 Log4Qt（确保所有 appender 文件安全关闭）
    Log4Qt::LogManager::shutdown();

    // 3️⃣ 最后删除 UI
    delete commManager;
    delete ui;
}

//刷新串口
void MainWindow::refreshSerialPort()
{
    //获取端口
    std::vector<SerialPortInfo> portNameList = CSerialPortInfo::availablePortInfos();
    if (portNameList.size() > 0)
    {
        QString message = "Avaiable Port is refreshed";
        // QString currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        // QString consoleEntry = QString("[%1] %2: %3").arg(currentTimestamp, "info", message);

        // //考虑到界面初始化调用此函数的时候，无法打印日志到界面
        // ui->textEdit_Log->moveCursor (QTextCursor::End);
        // ui->textEdit_Log->insertPlainText(consoleEntry);
        logger->info(message);
    }
    else
    {
        // ui->textEdit_Log->moveCursor (QTextCursor::End);
        // ui->textEdit_Log->insertPlainText("No avaiable Port");
        logger->fatal("No avaiable Port");
        return;
    }

    //串口已打开
    if(commManager->getSerialPortStatus()){
        //记录当前端口
        QString currentPortName = ui->comboBoxPortName->currentText();
        ui->comboBoxPortName->clear();
        ui->comboBoxPortName->insertItem(0, currentPortName);
        int index = 0;
        for (size_t i = 0; i < portNameList.size(); i++)
        {
            QString portName = QString::fromLocal8Bit(portNameList[i].portName);
            if(currentPortName == portName){
                continue;
            }
            ui->comboBoxPortName->insertItem(index++, portName);
        }

    }
    else //串口本身未打开状态
    {
        ui->comboBoxPortName->clear();
        for (size_t i = 0; i < portNameList.size(); i++)
        {
            QString portName = QString::fromLocal8Bit(portNameList[i].portName);
            ui->comboBoxPortName->insertItem(i, portName);
        }
        //ui
        ui->pushButtonOpen->setText(tr("open"));
    }
}

//配置日志输出相关信息
void MainWindow::ConfigLog()
{
    // === 确保日志目录存在 ===
    QDir dir("logs");
    if (!dir.exists())
        dir.mkpath(".");

    // === 1️⃣ 初始化 Log4Qt 配置 ===
    QString confPath = QCoreApplication::applicationDirPath() + "/config/log4qt.conf";
    if (QFile::exists(confPath)) {
        Log4Qt::PropertyConfigurator::configureAndWatch(confPath);
    } else {
        qWarning() << "配置文件不存在:" << confPath;
    }

    // === 2️⃣ 获取根 logger ===
    logger = Log4Qt::Logger::rootLogger();

    // === 3️⃣ 添加 UI 日志附加器 ===
    uiAppender = new UiLogAppender(this);
    Log4Qt::PatternLayout *layout = new Log4Qt::PatternLayout();
    layout->setConversionPattern("%d{yyyy-MM-dd HH:mm:ss.zzz} [%p] %m");//不换行，如果换行：("%d{yyyy-MM-dd HH:mm:ss} [%p] %m%n");
    layout->activateOptions();

    uiAppender->setLayout(layout);
    uiAppender->activateOptions();
    logger->addAppender(uiAppender);

    // === 4️⃣ 连接信号，用于更新界面日志 ===
    connect(uiAppender, &UiLogAppender::logToUi, this, &MainWindow::onLogMessage, Qt::QueuedConnection);

    logger->info("程序启动");
}

void MainWindow::loadCheckBoxStateFromJson()
{
    QJsonObject json = CommonUtils::ReadSetting();
    QJsonObject uiJson = json.value("UI").toObject();

    if (uiJson.isEmpty()) {
        logger->warn("读取\"checkValueA\"，配置文件中缺少 UI 节点");
    }

    int checkValueA = uiJson.value("checkValueA").toInt(0);
    int checkValueB = uiJson.value("checkValueB").toInt(0);

    logger->info(QString("从配置文件加载勾选状态: checkValueA=%1, checkValueB=%2")\
                     .arg(checkValueA).arg(checkValueB));

    // === 更新 A 组 ===
    for (int i = 0; i < m_checksA.size(); ++i) {
        int bitPos = m_bitMap[i];
        bool isChecked = (checkValueA >> bitPos) & 0x01;

        // 避免触发 onCheckStateChangedA 的信号递归
        disconnect(m_checksA[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedA);
        m_checksA[i]->setChecked(isChecked);
        connect(m_checksA[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedA);
    }

    // === 更新 B 组 ===
    for (int i = 0; i < m_checksB.size(); ++i) {
        int bitPos = m_bitMap[i];
        bool isChecked = (checkValueB >> bitPos) & 0x01;

        disconnect(m_checksB[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedB);
        m_checksB[i]->setChecked(isChecked);
        connect(m_checksB[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedB);
    }

    // === 更新寄存器缓存值 ===
    m_RegisterA = checkValueA;
    m_RegisterB = checkValueB;

    // === 更新“全选”勾选框状态 ===
    bool allA = std::all_of(m_checksA.begin(), m_checksA.end(),
                            [](QCheckBox *c){ return c->isChecked(); });
    bool allB = std::all_of(m_checksB.begin(), m_checksB.end(),
                            [](QCheckBox *c){ return c->isChecked(); });

    ui->checkA_all->setChecked(allA);
    ui->checkB_all->setChecked(allB);

    logger->info("界面勾选状态加载完成。");
}

void MainWindow::loadUiConfigFromJson()
{
    QJsonObject json = CommonUtils::ReadSetting();
    QJsonObject uiJson = json.value("UI").toObject();

    if (uiJson.isEmpty()) {
        logger->warn("配置文件中缺少 UI 节点");
        return;
    }

    // === 整数控件 ===
    ui->spinBox_LEDWidth->setValue(uiJson.value("LEDWidth").toInt(10));
    ui->spinBox_lightDelayTime->setValue(uiJson.value("LightDelayTime").toInt(1000));
    ui->spinBox_TriggerDelayTime->setValue(uiJson.value("TriggerDelayTime").toInt(1000));
    ui->spinBox_timesLED->setValue(uiJson.value("timesLED").toInt(1000));

    // === 浮点控件 ===
    ui->doubleSpinBox_loopStart->setValue(uiJson.value("IntensityLeft").toDouble(10.0));
    ui->doubleSpinBox_loopEnd->setValue(uiJson.value("IntensityRight").toDouble(200.0));

    // === 勾选框控件 ===
    int checkValueA = uiJson.value("checkValueA").toInt(0);
    int checkValueB = uiJson.value("checkValueB").toInt(0);

    // === 更新 A 组 ===
    for (int i = 0; i < m_checksA.size(); ++i) {
        int bitPos = m_bitMap[i];
        bool isChecked = (checkValueA >> bitPos) & 0x01;

        // 避免触发 onCheckStateChangedA 的信号递归
        disconnect(m_checksA[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedA);
        m_checksA[i]->setChecked(isChecked);
        connect(m_checksA[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedA);
    }

    // === 更新 B 组 ===
    for (int i = 0; i < m_checksB.size(); ++i) {
        int bitPos = m_bitMap[i];
        bool isChecked = (checkValueB >> bitPos) & 0x01;

        disconnect(m_checksB[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedB);
        m_checksB[i]->setChecked(isChecked);
        connect(m_checksB[i], &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedB);
    }

    // === 更新寄存器缓存值 ===
    m_RegisterA = checkValueA;
    m_RegisterB = checkValueB;

    // === 更新“全选”勾选框状态 ===
    bool allA = std::all_of(m_checksA.begin(), m_checksA.end(),
                            [](QCheckBox *c){ return c->isChecked(); });
    bool allB = std::all_of(m_checksB.begin(), m_checksB.end(),
                            [](QCheckBox *c){ return c->isChecked(); });

    ui->checkA_all->setChecked(allA);
    ui->checkB_all->setChecked(allB);

    // === 基线采集模式控件 ===
    bool modeBL = uiJson.value("BLSample_auto").toBool(true);
    m_BLmode = modeBL?AutoBL:ManualBL;
    ui->radioButton_auto->setChecked(m_BLmode==AutoBL);

    // === 文件选择 ===
    QString fileName = uiJson.value("loop_file").toString("");
    ui->lEdit_File->setText(fileName);
    if (validateCsvFile(fileName)){
        ui->lEdit_File->setStyleSheet("QLineEdit { color: green; }");
        readCsvFile(fileName);
    }

    logger->info("界面参数已从配置文件加载。");
}

void MainWindow::saveUiConfigToJson()
{
    QJsonObject json = CommonUtils::ReadSetting();
    QJsonObject uiJson = json.value("UI").toObject();

    uiJson["LEDWidth"]          = ui->spinBox_LEDWidth->value();
    uiJson["LightDelayTime"]    = ui->spinBox_lightDelayTime->value();
    uiJson["TriggerDelayTime"]  = ui->spinBox_TriggerDelayTime->value();
    uiJson["timesLED"]          = ui->spinBox_timesLED->value();
    uiJson["IntensityLeft"]     = ui->doubleSpinBox_loopStart->value();
    uiJson["IntensityRight"]    = ui->doubleSpinBox_loopEnd->value();
    uiJson["loop_file"]         = ui->lEdit_File->text();
    uiJson["BLSample_auto"]     = ui->radioButton_auto->isChecked();

    // 同时保存勾选框状态
    uiJson["checkValueA"] = m_RegisterA;
    uiJson["checkValueB"] = m_RegisterB;

    json["UI"] = uiJson;
    CommonUtils::WriteSetting(json);

    logger->info("界面参数已保存到配置文件。");
}

void MainWindow::OnUpdateReceive(QString str)
{
    logger->info(QString("Rec(Hex):%1").arg(str));
}

void MainWindow::btnSelectFile_clicked()
{
    // 设置文件过滤器
    QString filter = "csv文件 (*.csv);";

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择csv文件",
        QApplication::applicationDirPath(), // 默认路径
        filter
    );

    if (!fileName.isEmpty()) {
        m_loopFile = fileName;
        ui->lEdit_File->setText(m_loopFile);
        logger->info(QString("选择文件: %1").arg(fileName));

        // 验证文件
        if (validateCsvFile(fileName)) {
            ui->lEdit_File->setStyleSheet("QLineEdit { color: green; }");

            // 保存到配置文件
            QJsonObject jsonSetting = CommonUtils::ReadSetting();
            jsonSetting["loop_file"] = m_loopFile;
            CommonUtils::WriteSetting(jsonSetting);

            readCsvFile(fileName);
            logger->info("文件验证成功");
        } else {
            ui->lEdit_File->setStyleSheet("QLineEdit { color: red; }");
            logger->warn("文件验证成功");
            QMessageBox::warning(this, "文件验证失败", "选择的文件不是有效的csv文件或格式不正确。");
        }
    }
}

// 专门的日志槽函数 - 在UI线程中执行
void MainWindow::onLogMessage(const QString& message)
{
    // 在UI线程中生成时间戳，确保显示的时间是UI更新的时间
    ui->textEdit_Log->append(message);

    // 自动滚动到底部
    QTextCursor cursor = ui->textEdit_Log->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit_Log->setTextCursor(cursor);
}

// 验证csv文件
bool MainWindow::validateCsvFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        logger->error(QString("文件不存在: %1").arg(filePath));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        logger->error(QString("无法打开文件: %1").arg(filePath));
        return false;
    }
    return true;
}

void MainWindow::readCsvFile(const QString &filePath)
{
    dataLEDPara.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger->error(QString("文件\"%1\"打开失败").arg(filePath));
        return;
    }

    QTextStream in(&file);
    bool isFirstLine = true;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }

        QStringList fields = line.split(",");
        if (fields.size() < 2) {
            continue;  // 确保至少有两列数据
        }

        CsvDataRow row;
        row.ledIntensity = fields.at(0).toDouble();
        for (int i = 0; i < 10; ++i) {
            row.dacValues[i] = fields.at(i+1).toInt();
        }
        dataLEDPara.append(row);
    }
    file.close();
    logger->info(QString("文件\"%1\"读取成功").arg(filePath));
}

void MainWindow::slot_RebackUnable()
{
    ui->bt_startLoop->setEnabled(true);
    ui->bt_startLoop->setText("开始测量");
    UIcontrolEnable(true);
}

void MainWindow::slot_Reset_finished()
{
    logger->info("内核初始化完成");
    ui->bt_startLoop->setEnabled(true);
    UIcontrolEnable(true);
}
void MainWindow::slot_config_finished() //循环前的参数配置已经完成
{
    logger->info("参数配置完成");

    //开始真正的循环
    CsvDataRow data = tempLEDdata.first();
    logger->info(QString("开始循环,光强:%1").arg(data.ledIntensity));
    ui->progressBar->setRange(0,tempLEDdata.size());
    ui->progressBar->setValue(0);
    commManager->startOneLoop(data);
    tempLEDdata.removeFirst();
}

void MainWindow::slot_measureStoped()
{
    logger->info("测量已停止");
    ui->bt_startLoop->setEnabled(true);
    UIcontrolEnable(true);
}

/**
 * @brief MainWindow::slot_updateTemp 更新界面的温度显示
 * @param id 温度编号：1~4
 * @param temp 温度，单位：℃
 */
void MainWindow::slot_updateTemp(int id, double temp)
{
    switch (id) {
    case 1:
        ui->lineEdit_Temp1->setText(QString::number(temp,'f', 3));
        break;
    case 2:
        ui->lineEdit_Temp2->setText(QString::number(temp,'f', 3));
        break;
    case 3:
        ui->lineEdit_Temp3->setText(QString::number(temp,'f', 3));
        break;
    case 4:
        ui->lineEdit_Temp4->setText(QString::number(temp,'f', 3));
        break;
    default:
        break;
    }
}

void MainWindow::slot_finishedOneLoop()
{
    //更新进度条
    int total = ui->progressBar->maximum();
    ui->progressBar->setValue(total-tempLEDdata.size());

    if(tempLEDdata.size()>0)
    {
        CsvDataRow data = tempLEDdata.first();
        logger->info("当前循环结束");
        logger->info(QString("开始一次新的循环,光强:%1").arg(data.ledIntensity));
        commManager->startOneLoop(data);
        tempLEDdata.removeFirst();
    }
    else{
        logger->info("完成所有循环");
        ui->bt_startLoop->setText("开始循环");
        UIcontrolEnable(true);
        ui->bt_startLoop->setEnabled(true);
    }
}

void MainWindow::UIcontrolEnable(bool flag)
{
    ui->pushButtonOpen->setEnabled(flag);
    ui->bt_refreshPort->setEnabled(flag);
    ui->comboBoxPortName->setEnabled(flag);

    ui->spinBox_LEDWidth->setEnabled(flag);
    ui->spinBox_lightDelayTime->setEnabled(flag);
    ui->spinBox_TriggerDelayTime->setEnabled(flag);
    ui->spinBox_timesLED->setEnabled(flag);

    ui->checkA_all->setEnabled(flag);
    ui->checkB_all->setEnabled(flag);

    ui->checkA1->setEnabled(flag);
    ui->checkA2->setEnabled(flag);
    ui->checkA3->setEnabled(flag);
    ui->checkA4->setEnabled(flag);
    ui->checkA5->setEnabled(flag);
    ui->checkA6->setEnabled(flag);
    ui->checkA7->setEnabled(flag);
    ui->checkA8->setEnabled(flag);
    ui->checkA9->setEnabled(flag);
    ui->checkA10->setEnabled(flag);

    ui->checkB1->setEnabled(flag);
    ui->checkB2->setEnabled(flag);
    ui->checkB3->setEnabled(flag);
    ui->checkB4->setEnabled(flag);
    ui->checkB5->setEnabled(flag);
    ui->checkB6->setEnabled(flag);
    ui->checkB7->setEnabled(flag);
    ui->checkB8->setEnabled(flag);
    ui->checkB9->setEnabled(flag);
    ui->checkB10->setEnabled(flag);

    ui->doubleSpinBox_loopStart->setEnabled(flag);
    ui->doubleSpinBox_loopEnd->setEnabled(flag);

    ui->radioButton_manual->setEnabled(flag);
    ui->radioButton_auto->setEnabled(flag);

    if(ui->radioButton_manual->isChecked()){
        ui->bt_baseLineSample->setEnabled(flag);
    }
}

void MainWindow::on_pushButtonOpen_clicked()
{
    if(ui->pushButtonOpen->text() == tr("open"))
    {
        if(ui->comboBoxPortName->count() > 0)
        {
            QString portName = ui->comboBoxPortName->currentText();
            bool flag = commManager->init(portName, 115200);

            if(flag)
            {
                ui->pushButtonOpen->setText(tr("close"));
                logger->info(QString("串口%1已打开").arg(portName));
                commManager->ressetFPGA();
                ui->bt_startLoop->setEnabled(true);
                ui->bt_kernelReset->setEnabled(true);
                if(ui->radioButton_manual->isChecked())
                {
                    ui->bt_baseLineSample->setEnabled(true);
                }
            }
            else
            {
                ui->pushButtonOpen->setText(tr("open"));
                logger->error(QString("串口%1打开失败").arg(portName));
                ui->bt_startLoop->setEnabled(false);
                ui->bt_kernelReset->setEnabled(false);
                ui->bt_baseLineSample->setEnabled(false);
            }
        }
        else
        {
            QMessageBox::information(NULL,tr("information"),tr("This Computer no avaiable port"));
            qDebug()<< "This Computer no avaiable port";
        }
    }
    else
    {
        QString portName = ui->comboBoxPortName->currentText();
        commManager->close();
        logger->info(QString("串口%1已关闭").arg(portName));
        ui->pushButtonOpen->setText(tr("open"));
        ui->bt_startLoop->setEnabled(false);
        ui->bt_kernelReset->setEnabled(false);
        ui->bt_baseLineSample->setEnabled(false);
    }
}

//循环测量
void MainWindow::on_bt_startLoop_clicked()
{
    //开始循环
    if(ui->bt_startLoop->text() == "开始循环")
    {
        //禁用界面控件
        UIcontrolEnable(false);

        //获取界面FPGA相关控制参数
        CommonUtils::UI_FPGAconfig config;
        config.LEDWidth = ui->spinBox_LEDWidth->value();
        config.LightDelayTime = ui->spinBox_lightDelayTime->value();
        config.TriggerDelayTime = ui->spinBox_TriggerDelayTime->value();
        config.timesLED = ui->spinBox_timesLED->value();
        config.RegisterA = m_RegisterA;
        config.RegisterB = m_RegisterB;

        double intensityLeft = ui->doubleSpinBox_loopStart->value();
        double intensityRight = ui->doubleSpinBox_loopEnd->value();

        tempLEDdata.clear();
        for(auto data:dataLEDPara)
        {
            if(data.ledIntensity>=intensityLeft && data.ledIntensity <= intensityRight)
            {
                tempLEDdata.push_back(data);
            }
        }
        if(tempLEDdata.size()==0)
        {
            logger->fatal("设置的光强区间错误，请核对后重新开始循环");
            UIcontrolEnable(true);
            return;
        }
        logger->info(QString("循环的光强区间：%1~%2").arg(intensityLeft).arg(intensityRight));
        commManager->setConfigBeforeLoop(config, m_BLmode);
        ui->bt_kernelReset->setEnabled(false);
        ui->bt_startLoop->setText("停止循环");
    }
    else{ //停止测量
        commManager->stopMeasure();
        UIcontrolEnable(true);
        ui->bt_startLoop->setEnabled(false);
        ui->bt_kernelReset->setEnabled(true);
        ui->bt_startLoop->setText("开始循环");
    }
}

// 处理勾选框状态变化，更新m_RegisterA的对应位
void MainWindow::onCheckStateChangedA(int state) {
    // 找到触发信号的勾选框
    QCheckBox *senderCheck = qobject_cast<QCheckBox*>(sender());
    if (!senderCheck) return;

    // 获取该勾选框在m_checks中的索引（0~9，对应check1~check10）
    int index = m_checksA.indexOf(senderCheck);
    if (index < 0 || index >= 10) return; // 索引无效

    // 根据映射表获取对应的位号（例如index=0 → m_bitMap[0]=8 → 第8位）
    int bitPos = m_bitMap[index];

    // 判断勾选状态（Qt::Checked=2 → 置1；Qt::Unchecked=0 → 置0）
    bool isChecked = (state == Qt::Checked);

    if (isChecked) {
        // 置位：m_value |= (1 << 位号)
        m_RegisterA |= (1 << bitPos);

        // 调试输出（二进制显示16位，方便观察位变化）
        logger->info(QString("勾选A组/编号%1,当前值：0x%2,二进制：%3").arg(index)\
                        .arg(QString::number(m_RegisterA, 16).toUpper())\
                        .arg(QString::number(m_RegisterA, 2).rightJustified(16, '0')));
    } else {
        // 清位：m_value &= ~(1 << 位号)
        m_RegisterA &= ~(1 << bitPos);
        logger->info(QString("取消勾选A组/编号%1,当前值：0x%2,二进制：%3").arg(index)\
                        .arg(QString::number(m_RegisterA, 16).toUpper())\
                        .arg(QString::number(m_RegisterA, 2).rightJustified(16, '0')));
    }

    // 新增逻辑：检查所有子选项是否全选，更新checkA_all状态
    bool allChecked = true;
    foreach (QCheckBox *check, m_checksA) {
        if (!check->isChecked()) {
            allChecked = false;
            break;
        }
    }

    // 避免信号递归触发，先断开连接再更新状态
    disconnect(ui->checkA_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllAChanged);
    ui->checkA_all->setChecked(allChecked);
    connect(ui->checkA_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllAChanged);
}

// 处理勾选框状态变化，更新m_RegisterB的对应位
void MainWindow::onCheckStateChangedB(int state) {
    // 找到触发信号的勾选框
    QCheckBox *senderCheck = qobject_cast<QCheckBox*>(sender());
    if (!senderCheck) return;

    // 获取该勾选框在m_checks中的索引（0~9，对应check1~check10）
    int index = m_checksB.indexOf(senderCheck);
    if (index < 0 || index >= 10) return; // 索引无效

    // 根据映射表获取对应的位号（例如index=0 → m_bitMap[0]=8 → 第8位）
    int bitPos = m_bitMap[index];

    // 判断勾选状态（Qt::Checked=2 → 置1；Qt::Unchecked=0 → 置0）
    bool isChecked = (state == Qt::Checked);

    if (isChecked) {
        // 置位：m_value |= (1 << 位号)
        m_RegisterB |= (1 << bitPos);
    } else {
        // 清位：m_value &= ~(1 << 位号)
        m_RegisterB &= ~(1 << bitPos);
    }

    // 新增逻辑：检查所有子选项是否全选，更新checkA_all状态
    bool allChecked = true;
    foreach (QCheckBox *check, m_checksB) {
        if (!check->isChecked()) {
            allChecked = false;
            break;
        }
    }

    //打印更新日志
    if (state == Qt::Checked) {
        logger->info(QString("勾选B组/编号%1,当前值：0x%2,二进制：%3").arg(index)\
                        .arg(QString::number(m_RegisterB, 16).toUpper())\
                        .arg(QString::number(m_RegisterB, 2).rightJustified(16, '0')));
    }
    else
    {
        logger->info(QString("取消勾选B组/编号%1,当前值：0x%2,二进制：%3").arg(index)\
                        .arg(QString::number(m_RegisterB, 16).toUpper())\
                        .arg(QString::number(m_RegisterB, 2).rightJustified(16, '0')));
    }
    // 避免信号递归触发，先断开连接再更新状态
    disconnect(ui->checkB_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllBChanged);
    ui->checkB_all->setChecked(allChecked);
    connect(ui->checkB_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllBChanged);
}

void MainWindow::onCheckAllAChanged(int state)
{
    // 断开checkA1~checkA10的信号连接，避免触发onCheckStateChangedA
    foreach (QCheckBox *check, m_checksA) {
        disconnect(check, &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedA);
    }

    if (state == Qt::Checked) {
        // 全选：勾选所有checkA1~checkA10
        foreach (QCheckBox *check, m_checksA) {
            check->setChecked(true);
        }
    } else {
        // 取消全选：取消所有checkA1~checkA10的勾选
        foreach (QCheckBox *check, m_checksA) {
            check->setChecked(false);
        }
    }

    // 重新连接信号
    foreach (QCheckBox *check, m_checksA) {
        connect(check, &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedA);
    }

    // 手动更新m_RegisterA的值（因为断开连接期间状态变化未触发onCheckStateChangedA）
    m_RegisterA = 0;
    if (state == Qt::Checked) {
        foreach (int i, m_bitMap) {
            m_RegisterA |= (1 << i);
        }
    }

    //打印更新日志
    if (state == Qt::Checked) {
        logger->info(QString("勾选A组所有编号,当前值：0x%1,二进制：%2")\
                         .arg(QString::number(m_RegisterA, 16).toUpper())\
                         .arg(QString::number(m_RegisterA, 2).rightJustified(16, '0')));
    }
    else
    {
        logger->info(QString("取消勾选A组所有编号,当前值：0x%1,二进制：%2")\
                         .arg(QString::number(m_RegisterA, 16).toUpper())\
                         .arg(QString::number(m_RegisterA, 2).rightJustified(16, '0')));
    }
    // 处理checkA1~checkA10单独勾选时对checkA_all的影响（当所有子项都勾选时，checkA_all应自动勾选）
    // 检查所有checkA1~checkA10是否都被勾选
    bool allChecked = true;
    foreach (QCheckBox *check, m_checksA) {
        if (!check->isChecked()) {
            allChecked = false;
            break;
        }
    }

    // 断开checkA_all的信号连接，避免递归触发
    disconnect(ui->checkA_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllAChanged);
    ui->checkA_all->setChecked(allChecked);
    connect(ui->checkA_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllAChanged);
}

void MainWindow::onCheckAllBChanged(int state)
{
    // 断开checkB1~checkB10的信号连接，避免触发onCheckStateChangedB
    foreach (QCheckBox *check, m_checksB) {
        disconnect(check, &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedB);
    }

    if (state == Qt::Checked) {
        // 全选：勾选所有checkB1~checkB10
        foreach (QCheckBox *check, m_checksB) {
            check->setChecked(true);
            logger->info(QString("勾选B组所有编号,当前值：0x%1,二进制：%2")\
                        .arg(QString::number(m_RegisterB, 16).toUpper())\
                        .arg(QString::number(m_RegisterB, 2).rightJustified(16, '0')));
        }
    } else {
        // 取消全选：取消所有checkB1~checkB10的勾选
        foreach (QCheckBox *check, m_checksB) {
            check->setChecked(false);
            logger->info(QString("取消勾选B组所有编号,当前值：0x%1,二进制：%2")\
                        .arg(QString::number(m_RegisterB, 16).toUpper())\
                        .arg(QString::number(m_RegisterB, 2).rightJustified(16, '0')));
        }
    }

    // 重新连接信号
    foreach (QCheckBox *check, m_checksB) {
        connect(check, &QCheckBox::stateChanged, this, &MainWindow::onCheckStateChangedB);
    }

    // 手动更新m_RegisterB的值（因为断开连接期间状态变化未触发onCheckStateChangedB）
    m_RegisterB = 0;
    if (state == Qt::Checked) {
        foreach (int i, m_bitMap) {
            m_RegisterB |= (1 << i);
        }
    }

    // 处理checkB1~checkB10单独勾选时对checkB_all的影响（当所有子项都勾选时，checkB_all应自动勾选）
    // 检查所有checkB1~checkB10是否都被勾选
    bool allChecked = true;
    foreach (QCheckBox *check, m_checksB) {
        if (!check->isChecked()) {
            allChecked = false;
            break;
        }
    }

    // 断开checkB_all的信号连接，避免递归触发
    disconnect(ui->checkB_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllBChanged);
    ui->checkB_all->setChecked(allChecked);
    connect(ui->checkB_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllBChanged);
}

// 处理单选按钮选中状态变化（checked为true表示当前按钮被选中）
void MainWindow::onBLmodeToggled(bool checked)
{
    if (checked) { // 只处理“被选中”的情况（避免重复触发）
        QRadioButton *senderRadio = qobject_cast<QRadioButton*>(sender());
        if (senderRadio == ui->radioButton_auto) {
            logger->info("设置基线采集模式：自动");
            ui->bt_baseLineSample->setEnabled(false);
            m_BLmode = AutoBL;
        } else if (senderRadio == ui->radioButton_manual) {
            logger->info("设置基线采集模式：手动");
            ui->bt_baseLineSample->setEnabled(true);
            m_BLmode = ManualBL;
        }
    }
}

//内核初始化，用于对FPGA进行一系列关闭指令。
void MainWindow::on_bt_kernelReset_clicked()
{
    commManager->ressetFPGA();
    UIcontrolEnable(false); //禁用控件
    ui->bt_startLoop->setEnabled(false);
}


void MainWindow::on_bt_baseLineSample_clicked()
{
    commManager->baseLineSample_manual();
}


void MainWindow::on_bt_refreshPort_clicked()
{
    refreshSerialPort();
}


void MainWindow::on_BaglosttestButton_clicked()
{
    // 使用文件对话框让用户选择文件
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择日志文件",
        "C:/Users/25368/Desktop",  // 默认指向桌面
        "日志文件 (*.log *.txt);;所有文件 (*.*)"
        );

    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "未选择文件！");
        return;
    }

    // 转换QString为std::string
    std::string filename = filePath.toLocal8Bit().constData();

    // 解析文件
    if (parser.parseLogFile(filename)) {
        // 计算丢包率
        double lossRate = parser.calculatePacketLossRate() * 100;

        // 构建完整的显示内容
        QString result;
        result += "=== 日志分析结果 ===\n\n";
        result += QString("总数据组数: %1\n").arg(parser.getTotalPairs());
        result += QString("丢包次数: %1\n").arg(parser.getLostPackets());
        result += QString("丢包率: %1%\n\n").arg(lossRate, 0, 'f', 2);
        result += "=== 详细数据 ===\n\n";

        // 添加所有数据对的详细信息
        const auto& pairs = parser.getHexPairs();
        for (size_t i = 0; i < pairs.size(); ++i) {
            const auto& pair = pairs[i];

            result += QString("组 %1").arg(i + 1);

            // 添加时间戳（如果不为空）
            if (!pair.timestamp.empty()) {
                result += QString(" [%1]").arg(QString::fromStdString(pair.timestamp));
            }

            // 添加匹配状态
            result += QString(": %1\n").arg(pair.isMatched ? "[OK] 匹配" : "[ERR] 不匹配");

            // 添加发送和接收数据
            result += QString("  发送: %1\n").arg(QString::fromStdString(pair.sendHex));
            result += QString("  接收: %1\n").arg(QString::fromStdString(pair.recvHex));

            // 如果不匹配，添加警告
            if (!pair.isMatched) {
                result += "  [警告] 数据不匹配！\n";
            }

            result += "\n"; // 组间空行
        }

        // 创建消息框显示完整结果
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("丢包率分析结果");
        msgBox.setText("分析完成！");
        msgBox.setDetailedText(result);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();

    } else {
        QMessageBox::critical(this, "错误", "文件解析失败！请检查文件路径和格式。");
    }
}

