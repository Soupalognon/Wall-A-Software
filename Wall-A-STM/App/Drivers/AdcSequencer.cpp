#include "Drivers/AdcSequencer.h"

AdcSequencer::AdcSequencer(ADC_HandleTypeDef *hadc, uint32_t doneFlag, const uint32_t *channels,
	uint8_t maxChannel, uint16_t *rawValues) :
	_hadc(hadc), _doneFlag(doneFlag), _channels(channels), _maxChannel(maxChannel), _rawValues(
		rawValues) {
}

void AdcSequencer::configureAndStart(uint8_t index) {
	ADC_ChannelConfTypeDef sConfig = { };
	sConfig.Channel = _channels[index];
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

	HAL_ADC_ConfigChannel(_hadc, &sConfig);
	HAL_ADC_Start_IT(_hadc);
}

void AdcSequencer::onConversionComplete() {
	_rawValues[_conversionIndex++] = (uint16_t) HAL_ADC_GetValue(_hadc);

	if (_conversionIndex < _maxChannel) {
		configureAndStart(_conversionIndex);
	} else {
		_conversionIndex = 0;
		if (_notifyThreadId != nullptr) {
			BaseType_t xHPTW = pdFALSE;
			xTaskNotifyFromISR(_notifyThreadId, _doneFlag, eSetBits, &xHPTW);
			portYIELD_FROM_ISR(xHPTW);
		}
	}
}

void AdcSequencer::bind() {
	_notifyThreadId = xTaskGetCurrentTaskHandle();
}

void AdcSequencer::trigger() {
	_conversionIndex = 0;
	configureAndStart(0);
}

void AdcSequencer::wait() {
	uint32_t flags = 0;
	xTaskNotifyWait(0, _doneFlag, &flags, pdMS_TO_TICKS(100));
}

ADC_HandleTypeDef* AdcSequencer::getInstance() {
	return _hadc;
}
