#ifndef INTERNAL_TEMPERATURES_HPP_
#define INTERNAL_TEMPERATURES_HPP_

#pragma once

#include <cstdint>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

class InternalTemperature {
public:
	typedef enum {
		PRIMARY_MOTOR = 0, SECONDARY_MOTOR = 1, POWER_SUPPLIES = 2,
	} channel;

	static constexpr uint32_t ADC_DONE_FLAG = 0x01;

	InternalTemperature(ADC_HandleTypeDef *hadc) :
		_hadc(hadc) {
	}

	void setNotifyThread(TaskHandle_t threadId);
	void startConversion();
	float getTemperature(channel ch);

	// Called from HAL_ADC_ConvCpltCallback
	void onConversionComplete();

	ADC_HandleTypeDef* getInstance() {
		return _hadc;
	}

private:
	ADC_HandleTypeDef *_hadc;

	static constexpr uint8_t MAX_CHANNEL = 3;
	static constexpr uint32_t CHANNELS[MAX_CHANNEL] = {
	ADC_CHANNEL_4,
	ADC_CHANNEL_5,
	ADC_CHANNEL_6, };

	static constexpr uint16_t RESISTANCE_REFERENCE = 10000;
	static constexpr float B_REFERENCE = 3434.0f;
	static constexpr uint8_t TEMPERATURE_REFERENCE = 25;

	TaskHandle_t _notifyThreadId = nullptr;
	uint16_t _rawValues[3] = { 0, 0, 0 };
	uint8_t _conversionIndex = 0;

	void configureAndStart(uint8_t index);
	float voltageToCelsius(uint16_t adcVal);
};

#endif /* INTERNAL_TEMPERATURES_HPP_ */
