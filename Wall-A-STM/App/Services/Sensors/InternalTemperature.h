#ifndef APP_SERVICES_INTERNALTEMPERATURE_H
#define APP_SERVICES_INTERNALTEMPERATURE_H

#include <cstdint>
#include "Interfaces/IAdcHAL.h"
#include "Interfaces/ISensor.h"

class InternalTemperature: public ISensor {
public:
	InternalTemperature(IAdcHAL &adc, uint8_t id, const char *name, float alarmThreshold,
		uint32_t periodWindowMs = 0);
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
	static constexpr uint16_t RESISTANCE_REFERENCE = 10000;
	static constexpr float B_REFERENCE = 3434.0f;
	static constexpr uint8_t TEMPERATURE_REFERENCE = 25;

	IAdcHAL &_adc;
	uint8_t _id;
	const char *_name;
	float _alarmThreshold;
	float _lastValue = 0.0f;
	uint32_t _periodWindowMs;

	uint32_t _riseTime = 0;
	bool _wasAbove = false;

	float voltageToCelsius(uint16_t adcVal);
};

#endif /* APP_SERVICES_INTERNALTEMPERATURE_H */
