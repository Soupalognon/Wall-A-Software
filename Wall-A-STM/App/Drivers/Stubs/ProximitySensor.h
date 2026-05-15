#ifndef APP_DRIVERS_PROXIMITYSENSOR_H
#define APP_DRIVERS_PROXIMITYSENSOR_H

#include "Interfaces/ISensor.h"
#include "Interfaces/ISensorHAL.h"
#include "Config.h"
#include <cstdint>

class ProximitySensor : public ISensor {
public:
    ProximitySensor(uint8_t id, ISensorHAL* hal);
    uint8_t     id()   const override;
    const char* name() const override;
    float       read()       override;
    bool        isAlarm()    override;
private:
    uint8_t     _id;
    ISensorHAL* _hal;
    float       _lastValue;
};

#endif // APP_DRIVERS_PROXIMITYSENSOR_H
