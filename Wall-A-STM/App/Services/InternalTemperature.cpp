#include <math.h>
#include <Services/InternalTemperature.h>
#include "stm32f4xx_hal.h"

InternalTemperature::InternalTemperature(IAdcHAL &adc, uint8_t id, const char *name,
	float alarmThreshold, uint32_t periodWindowMs) :
	_adc(adc), _id(id), _name(name), _alarmThreshold(alarmThreshold), _periodWindowMs(
		periodWindowMs) {
}

void InternalTemperature::bind() {
	_adc.bind(1u << _id); // _id == SensorType : bit de notif unique global
}

void InternalTemperature::trigger() {
	_adc.start();
}

uint32_t InternalTemperature::doneFlag() const {
	return _adc.doneFlag();
}

float InternalTemperature::read() {
	_lastValue = voltageToCelsius(_adc.rawValue());

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

bool InternalTemperature::isAlarm() {
	if (_periodWindowMs == 0)
		return _lastValue > _alarmThreshold;

	return _wasAbove && (HAL_GetTick() - _riseTime) >= _periodWindowMs;
}

float InternalTemperature::voltageToCelsius(uint16_t adcVal) {
	float voltage = (adcVal / 4096.0f) * 3.3f;
	float res = voltage * RESISTANCE_REFERENCE / (3.3f - voltage);
	float tempK =
		1.0f
			/ (1.0f / (273.15f + TEMPERATURE_REFERENCE)
				+ logf(res / RESISTANCE_REFERENCE) / B_REFERENCE);
	return tempK - 273.15f;
}

uint8_t InternalTemperature::id() const {
	return _id;
}

const char* InternalTemperature::name() const {
	return _name;
}
