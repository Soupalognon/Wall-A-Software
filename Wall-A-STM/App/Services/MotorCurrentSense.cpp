#include "Services/MotorCurrentSense.h"
#include "stm32f4xx_hal.h"

MotorCurrentSense::MotorCurrentSense(IAdcHAL &adc, MotorType type, uint8_t id,
	const char *name, float alarmThreshold, uint32_t periodWindowMs) :
	_adc(adc), _id(id), _name(name), _alarmThreshold(alarmThreshold), _periodWindowMs(
		periodWindowMs), _isPrimaryMotor(type == MotorType::PRIMARY) {
}

void MotorCurrentSense::bind() {
	_adc.bind(1u << _id); // _id == SensorType : bit de notif unique global
}

void MotorCurrentSense::trigger() {
	_adc.start();
}

uint32_t MotorCurrentSense::doneFlag() const {
	return _adc.doneFlag();
}

float MotorCurrentSense::read() {
	if (_isPrimaryMotor)
		_lastValue = voltageToCurrentPrimary(_adc.rawValue());
	else
		_lastValue = voltageToCurrentSecondary(_adc.rawValue());

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

bool MotorCurrentSense::isAlarm() {
	if (_periodWindowMs == 0)
		return _lastValue > _alarmThreshold;

	return _wasAbove && (HAL_GetTick() - _riseTime) >= _periodWindowMs;
}

uint8_t MotorCurrentSense::id() const {
	return _id;
}

const char* MotorCurrentSense::name() const {
	return _name;
}

float MotorCurrentSense::voltageToCurrentPrimary(uint16_t adcVal) {
	float voltage = (adcVal / 4096.0f) * 3.3f;
	voltage *= 1000.0f;
	voltage /= PRI_GAIN_FACTOR;
	return voltage / PRI_RESISTANCE_REFERENCE;
}

float MotorCurrentSense::voltageToCurrentSecondary(uint16_t adcVal) {
	float voltage = (adcVal / 4096.0f) * 3.3f;
	return voltage / SEC_RESISTANCE_REFERENCE;
}
