//#ifndef MOTOR_MOTORTEMPERATURES_HPP_
//#define MOTOR_MOTORTEMPERATURES_HPP_
//
//#pragma once
//
//#include <cstdint>
//#include "stm32f4xx_hal.h"
//#include "cmsis_os2.h"
//
//namespace DriversCustom {
//
//class MotorTemperatures {
//public:
//	static constexpr uint32_t ADC_DONE_FLAG = 0x02;
//
//	typedef enum {
//		PRIMARY_MOTOR_LEFT = 0, PRIMARY_MOTOR_RIGHT = 1,
//		SECONDARY_MOTOR_LEFT = 2, SECONDARY_MOTOR_RIGHT = 3
//	} channel;
//
//	MotorTemperatures() = delete;
//
//	static void init();
//	static void setNotifyThread(osThreadId_t threadId);
//	static void startConversion();
//	static float getCurrent(channel ch);
//
//	// Called from HAL_ADC_ConvCpltCallback in main_cpp.cpp
//	static void onConversionComplete(ADC_HandleTypeDef *hadc);
//
//private:
//	static constexpr uint32_t CHANNELS[4] = {
//		ADC_CHANNEL_3,
//		ADC_CHANNEL_4,
//		ADC_CHANNEL_6,
//		ADC_CHANNEL_8
//	};
//
//	static constexpr float PRI_RESISTANCE_REFERENCE = 3090.0f;
//	static constexpr float PRI_GAIN_FACTOR = 0.000212f;
//	static constexpr float SEC_RESISTANCE_REFERENCE = 0.5f;
//
//	static osThreadId_t notifyThreadId_;
//	static uint16_t     rawValues_[4];
//	static uint8_t      conversionIndex_;
//
//	static void configureAndStart(uint8_t index);
//	static float voltageToCurrentPrimary(uint16_t adcVal);
//	static float voltageToCurrentSecondary(uint16_t adcVal);
//};
//
//} // namespace DriversCustom
//
//#endif /* MOTOR_MOTORTEMPERATURES_HPP_ */
