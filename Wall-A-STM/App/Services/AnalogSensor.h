#ifndef APP_DRIVERS_ANALOGSENSOR_H
#define APP_DRIVERS_ANALOGSENSOR_H

#include "Interfaces/ISensor.h"
#include "Interfaces/IAnalogSource.h"
#include <cstdint>

class AnalogSensor: public ISensor {
public:
	AnalogSensor(uint8_t id, const char *name, IAnalogSource *src, uint8_t channel,
		float alarmThreshold);
	uint8_t id() const override;
	const char* name() const override;
	float read() override;
	bool isAlarm() override;

private:
	uint8_t _id;
	const char *_name;
	IAnalogSource *_src;
	uint8_t _channel;
	float _alarmThreshold;
	float _lastValue = 0.0f;
};

#endif // APP_DRIVERS_ANALOGSENSOR_H
