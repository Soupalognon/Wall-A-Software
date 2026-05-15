#include "Drivers/Stubs/Pump.h"

Pump::Pump(uint8_t id, IActuatorHAL* hal) : _id(id), _hal(hal) {}
uint8_t     Pump::id()   const { return _id; }
const char* Pump::name() const { return "PUMP"; }

void Pump::command(const char* cmd) {
    if (strcmp(cmd, "ON") == 0)        _hal->set(1.0f);
    else if (strcmp(cmd, "OFF") == 0)  _hal->set(0.0f);
}
