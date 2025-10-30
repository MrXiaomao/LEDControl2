#include "common.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

QString CommonUtils::jsonPath = "./config/setting.json";

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
        "HLpoint"
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
    config.PowerStableTime = user.value("PowerStableTime").toInt();
    config.TempMonitorGap  = user.value("TempMonitorGap").toInt();
    config.TriggerWidth    = user.value("TriggerWidth").toInt();
    config.clockFrequency  = user.value("clockFrequency").toInt();
    config.HLpoint         = user.value("HLpoint").toInt();

    qInfo() << "✅ User 配置加载成功:";
    qInfo() << "   PowerStableTime =" << config.PowerStableTime;
    qInfo() << "   TempMonitorGap  =" << config.TempMonitorGap;
    qInfo() << "   TriggerWidth    =" << config.TriggerWidth;
    qInfo() << "   clockFrequency  =" << config.clockFrequency;
    qInfo() << "   HLpoint         =" << config.HLpoint;

    return config;
}
