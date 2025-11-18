#ifndef LOGPARSER_H
#define LOGPARSER_H

#include <QString>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>

using namespace std;

//-----------------
// HexPair结构体定义
struct HexPair
{
    std::string timestamp;    // 时间戳
    std::string sendHex;      // 发送的十六进制数据
    std::string recvHex;      // 接收的十六进制数据
    bool isMatched;           // 发送和接收是否匹配
    bool hasResponse;         // 是否有回复
};

// LogParser类定义
class LogParser
{
private:
    std::vector<HexPair> hexPairs;     // 存储所有数据对的向量
    int totalPairs;                    // 总数据组数
    int lostPackets;                   // 丢包次数（包含不匹配和无回复）
    int noResponsePackets;             // 无回复包计数

    // 私有工具函数
    std::string removeSpacesAndLower(const std::string& str);
    std::string trim(const std::string& str);
    void addDataPacket(const std::string& timestamp, const std::string& sendHex, const std::string& recvHex);
    void addNoResponsePacket(const std::string& timestamp, const std::string& sendHex);

public:
    // 构造函数
    LogParser();

    // 解析日志文件
    bool parseLogFile(const std::string& filename);

    // 计算各种比率
    double calculatePacketLossRate() const;
    double calculateNoResponseRate() const;
    double calculateMismatchRate() const;

    // 获取统计信息
    int getTotalPairs() const;
    int getLostPackets() const;
    int getNoResponsePackets() const;
    const std::vector<HexPair>& getHexPairs() const;
};

#endif // LOGPARSER_H
