#include "Services/B5WLB2101.h"
#include "stm32f4xx_hal.h"

B5WLB2101::B5WLB2101(IAdcHAL &adc, uint8_t id, const char *name, float alarmThreshold,
	uint32_t periodWindowMs) :
	_adc(adc), _id(id), _name(name), _alarmThreshold(alarmThreshold), _periodWindowMs(
		periodWindowMs) {
}

void B5WLB2101::bind() {
	_adc.bind(1u << _id); // _id == SensorType : bit de notif unique global
}

void B5WLB2101::trigger() {
	_adc.start();
}

uint32_t B5WLB2101::doneFlag() const {
	return _adc.doneFlag();
}

float B5WLB2101::read() {
	_lastValue = (_adc.rawValue() / 4096.0f) * 3.3f;

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

bool B5WLB2101::isAlarm() {
	if (_periodWindowMs == 0)
		return _lastValue > _alarmThreshold;

	return _wasAbove && (HAL_GetTick() - _riseTime) >= _periodWindowMs;
}

uint8_t B5WLB2101::id() const {
	return _id;
}

const char* B5WLB2101::name() const {
	return _name;
}
