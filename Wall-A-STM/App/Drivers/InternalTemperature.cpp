/*
 * InternalTemperatures.cpp
 *
 *  Created on: 8 mai 2026
 *      Author: gdurand
 */

#include "Drivers/InternalTemperature.h"
#include <cstdint>
#include <math.h>

void InternalTemperature::setNotifyThread(TaskHandle_t threadId) {
	_notifyThreadId = threadId;
}

void InternalTemperature::startConversion() {
	_conversionIndex = 0;
	configureAndStart(0);
}

void InternalTemperature::configureAndStart(uint8_t index) {
	ADC_ChannelConfTypeDef sConfig = { };
	sConfig.Channel = CHANNELS[index];
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

	HAL_ADC_ConfigChannel(_hadc, &sConfig);
	HAL_ADC_Start_IT(_hadc);
}

// Called from HAL_ADC_ConvCpltCallback — ISR context
void InternalTemperature::onConversionComplete() {
	_rawValues[_conversionIndex++] = (uint16_t) HAL_ADC_GetValue(_hadc);

	if (_conversionIndex < MAX_CHANNEL) {
		configureAndStart(_conversionIndex);
	} else {
		_conversionIndex = 0;
		if (_notifyThreadId != nullptr) {
			BaseType_t xHPTW = pdFALSE;
			xTaskNotifyFromISR(_notifyThreadId, ADC_DONE_FLAG, eSetBits, &xHPTW);
			portYIELD_FROM_ISR(xHPTW);
		}
	}
}

float InternalTemperature::getTemperature(channel ch) {
	return voltageToCelsius(_rawValues[ch]);
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
