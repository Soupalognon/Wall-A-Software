#ifndef APP_DRIVERS_SERVO_H
#define APP_DRIVERS_SERVO_H

#include "Interfaces/IActuator.h"
#include "Interfaces/IActuatorHAL.h"
#include <cstdint>

class Servo : public IActuator {
public:
    Servo(uint8_t id, IActuatorHAL* hal);
    uint8_t     id()   const override;
    const char* name() const override;
    void command(const char* cmd) override;
private:
    uint8_t       _id;
    IActuatorHAL* _hal;
};

#endif // APP_DRIVERS_SERVO_H
