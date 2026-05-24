#include "Services/AnalogSensor.h"
#include "stm32f4xx_hal.h"

AnalogSensor::AnalogSensor(uint8_t id, const char *name, IAnalogSource *src,
	uint8_t channel, float alarmThreshold, uint32_t periodWindowMs) :
	_id(id), _name(name), _src(src), _channel(channel),
	_alarmThreshold(alarmThreshold),
	_periodWindowMs(periodWindowMs) {
}

uint8_t AnalogSensor::id() const {
	return _id;
}

const char* AnalogSensor::name() const {
	return _name;
}

float AnalogSensor::read() {
	_lastValue = _src->read(_channel);

	if (_periodWindowMs > 0) {
		bool above = _lastValue > _alarmThreshold;
		if (above && !_wasAbove)
			_riseTime = HAL_GetTick();
		else if (!above)
			_riseTime = 0;
		_wasAbove = above;
	}

	return _lastValue;
}

bool AnalogSensor::isAlarm() {
	if (_periodWindowMs == 0)
		return _lastValue > _alarmThreshold;

	return _wasAbove && (HAL_GetTick() - _riseTime) >= _periodWindowMs;
}
