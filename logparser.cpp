#include "logparser.h"


// LogParser类的实现
LogParser::LogParser() : totalPairs(0), lostPackets(0) {}

bool LogParser::parseLogFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string currentSendHex;
    std::string currentTime;
    bool expectRecv = false;

    std::regex timePattern(R"(^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}))");
    std::regex sendPattern(R"(Send HEX: ([0-9a-fA-F ]+))");
    std::regex recvPattern(R"(Recv HEX: ([0-9a-fA-F ]+))");
    std::smatch matches;

    while (std::getline(file, line)) {
        if (std::regex_search(line, matches, timePattern) && matches.size() > 1) {
            currentTime = matches[1].str();
        }

        if (std::regex_search(line, matches, sendPattern) && matches.size() > 1) {
            currentSendHex = matches[1].str();
            currentSendHex = removeSpacesAndLower(currentSendHex);
            expectRecv = true;
        }
        else if (expectRecv && std::regex_search(line, matches, recvPattern) && matches.size() > 1) {
            std::string recvHex = matches[1].str();
            recvHex = removeSpacesAndLower(recvHex);

            HexPair pair;
            pair.timestamp = currentTime;
            pair.sendHex = currentSendHex;
            pair.recvHex = recvHex;
            pair.isMatched = (currentSendHex == recvHex);

            hexPairs.push_back(pair);
            totalPairs++;

            if (!pair.isMatched) {
                lostPackets++;
            }

            expectRecv = false;
            currentSendHex.clear();
        }
    }

    file.close();
    return true;
}

double LogParser::calculatePacketLossRate() const {
    if (totalPairs == 0) return 0.0;
    return static_cast<double>(lostPackets) / totalPairs;
}

int LogParser::getTotalPairs() const {
    return totalPairs;
}

int LogParser::getLostPackets() const {
    return lostPackets;
}

const std::vector<HexPair>& LogParser::getHexPairs() const {
    return hexPairs;
}

std::string LogParser::removeSpacesAndLower(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c != ' ') {
            result += std::tolower(c);
        }
    }
    return result;
}
