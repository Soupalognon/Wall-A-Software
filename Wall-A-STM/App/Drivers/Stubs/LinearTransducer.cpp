#include "Drivers/Stubs/LinearTransducer.h"
#include <cstdio>

LinearTransducer::LinearTransducer(uint8_t id, IActuatorHAL* hal) : _id(id), _hal(hal) {}
uint8_t     LinearTransducer::id()   const { return _id; }
const char* LinearTransducer::name() const { return "LINEAR_TRANSDUCER"; }

void LinearTransducer::command(const char* cmd) {
    float pos = 0.0f;
    sscanf(cmd, "%f", &pos);
    _hal->set(pos);
}
