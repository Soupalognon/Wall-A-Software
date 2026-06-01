#include "Drivers/Adc.h"

Adc *Adc::s_head = nullptr;

Adc::Adc(ADC_HandleTypeDef *hadc, const uint32_t channel) :
	_hadc(hadc) {
	_sConfig.Channel = channel;
	_sConfig.Rank = 1;
	_sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

	_next = s_head;
	s_head = this;
}

void Adc::bind(uint32_t doneFlag) {
	_notifyThreadId = xTaskGetCurrentTaskHandle();
	_doneFlag = doneFlag;
}

void Adc::start() {
	HAL_ADC_ConfigChannel(_hadc, &_sConfig);
	_isActive = true;	// before Start_IT: avoids the race if the IT fires immediately
	HAL_ADC_Start_IT(_hadc);
}

void Adc::onConversionComplete() {
	_rawValue = (uint16_t) HAL_ADC_GetValue(_hadc);

	if (_notifyThreadId != nullptr) {
		BaseType_t xHPTW = pdFALSE;
		xTaskNotifyFromISR(_notifyThreadId, _doneFlag, eSetBits, &xHPTW);
		portYIELD_FROM_ISR(xHPTW);
	}

	_isActive = false;
}

void Adc::dispatchCallback(ADC_HandleTypeDef *hadc) {
	for (Adc *a = s_head; a != nullptr; a = a->_next) {
		if (a->_hadc == hadc && a->_isActive) {
			a->onConversionComplete();	// resets _isActive = false
			return;
		}
	}
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	Adc::dispatchCallback(hadc);
}
