#include "Services/MotorCurrentSense.h"

static constexpr uint32_t CHANNELS[] = {
ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_6, ADC_CHANNEL_8 };

MotorCurrentSense::MotorCurrentSense(ADC_HandleTypeDef *hadc, channelEnum channel,
	uint32_t doneFlag, uint8_t id, const char *name, float alarmThreshold, uint32_t periodWindowMs) :
	Adc(hadc, CHANNELS[channel], doneFlag), _id(id), _name(name), _alarmThreshold(alarmThreshold), _periodWindowMs(
		periodWindowMs) {
	if (channel == channelEnum::PRIMARY_MOTOR_LEFT || channel == channelEnum::PRIMARY_MOTOR_RIGHT)
		_isPrimaryMotor = true;
}

void MotorCurrentSense::bind() {
	_notifyThreadId = xTaskGetCurrentTaskHandle();
}

void MotorCurrentSense::trigger() {
	start();
}

uint32_t MotorCurrentSense::doneFlag() const {
	return _doneFlag;
}

float MotorCurrentSense::read() {
	if (_isPrimaryMotor)
		_lastValue = voltageToCurrentPrimary(rawValue());
	else
		_lastValue = voltageToCurrentSecondary(rawValue());

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
