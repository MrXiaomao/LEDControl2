#ifndef COMMON_H
#define COMMON_H

#include <QVector>
#include <QByteArray>

// 声明常用结构体
struct CsvDataRow {
    double ledIntensity;       // 第一列：光强（double）
    QVector<int> dacValues;    // 其他列：DAC值（int）
};

// 声明常用函数（工具函数）
// namespace CommonUtils {
// // 字符串转十六进制QByteArray
// QByteArray stringToHex(const QString &str);

// // 检查两个QByteArray是否完全相同
// bool isByteArrayEqual(const QByteArray &a, const QByteArray &b);

// // 从文件路径读取CSV数据（返回CsvDataRow结构体列表）
// QVector<CsvDataRow> readCsvFromFile(const QString &filePath);
// }

#endif // COMMON_H
