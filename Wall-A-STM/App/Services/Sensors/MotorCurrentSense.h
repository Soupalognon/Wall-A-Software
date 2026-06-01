#ifndef APP_SERVICES_MOTORCURRENTSENSE_H
#define APP_SERVICES_MOTORCURRENTSENSE_H

#include <cstdint>
#include "Interfaces/IAdcHAL.h"
#include "Interfaces/ISensor.h"

class MotorCurrentSense: public ISensor {
public:
	enum class MotorType {
		PRIMARY, SECONDARY
	};

	MotorCurrentSense(IAdcHAL &adc, MotorType type, uint8_t id, const char *name,
		float alarmThreshold, uint32_t periodWindowMs = 0);
	uint8_t id() const override;
	const char* name() const override;
	float read() override;
	bool isAlarm() override;

	void bind() override;
	void trigger() override;
	uint32_t doneFlag() const override;
	bool isActive() const override {
		return _adc.isActive();
	}

private:
	static constexpr float PRI_RESISTANCE_REFERENCE = 3090.0f;
	static constexpr float PRI_GAIN_FACTOR = 0.000212f;
	static constexpr float SEC_RESISTANCE_REFERENCE = 0.5f;

	IAdcHAL &_adc;
	uint8_t _id;
	const char *_name;
	float _alarmThreshold;
	float _lastValue = 0.0f;
	uint32_t _periodWindowMs;

	uint32_t _riseTime = 0;
	bool _wasAbove = false;

	bool _isPrimaryMotor;

	float voltageToCurrentPrimary(uint16_t adcVal);
	float voltageToCurrentSecondary(uint16_t adcVal);
};

#endif /* APP_SERVICES_MOTORCURRENTSENSE_H */
