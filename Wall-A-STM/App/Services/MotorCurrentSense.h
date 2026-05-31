#ifndef MOTOR_MOTORTEMPERATURES_HPP_
#define MOTOR_MOTORTEMPERATURES_HPP_

#include <cstdint>
#include "Drivers/Adc.h"
#include "Interfaces/ISensor.h"

class MotorCurrentSense: public Adc, public ISensor {
public:
	typedef enum {
		PRIMARY_MOTOR_LEFT = 0,
		PRIMARY_MOTOR_RIGHT = 1,
		SECONDARY_MOTOR_LEFT = 2,
		SECONDARY_MOTOR_RIGHT = 3
	} channelEnum;

	MotorCurrentSense(ADC_HandleTypeDef *hadc, channelEnum channel, uint32_t doneFlag, uint8_t id,
		const char *name, float alarmThreshold, uint32_t periodWindowMs = 0);
	uint8_t id() const override;
	const char* name() const override;
	float read() override;
	bool isAlarm() override;

	void bind() override;
	void trigger() override;
	uint32_t doneFlag() const override;
	bool isActive() const override {
		return Adc::isActive();
	}

private:
	static constexpr float PRI_RESISTANCE_REFERENCE = 3090.0f;
	static constexpr float PRI_GAIN_FACTOR = 0.000212f;
	static constexpr float SEC_RESISTANCE_REFERENCE = 0.5f;

	uint8_t _id;
	const char *_name;
	float _alarmThreshold;
	float _lastValue = 0.0f;
	uint32_t _periodWindowMs;

	uint32_t _riseTime = 0;
	bool _wasAbove = false;

	bool _isPrimaryMotor = false;

	float voltageToCurrentPrimary(uint16_t adcVal);
	float voltageToCurrentSecondary(uint16_t adcVal);
};

#endif /* MOTOR_MOTORTEMPERATURES_HPP_ */
