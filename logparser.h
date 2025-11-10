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
    std::string timestamp;
    std::string sendHex;
    std::string recvHex;
    bool isMatched;
};

// LogParser类定义
class LogParser
{
private:
    std::vector<HexPair> hexPairs;
    int totalPairs;
    int lostPackets;
    std::string removeSpacesAndLower(const std::string& str);

public:
    LogParser();
    bool parseLogFile(const std::string& filename);
    double calculatePacketLossRate() const;
    int getTotalPairs() const;
    int getLostPackets() const;
    const std::vector<HexPair>& getHexPairs() const;
};

#endif // LOGPARSER_H
