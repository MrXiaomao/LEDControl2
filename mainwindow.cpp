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

    //初始化串口
    refreshSerialPort();

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
}

MainWindow::~MainWindow()
{
    //退出时关闭串口
    commManager->close();

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
        QString currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        QString consoleEntry = QString("[%1] %2: %3").arg(currentTimestamp, "info", message);

        //考虑到界面初始化调用此函数的时候，无法打印日志到界面
        ui->textEdit_Log->moveCursor (QTextCursor::End);
        ui->textEdit_Log->insertPlainText(consoleEntry);
    }
    else
    {
        ui->textEdit_Log->moveCursor (QTextCursor::End);
        ui->textEdit_Log->insertPlainText("No avaiable Port");
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
    QString confPath = QCoreApplication::applicationDirPath() + "/log4qt.conf";
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
    layout->setConversionPattern("%d{yyyy-MM-dd HH:mm:ss} [%p] %m");//不换行，如果换行：("%d{yyyy-MM-dd HH:mm:ss} [%p] %m%n");
    layout->activateOptions();

    uiAppender->setLayout(layout);
    uiAppender->activateOptions();
    logger->addAppender(uiAppender);

    // === 4️⃣ 连接信号，用于更新界面日志 ===
    connect(uiAppender, &UiLogAppender::logToUi, this, &MainWindow::onLogMessage, Qt::QueuedConnection);

    logger->info("程序启动");
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
        UIcontrolEnable(true);
        ui->bt_startLoop->setEnabled(true);
    }
}

void MainWindow::UIcontrolEnable(bool flag)
{
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
        bool isAutoMode = ui->radioButton_auto->isChecked();
        commManager->setConfigBeforeLoop(config, isAutoMode);
    }
    else{ //停止测量
        commManager->stopMeasure();
        ui->bt_startLoop->setEnabled(false);
        ui->bt_startLoop->setText("停止循环");
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
    } else {
        // 清位：m_value &= ~(1 << 位号)
        m_RegisterA &= ~(1 << bitPos);
    }

    // 调试输出（二进制显示16位，方便观察位变化）
    qDebug() << "当前值：0x" << QString::number(m_RegisterA, 16).toUpper()
             << "二进制：" << QString::number(m_RegisterA, 2).rightJustified(16, '0');

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

    // 调试输出（二进制显示16位，方便观察位变化）
    qDebug() << "当前值：0x" << QString::number(m_RegisterB, 16).toUpper()
             << "二进制：" << QString::number(m_RegisterB, 2).rightJustified(16, '0');

    // 新增逻辑：检查所有子选项是否全选，更新checkA_all状态
    bool allChecked = true;
    foreach (QCheckBox *check, m_checksB) {
        if (!check->isChecked()) {
            allChecked = false;
            break;
        }
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
        }
    } else {
        // 取消全选：取消所有checkB1~checkB10的勾选
        foreach (QCheckBox *check, m_checksB) {
            check->setChecked(false);
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
        } else if (senderRadio == ui->radioButton_manual) {
            logger->info("设置基线采集模式：手动");
            ui->bt_baseLineSample->setEnabled(true);
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

