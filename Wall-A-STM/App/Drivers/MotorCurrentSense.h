#ifndef MOTOR_MOTORTEMPERATURES_HPP_
#define MOTOR_MOTORTEMPERATURES_HPP_

#pragma once

#include <cstdint>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

class MotorCurrentSense {
public:
	static constexpr uint32_t ADC_DONE_FLAG = 0x02;

	typedef enum {
		PRIMARY_MOTOR_LEFT = 0,
		PRIMARY_MOTOR_RIGHT = 1,
		SECONDARY_MOTOR_LEFT = 2,
		SECONDARY_MOTOR_RIGHT = 3
	} channel;

	MotorCurrentSense(ADC_HandleTypeDef *hadc) :
		_hadc(hadc) {
	}

	void setNotifyThread(TaskHandle_t threadId);
	void startConversion();
	float getCurrentSense(channel ch);

	// Called from HAL_ADC_ConvCpltCallback
	void onConversionComplete();

	ADC_HandleTypeDef* getInstance() {
		return _hadc;
	}

private:
	ADC_HandleTypeDef *_hadc;

	static constexpr uint8_t MAX_CHANNEL = 4;
	static constexpr uint32_t CHANNELS[MAX_CHANNEL] = {
	ADC_CHANNEL_3,
	ADC_CHANNEL_4,
	ADC_CHANNEL_6,
	ADC_CHANNEL_8 };

	static constexpr float PRI_RESISTANCE_REFERENCE = 3090.0f;
	static constexpr float PRI_GAIN_FACTOR = 0.000212f;
	static constexpr float SEC_RESISTANCE_REFERENCE = 0.5f;

	TaskHandle_t _notifyThreadId = nullptr;
	uint16_t _rawValues[MAX_CHANNEL] = { };
	uint8_t _conversionIndex = 0;

	void configureAndStart(uint8_t index);
	float voltageToCurrentPrimary(uint16_t adcVal);
	float voltageToCurrentSecondary(uint16_t adcVal);
};

#endif /* MOTOR_MOTORTEMPERATURES_HPP_ */
