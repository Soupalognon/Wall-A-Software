#ifndef APP_INTERFACES_IBUS_H
#define APP_INTERFACES_IBUS_H

#include <cstdint>

enum class Topic : uint8_t {
    TELEMETRY = 0,
    ALERT     = 1,
    LOG       = 2,
    HEALTH    = 3
};

class IBus {
public:
    virtual void publish(Topic topic, const char* payload) = 0;
    virtual ~IBus() = default;
};

#endif // APP_INTERFACES_IBUS_H
