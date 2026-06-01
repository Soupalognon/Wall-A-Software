#ifndef APP_SERVICES_PROXIMETERPOLOLU5472_H
#define APP_SERVICES_PROXIMETERPOLOLU5472_H

#include <cstdint>
#include "Interfaces/IInputCaptureHAL.h"
#include "Interfaces/ISensor.h"

class Pololu5472: public ISensor {
public:
	Pololu5472(IInputCaptureHAL &ic, uint8_t id, const char *name,
		float alarmThreshold, uint32_t periodWindowMs = 0);
	uint8_t id() const override;
	const char* name() const override;
	float read() override;
	bool isAlarm() override;

	void bind() override;
	void trigger() override;
	uint32_t doneFlag() const override;
	bool isActive() const override {
		return false;
	}

private:
	IInputCaptureHAL &_ic;
	uint8_t _id;
	const char *_name;
	float _alarmThreshold;
	float _lastValue = 0.0f;
	uint32_t _periodWindowMs;

	uint32_t _riseTime = 0;
	bool _wasAbove = false;
};

#endif // APP_SERVICES_PROXIMETERPOLOLU5472_H
