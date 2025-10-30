#include "mainwindow.h"
#include "ui_mainwindow.h"

// #include <QFile>
#include <QJsonDocument>
#include <Qjsonarray>

#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QDebug>
#include "order.h"

QString MainWindow::jsonPath = "./config/setting.json";

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

    //init
    std::vector<SerialPortInfo> portNameList = CSerialPortInfo::availablePortInfos();

    for (size_t i = 0; i < portNameList.size(); i++)
    {
        ui->comboBoxPortName->insertItem(i,QString::fromLocal8Bit(portNameList[i].portName));
    }

    //ui
    ui->pushButtonOpen->setText(tr("open"));

    if (portNameList.size() > 0)
    {
        QString message = QString("First avaiable Port: %1").arg(portNameList[0].portName);
        QString currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        QString consoleEntry = QString("[%1] %2: %3").arg(currentTimestamp, "info", message);

        ui->textEdit_Log->moveCursor (QTextCursor::End);
        ui->textEdit_Log->insertPlainText(consoleEntry);
    }
    else
    {
        ui->textEdit_Log->moveCursor (QTextCursor::End);
        ui->textEdit_Log->insertPlainText("No avaiable Port");
        return;
    }

    ui->bt_startLoop->setEnabled(false);
    // 连接日志信号槽
    connect(this, &MainWindow::sigLogMessage, this, &MainWindow::onLogMessage, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigUpdateReceive, this, &MainWindow::OnUpdateReceive, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigFPGA_prepared, this, &MainWindow::slot_FPGA_prepared, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigFPGA_stoped, this, &MainWindow::slot_measureStoped, Qt::QueuedConnection);
    connect(commManager, &CommandHelper::sigFinishCurrentloop, this, &MainWindow::slot_finishedOneLoop, Qt::QueuedConnection);


    // 2. （可选）关联选中状态变化的信号（两个单选按钮可共用一个槽函数）
    connect(ui->radioButton_auto, &QRadioButton::toggled,this, &MainWindow::onBLmodeToggled);
    connect(ui->radioButton_manual, &QRadioButton::toggled,this, &MainWindow::onBLmodeToggled);

    connect(ui->checkA_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllAChanged);
    connect(ui->checkB_all, &QCheckBox::stateChanged, this, &MainWindow::onCheckAllBChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OnUpdateReceive(QString str)
{
    logInfo(QString("Rec(Hex):%1").arg(str));
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
        logInfo(QString("选择文件: %1").arg(fileName));

        // 验证文件
        if (validateCsvFile(fileName)) {
            ui->lEdit_File->setStyleSheet("QLineEdit { color: green; }");

            // 保存到配置文件
            QJsonObject jsonSetting = ReadSetting();
            jsonSetting["loop_file"] = m_loopFile;
            WriteSetting(jsonSetting);

            readCsvFile(fileName);
            logDebug("文件验证成功");
        } else {
            ui->lEdit_File->setStyleSheet("QLineEdit { color: red; }");
            logWarning("文件验证失败");
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

// 修改日志输出方法
void MainWindow::logMessage(const QString& message, const QString& type)
{
    // 立即在调用线程中输出到控制台（时间准确）
    QString currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString consoleEntry = QString("[%1] %2: %3").arg(currentTimestamp, type, message);
    qDebug().noquote() << consoleEntry;

    // 发送信号到主线程更新UI，时间戳在槽函数中生成
    if(type!="DEBUG")
    {
        emit sigLogMessage(consoleEntry);
    }
}

// 各种级别的日志方法
void MainWindow::logDebug(const QString& message)
{
    logMessage(message, "DEBUG");
}

void MainWindow::logInfo(const QString& message)
{
    logMessage(message, "INFO");
}

void MainWindow::logWarning(const QString& message)
{
    logMessage(message, "WARNING");
}

void MainWindow::logError(const QString& message)
{
    logMessage(message, "ERROR");
}

// 验证csv文件
bool MainWindow::validateCsvFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        logError(QString("文件不存在: %1").arg(filePath));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("无法打开文件: %1").arg(filePath));
        return false;
    }
    return true;
}

void MainWindow::readCsvFile(const QString &filePath)
{
    dataLEDPara.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logError(QString("文件%1打开失败").arg(filePath));
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
    logInfo(QString("文件%1读取成功").arg(filePath));
}

// 读取配置文件
QJsonObject MainWindow::ReadSetting()
{
    // 读取文件
    QFile file(jsonPath);
    file.open(QFile::ReadOnly);
    QByteArray all = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(all);//转换成文档对象
    QJsonObject obj;
    if (doc.isObject())//可以不做格式判断，因为，解析的时候已经知道是什么数据了
    {
        obj = doc.object(); //得到Json对象
    }
    return obj;
}

// 写入配置文件，实际上是修改配置文件
void MainWindow::WriteSetting(QJsonObject myJson)
{
    //创建QJsonDocument对象并将根对象传入
    QJsonDocument jDoc(myJson);
    //打开存放json串的文件
    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) return ;

    //使用QJsonDocument的toJson方法获取json串并保存到数组
    QByteArray data(jDoc.toJson());

    //将json串写入文件
    file.write(data);
    file.close();
}

void MainWindow::slot_FPGA_prepared()
{
    logInfo("内核初始化完成");
    ui->bt_startLoop->setEnabled(true);
    UIcontrolEnable(true);
}

void MainWindow::slot_measureStoped()
{
    logInfo("测量已停止");
    ui->bt_startLoop->setEnabled(true);
    UIcontrolEnable(true);
}

void MainWindow::slot_finishedOneLoop()
{
    if(tempLEDdata.size())
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
                logInfo(QString("串口%1已打开").arg(portName));
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
                logError(QString("串口%1打开失败").arg(portName));
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
        logInfo(QString("串口%1已关闭").arg(portName));
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
    {
        //禁用界面控件
        UIcontrolEnable(false);

        //发光宽度,单位ns
        int LED_width = ui->spinBox_LEDWidth->value();
        //LED发光延迟时间,单位ns
        int delayTime = ui->spinBox_lightDelayTime->value();
        //发光次数
        int times = ui->spinBox_timesLED->value();

        double intensityLeft = ui->doubleSpinBox_loopStart->value();
        double intensityRight = ui->doubleSpinBox_loopEnd->value();

        tempLEDdata.clear();
        for(auto data:dataLEDPara)
        {
            if(data.LED_intensity>=intensityLeft && data.LED_intensity <= intensityRight)
            {
                tempLEDdata.push_back(data);
            }
        }
        if(tempLEDdata.size()==0)
        {
            logError("设置的光强区间错误，请核对后重新开始循环");
            return;
        }

        commManager->
        tempLEDdata.removeFirst();
    }

    //停止测量
    {
        commManager->stopMeasure();
        ui->bt_startLoop->setEnabled(false);
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
            logInfo("基线采集模式：自动");
            ui->bt_baseLineSample->setEnabled(false);
        } else if (senderRadio == ui->radioButton_manual) {
            logInfo("基线采集模式：手动");
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
    commManager->baseLineSample();
}

