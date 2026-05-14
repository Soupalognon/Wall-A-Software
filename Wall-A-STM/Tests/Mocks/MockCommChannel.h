#ifndef TESTS_MOCKS_MOCKCOMMCHANNEL_H
#define TESTS_MOCKS_MOCKCOMMCHANNEL_H

#include "Interfaces/ICommChannel.h"
#include <queue>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

class MockCommChannel : public ICommChannel {
public:
    std::vector<std::string> transmitted;
    std::queue<std::string>  rxQueue;

    void transmit(const char* data, uint16_t len) override {
        transmitted.emplace_back(data, len);
    }

    uint16_t receive(char* buf, uint16_t maxLen, uint32_t) override {
        if (rxQueue.empty()) return 0;
        const std::string& front = rxQueue.front();
        uint16_t n = static_cast<uint16_t>(std::min(front.size(), static_cast<size_t>(maxLen)));
        std::memcpy(buf, front.c_str(), n);
        rxQueue.pop();
        return n;
    }

    void pushRx(const std::string& line) { rxQueue.push(line); }
    void clearTx() { transmitted.clear(); }
};

#endif // TESTS_MOCKS_MOCKCOMMCHANNEL_H
