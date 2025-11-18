#include "common.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

QString CommonUtils::jsonPath = "./config/setting.json";
//上传新内容

// ============ 基础函数 =============
QJsonObject CommonUtils::ReadSetting()
{
    QFile file(jsonPath);
    if (!file.open(QFile::ReadOnly)) {
        qCritical() << "无法打开配置文件:" << jsonPath;
        throw std::runtime_error("配置文件打开失败");
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qCritical() << "JSON 解析错误:" << err.errorString();
        throw std::runtime_error("配置文件格式错误");
    }

    return doc.object();
}

void CommonUtils::WriteSetting(QJsonObject myJson)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法写入配置文件:" << jsonPath;
        return;
    }

    QJsonDocument doc(myJson);
    file.write(doc.toJson());
    file.close();
}

// ============ 校验与读取 User 配置 =============
CommonUtils::UserConfig CommonUtils::loadUserConfig()
{
    QJsonObject root = ReadSetting();
    QJsonObject user = root.value("User").toObject();

    QStringList requiredKeys = {
        "PowerStableTime",
        "TempMonitorGap",
        "TriggerWidth",
        "clockFrequency",
        "HLpoint",
        "timesTrigger"
    };

    // 校验所有关键字段
    for (const QString &key : requiredKeys) {
        if (!user.contains(key)) {
            qCritical() << "配置文件缺少关键字段:" << key;
            throw std::runtime_error(("配置文件缺少字段: " + key).toStdString());
        }
    }

    // 全部存在，安全读取
    UserConfig config;
    config.PowerStableTime = user.value("PowerStableTime").toInt(10);
    config.TempMonitorGap  = static_cast<unsigned short>(user.value("TempMonitorGap").toInt(1));
    config.TriggerWidth    = user.value("TriggerWidth").toInt(1000);
    config.clockFrequency  = user.value("clockFrequency").toInt(10);
    config.HLpoint         = user.value("HLpoint").toInt(1);
    config.timesTrigger    = user.value("timesTrigger").toInt(1000);

    qDebug() << "  User 配置加载成功:";
    qDebug() << "   PowerStableTime =" << config.PowerStableTime;
    qDebug() << "   TempMonitorGap  =" << config.TempMonitorGap;
    qDebug() << "   TriggerWidth    =" << config.TriggerWidth;
    qDebug() << "   clockFrequency  =" << config.clockFrequency;
    qDebug() << "   HLpoint         =" << config.HLpoint;
    qDebug() << "   timesTrigger    =" << config.timesTrigger;

    return config;
}

unsigned short CommonUtils::loadJson_heckValueA()
{
    unsigned short value = 1;
    QJsonObject json = CommonUtils::ReadSetting();
    QJsonObject uiJson = json.value("UI").toObject();

    if (uiJson.isEmpty()) {
        qCritical()<<"Setin.jon配置文件中缺少 UI 节点";
        return value;
    }
    int checkValue = uiJson.value("checkValueA").toInt(1);
    value = static_cast<unsigned short>(checkValue);
    return value;
}

unsigned short CommonUtils::loadJson_heckValueB()
{
    unsigned short value = 1;
    QJsonObject json = CommonUtils::ReadSetting();
    QJsonObject uiJson = json.value("UI").toObject();

    if (uiJson.isEmpty()) {
        qCritical()<<"Setin.jon配置文件中缺少 UI 节点";
        return value;
    }
    int checkValue = uiJson.value("checkValueB").toInt(1);
    value = static_cast<unsigned short>(checkValue);
    return value;
}
