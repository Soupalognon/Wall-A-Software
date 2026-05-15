#include "Drivers/Stubs/CurrentSensor.h"

CurrentSensor::CurrentSensor(uint8_t id, ISensorHAL* hal)
    : _id(id), _hal(hal), _lastValue(0.0f) {}

uint8_t     CurrentSensor::id()   const { return _id; }
const char* CurrentSensor::name() const { return "CURRENT"; }

float CurrentSensor::read() {
    _lastValue = _hal->read();
    return _lastValue;
}

bool CurrentSensor::isAlarm() {
    return _lastValue > Config::CURRENT_ALARM_A;
}
