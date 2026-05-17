#include <cstdint>
#include <math.h>
#include "Drivers/MotorCurrentSense.h"

void MotorCurrentSense::setNotifyThread(TaskHandle_t threadId) {
	_notifyThreadId = threadId;
}

void MotorCurrentSense::startConversion() {
	_conversionIndex = 0;
	configureAndStart(0);
}

void MotorCurrentSense::configureAndStart(uint8_t index) {
	ADC_ChannelConfTypeDef sConfig = { };
	sConfig.Channel = CHANNELS[index];
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

	HAL_ADC_ConfigChannel(_hadc, &sConfig);
	HAL_ADC_Start_IT(_hadc);
}

// Called from HAL_ADC_ConvCpltCallback — ISR context
void MotorCurrentSense::onConversionComplete() {
	_rawValues[_conversionIndex++] = (uint16_t) HAL_ADC_GetValue(_hadc);

	if (_conversionIndex < 2) {
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

float MotorCurrentSense::getCurrentSense(channel ch) {
	if (ch == channel::PRIMARY_MOTOR_LEFT || ch == channel::PRIMARY_MOTOR_RIGHT)
		return voltageToCurrentPrimary(_rawValues[ch]);
//	if (ch == channel::SECONDARY_MOTOR_LEFT || ch == channel::SECONDARY_MOTOR_RIGHT)
	else
		return voltageToCurrentSecondary(_rawValues[ch]);
}

float MotorCurrentSense::voltageToCurrentPrimary(uint16_t adcVal) {
	float voltage = (adcVal / 4096.0f) * 3.3f;
	voltage *= 1000.0f;	//Convert to mA
	voltage /= PRI_GAIN_FACTOR;
	return voltage / PRI_RESISTANCE_REFERENCE;
}

float MotorCurrentSense::voltageToCurrentSecondary(uint16_t adcVal) {
	float voltage = (adcVal / 4096.0f) * 3.3f;
//	voltage *= 1000.0f;
//	return voltage / SEC_RESISTANCE_REFERENCE;
	return voltage;
}
