#ifndef APP_INTERFACES_IACTUATORMANAGER_H
#define APP_INTERFACES_IACTUATORMANAGER_H

#include <cstdint>

class IActuatorManager {
public:
    virtual void commandById(uint8_t id, const char* cmd) = 0;
    virtual ~IActuatorManager() = default;
};

#endif // APP_INTERFACES_IACTUATORMANAGER_H
