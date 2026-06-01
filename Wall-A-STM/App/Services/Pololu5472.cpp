#include "stm32f4xx_hal.h"
#include <math.h>
#include <Services/Pololu5472.h>

Pololu5472::Pololu5472(IInputCaptureHAL &ic, uint8_t id,
	const char *name, float alarmThreshold, uint32_t periodWindowMs) :
	_ic(ic), _id(id), _name(name), _alarmThreshold(alarmThreshold), _periodWindowMs(
		periodWindowMs) {
}

void Pololu5472::bind() {
}

void Pololu5472::trigger() {
}

uint32_t Pololu5472::doneFlag() const {
	return 0;
}

float Pololu5472::read() {
	if(!_ic.hasNewPulse())
		return std::nanf("");

	_lastValue = static_cast<float>(_ic.getLastPulse());

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

bool Pololu5472::isAlarm() {
	if (_periodWindowMs == 0)
		return _lastValue > _alarmThreshold;

	return _wasAbove && (HAL_GetTick() - _riseTime) >= _periodWindowMs;
}

uint8_t Pololu5472::id() const {
	return _id;
}

const char* Pololu5472::name() const {
	return _name;
}
