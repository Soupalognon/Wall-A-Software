#ifndef APP_INTERFACES_ISENSOR_H
#define APP_INTERFACES_ISENSOR_H

#include <cstdint>

class ISensor {
public:
    virtual uint8_t     id()   const = 0;
    virtual const char* name() const = 0;
    virtual float       read()       = 0;
    virtual ~ISensor() = default;
};

#endif // APP_INTERFACES_ISENSOR_H
