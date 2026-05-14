#ifndef APP_INTERFACES_ICOMMCHANNEL_H
#define APP_INTERFACES_ICOMMCHANNEL_H

#include <cstdint>

class ICommChannel {
public:
    virtual void     transmit(const char* data, uint16_t len) = 0;
    virtual uint16_t receive(char* buf, uint16_t maxLen, uint32_t timeoutMs) = 0;
    virtual ~ICommChannel() = default;
};

#endif // APP_INTERFACES_ICOMMCHANNEL_H
