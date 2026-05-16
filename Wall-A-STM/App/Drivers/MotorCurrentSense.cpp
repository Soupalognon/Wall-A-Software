//#include <cstdint>
//#include <math.h>
//#include <MotorTemperatures.hpp>
//
//extern ADC_HandleTypeDef hadc1;
//
//namespace DriversCustom {
//
//// Statics
//constexpr uint32_t MotorTemperatures::CHANNELS[4];
//osThreadId_t MotorTemperatures::notifyThreadId_ = nullptr;
//uint16_t MotorTemperatures::rawValues_[4] = { 0, 0, 0, 0 };
//uint8_t MotorTemperatures::conversionIndex_ = 0;
//
//void MotorTemperatures::init() {
//	conversionIndex_ = 0;
//}
//
//void MotorTemperatures::setNotifyThread(osThreadId_t threadId) {
//	notifyThreadId_ = threadId;
//}
//
//void MotorTemperatures::startConversion() {
//	conversionIndex_ = 0;
//	configureAndStart(0);
//}
//
//void MotorTemperatures::configureAndStart(uint8_t index) {
//	ADC_ChannelConfTypeDef sConfig = { };
//	sConfig.Channel = CHANNELS[index];
//	sConfig.Rank = 1;
//	sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
//
//	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//	HAL_ADC_Start_IT(&hadc1);
//}
//
//// Called from HAL_ADC_ConvCpltCallback — ISR context
//void MotorTemperatures::onConversionComplete(ADC_HandleTypeDef *hadc) {
//	if (hadc != &hadc1)
//		return;
//
//	rawValues_[conversionIndex_++] = (uint16_t) HAL_ADC_GetValue(hadc);
//
//	if (conversionIndex_ < 2) {
//		configureAndStart(conversionIndex_);
//	} else {
//		conversionIndex_ = 0;
//		if (notifyThreadId_ != nullptr) {
//			osThreadFlagsSet(notifyThreadId_, ADC_DONE_FLAG);
//		}
//	}
//}
//
//float MotorTemperatures::getCurrent(channel ch) {
//	if (ch == channel::PRIMARY_MOTOR_LEFT || ch == channel::PRIMARY_MOTOR_RIGHT)
//		return voltageToCurrentPrimary(rawValues_[ch]);
////	if (ch == channel::SECONDARY_MOTOR_LEFT || ch == channel::SECONDARY_MOTOR_RIGHT)
//	else
//		return voltageToCurrentSecondary(rawValues_[ch]);
//}
//
//float MotorTemperatures::voltageToCurrentPrimary(uint16_t adcVal) {
//	float voltage = (adcVal / 4096.0f) * 3.3f;
//	voltage *= 1000.0f;	//Convert to mA
//	voltage /= PRI_GAIN_FACTOR;
//	return voltage / PRI_RESISTANCE_REFERENCE;
//}
//
//float MotorTemperatures::voltageToCurrentSecondary(uint16_t adcVal) {
//	float voltage = (adcVal / 4096.0f) * 3.3f;
////	voltage *= 1000.0f;
////	return voltage / SEC_RESISTANCE_REFERENCE;
//	return voltage;
//}
//
//} // namespace DriversCustom
