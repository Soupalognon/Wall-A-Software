#include "Drivers/Stubs/Servo.h"
#include <cstdio>

Servo::Servo(uint8_t id, IActuatorHAL* hal) : _id(id), _hal(hal) {}
uint8_t     Servo::id()   const { return _id; }
const char* Servo::name() const { return "SERVO"; }

void Servo::command(const char* cmd) {
    float angle = 0.0f;
    sscanf(cmd, "%f", &angle);
    _hal->set(angle);
}
