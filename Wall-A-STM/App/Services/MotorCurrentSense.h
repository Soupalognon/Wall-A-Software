#ifndef MOTOR_MOTORTEMPERATURES_HPP_
#define MOTOR_MOTORTEMPERATURES_HPP_

#include <cstdint>
#include "Drivers/AdcSequencer.h"

class MotorCurrentSense: public AdcSequencer {
public:
	typedef enum {
		PRIMARY_MOTOR_LEFT = 0,
		PRIMARY_MOTOR_RIGHT = 1,
		SECONDARY_MOTOR_LEFT = 2,
		SECONDARY_MOTOR_RIGHT = 3
	} channel;

	MotorCurrentSense(ADC_HandleTypeDef *hadc);
	float read(uint8_t ch) override;

private:
	static constexpr float PRI_RESISTANCE_REFERENCE = 3090.0f;
	static constexpr float PRI_GAIN_FACTOR = 0.000212f;
	static constexpr float SEC_RESISTANCE_REFERENCE = 0.5f;

	uint16_t _rawBuf[4] = { };

	float voltageToCurrentPrimary(uint16_t adcVal);
	float voltageToCurrentSecondary(uint16_t adcVal);
};

#endif /* MOTOR_MOTORTEMPERATURES_HPP_ */
