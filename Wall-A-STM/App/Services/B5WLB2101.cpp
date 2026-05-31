#include "Services/B5WLB2101.h"

static constexpr uint32_t CHANNELS[] = {
	ADC_CHANNEL_12, ADC_CHANNEL_10, ADC_CHANNEL_13, ADC_CHANNEL_9
};

B5WLB2101::B5WLB2101(ADC_HandleTypeDef *hadc, channelEnum channel,
	uint32_t doneFlag, uint8_t id, const char *name, float alarmThreshold, uint32_t periodWindowMs) :
	Adc(hadc, CHANNELS[channel], doneFlag), _id(id), _name(name), _alarmThreshold(alarmThreshold), _periodWindowMs(
		periodWindowMs) {
}

void B5WLB2101::bind() {
	_notifyThreadId = xTaskGetCurrentTaskHandle();
}

void B5WLB2101::trigger() {
	start();
}

uint32_t B5WLB2101::doneFlag() const {
	return _doneFlag;
}

float B5WLB2101::read() {
	_lastValue = (rawValue() / 4096.0f) * 3.3f;

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
