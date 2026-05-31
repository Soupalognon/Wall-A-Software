#ifndef INTERNAL_TEMPERATURES_HPP_
#define INTERNAL_TEMPERATURES_HPP_

#include <Interfaces/ISensor.h>
#include <cstdint>
#include "Drivers/Adc.h"

class InternalTemperature: public Adc, public ISensor {
public:
	typedef enum {
		PRIMARY_MOTOR = 0, SECONDARY_MOTOR = 1, POWER_SUPPLIES = 2,
	} channelEnum;

	InternalTemperature(ADC_HandleTypeDef *hadc, channelEnum channel, uint32_t doneFlag,
		uint8_t id, const char *name, float alarmThreshold, uint32_t periodWindowMs = 0);
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
	static constexpr uint16_t RESISTANCE_REFERENCE = 10000;
	static constexpr float B_REFERENCE = 3434.0f;
	static constexpr uint8_t TEMPERATURE_REFERENCE = 25;

	uint16_t _rawBuf[3] = { };
	uint8_t _id;
	const char *_name;
	float _alarmThreshold;
	float _lastValue = 0.0f;
	uint32_t _periodWindowMs;

	uint32_t _riseTime = 0;
	bool _wasAbove = false;

	float voltageToCelsius(uint16_t adcVal);
};

#endif /* INTERNAL_TEMPERATURES_HPP_ */
