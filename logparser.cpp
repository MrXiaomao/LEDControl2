#include "logparser.h"
#include <fstream>
#include <regex>
#include <algorithm>


// LogParser类的实现
LogParser::LogParser() : totalPairs(0), lostPackets(0) {}

bool LogParser::parseLogFile(const std::string& filename)
{
    // 重置数据
    hexPairs.clear();
    totalPairs = 0;
    lostPackets = 0;
    noResponsePackets = 0;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;               // 存储每行日志内容
    std::string currentSendHex;     // 当前发送的十六进制数据
    std::string currentTime;        // 当前时间戳
    std::string sendTime;           // 发送时的时间戳
    bool expectRecv = false;        // 是否期望接收行
    bool foundSendTag = false;      // 是否找到发送标签
    bool foundRecvTag = false;      // 是否找到接收标签

    // 支持两种格式的正则表达式
    // 时间戳模式 - 支持两种格式
    std::regex timePattern1(R"(^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}))");  // 格式1
    std::regex timePattern2(R"(^\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\])"); // 格式2

    // 发送行模式
    std::regex sendPattern1(R"(Send HEX: ([0-9a-fA-F ]+))");      // 格式1
    std::regex sendPattern2(R"(# SEND HEX>)", std::regex::icase); // 格式2标签（忽略大小写）

    // 接收行模式
    std::regex recvPattern1(R"(Recv HEX: ([0-9a-fA-F ]+))");      // 格式1
    std::regex recvPattern2(R"(# RECV HEX>)", std::regex::icase); // 格式2标签（忽略大小写）

    // 数据行模式（用于格式2）
    std::regex dataPattern(R"(^([0-9a-fA-F ]+)$)");               // 纯十六进制数据行

    std::smatch matches;

    // 逐行读取日志文件
    while (std::getline(file, line)) {
        // 去除行首行尾空白字符
        line = trim(line);
        if (line.empty()) continue;

        // 尝试匹配两种格式的时间戳
        if (std::regex_search(line, matches, timePattern1) && matches.size() > 1) {
            currentTime = matches[1].str();  // 格式1的时间戳
        }
        else if (std::regex_search(line, matches, timePattern2) && matches.size() > 1) {
            currentTime = matches[1].str();  // 格式2的时间戳（去掉方括号）
        }

        // 格式1：在同一行中找到完整的数据
        if (std::regex_search(line, matches, sendPattern1) && matches.size() > 1) {
            // 处理之前未配对的发送
            if (expectRecv) {
                addNoResponsePacket(sendTime, currentSendHex);
            }

            currentSendHex = matches[1].str();
            currentSendHex = removeSpacesAndLower(currentSendHex);
            sendTime = currentTime;
            expectRecv = true;
            foundSendTag = false;  // 重置标志
        }
        // 格式2：找到发送标签
        else if (std::regex_search(line, matches, sendPattern2)) {
            foundSendTag = true;
            sendTime = currentTime;
            // 等待下一行的数据
        }
        // 格式2：找到接收标签
        else if (std::regex_search(line, matches, recvPattern2)) {
            foundRecvTag = true;
            // 等待下一行的数据
        }
        // 格式1：在同一行中找到接收数据
        else if (expectRecv && std::regex_search(line, matches, recvPattern1) && matches.size() > 1) {
            std::string recvHex = matches[1].str();
            recvHex = removeSpacesAndLower(recvHex);
            addDataPacket(sendTime, currentSendHex, recvHex);
            expectRecv = false;
            currentSendHex.clear();
        }
        // 格式2：处理数据行（可能是发送或接收的数据）
        else if (std::regex_search(line, matches, dataPattern) && matches.size() > 1) {
            std::string hexData = matches[1].str();
            hexData = removeSpacesAndLower(hexData);

            if (foundSendTag) {
                // 处理之前未配对的发送
                if (expectRecv) {
                    addNoResponsePacket(sendTime, currentSendHex);
                }

                currentSendHex = hexData;
                sendTime = currentTime;
                expectRecv = true;
                foundSendTag = false;
            }
            else if (foundRecvTag && expectRecv) {
                // 找到期待的接收数据
                addDataPacket(sendTime, currentSendHex, hexData);
                expectRecv = false;
                currentSendHex.clear();
                foundRecvTag = false;
            }
            else if (foundRecvTag) {
                // 意外的接收数据（没有对应的发送）
                foundRecvTag = false;
            }
        }
    }

    // 处理文件末尾可能存在的未配对发送
    if (expectRecv) {
        addNoResponsePacket(sendTime, currentSendHex);
    }

    file.close();
    return true;
}

// 添加数据包（有回复）
void LogParser::addDataPacket(const std::string& timestamp, const std::string& sendHex, const std::string& recvHex) {
    HexPair pair;
    pair.timestamp = timestamp;
    pair.sendHex = sendHex;
    pair.recvHex = recvHex;
    pair.isMatched = (sendHex == recvHex);
    pair.hasResponse = true;

    hexPairs.push_back(pair);
    totalPairs++;

    if (!pair.isMatched) {
        lostPackets++;
    }
}

// 添加无回复包
void LogParser::addNoResponsePacket(const std::string& timestamp, const std::string& sendHex) {
    HexPair pair;
    pair.timestamp = timestamp;
    pair.sendHex = sendHex;
    pair.recvHex = "[无回复]";
    pair.isMatched = false;
    pair.hasResponse = false;

    hexPairs.push_back(pair);
    totalPairs++;
    lostPackets++;
    noResponsePackets++;
}

// 计算总丢包率（包含数据不匹配和无回复）
double LogParser::calculatePacketLossRate() const {
    if (totalPairs == 0) return 0.0;
    return static_cast<double>(lostPackets) / totalPairs;
}

// 计算无回复率
double LogParser::calculateNoResponseRate() const {
    if (totalPairs == 0) return 0.0;
    return static_cast<double>(noResponsePackets) / totalPairs;
}

// 计算数据不匹配率
double LogParser::calculateMismatchRate() const {
    if (totalPairs == 0) return 0.0;
    return static_cast<double>(lostPackets - noResponsePackets) / totalPairs;
}

// 获取总组数
int LogParser::getTotalPairs() const {
    return totalPairs;
}

// 获取丢包次数
int LogParser::getLostPackets() const {
    return lostPackets;
}

// 获取无回复包次数
int LogParser::getNoResponsePackets() const {
    return noResponsePackets;
}

// 获取所有数据对
const std::vector<HexPair>& LogParser::getHexPairs() const {
    return hexPairs;
}

// 工具函数：移除字符串中的空格并转换为小写
std::string LogParser::removeSpacesAndLower(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c != ' ') {
            result += std::tolower(c);  // 转换为小写
        }
    }
    return result;
}

// 去除字符串首尾空白
std::string LogParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}
