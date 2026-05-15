#include "Drivers/Stubs/ProximitySensor.h"

ProximitySensor::ProximitySensor(uint8_t id, ISensorHAL* hal)
    : _id(id), _hal(hal), _lastValue(0.0f) {}

uint8_t     ProximitySensor::id()   const { return _id; }
const char* ProximitySensor::name() const { return "PROXIMITY"; }

float ProximitySensor::read() {
    _lastValue = _hal->read();
    return _lastValue;
}

bool ProximitySensor::isAlarm() {
    return _lastValue < Config::PROXIMITY_ALARM_M;
}
