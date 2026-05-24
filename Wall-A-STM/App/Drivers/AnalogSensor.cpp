#include "Drivers/AnalogSensor.h"

AnalogSensor::AnalogSensor(uint8_t id, const char *name, IAnalogSource *src,
                           uint8_t channel, float alarmThreshold)
    : _id(id), _name(name), _src(src), _channel(channel),
      _alarmThreshold(alarmThreshold) {}

uint8_t AnalogSensor::id() const {
    return _id;
}

const char *AnalogSensor::name() const {
    return _name;
}

float AnalogSensor::read() {
    _lastValue = _src->read(_channel);
    return _lastValue;
}

bool AnalogSensor::isAlarm() {
    return _lastValue > _alarmThreshold;
}
