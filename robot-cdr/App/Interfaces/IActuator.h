#ifndef APP_INTERFACES_IACTUATOR_H
#define APP_INTERFACES_IACTUATOR_H

#include <cstdint>

class IActuator {
public:
    virtual uint8_t     id()   const = 0;
    virtual const char* name() const = 0;
    virtual void command(const char* cmd) = 0;
    virtual ~IActuator() = default;
};

#endif // APP_INTERFACES_IACTUATOR_H
