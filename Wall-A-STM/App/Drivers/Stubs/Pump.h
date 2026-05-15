#ifndef APP_DRIVERS_PUMP_H
#define APP_DRIVERS_PUMP_H

#include "Interfaces/IActuator.h"
#include "Interfaces/IActuatorHAL.h"
#include <cstdint>
#include <cstring>

class Pump : public IActuator {
public:
    Pump(uint8_t id, IActuatorHAL* hal);
    uint8_t     id()   const override;
    const char* name() const override;
    void command(const char* cmd) override;
private:
    uint8_t       _id;
    IActuatorHAL* _hal;
};

#endif // APP_DRIVERS_PUMP_H
