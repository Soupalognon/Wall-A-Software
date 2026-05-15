#include "Drivers/Stubs/TemperatureSensor.h"

TemperatureSensor::TemperatureSensor(uint8_t id, ISensorHAL* hal)
    : _id(id), _hal(hal), _lastValue(0.0f) {}

uint8_t     TemperatureSensor::id()   const { return _id; }
const char* TemperatureSensor::name() const { return "TEMPERATURE"; }

float TemperatureSensor::read() {
    _lastValue = _hal->read();
    return _lastValue;
}

bool TemperatureSensor::isAlarm() {
    return _lastValue > Config::TEMP_ALARM_C;
}
