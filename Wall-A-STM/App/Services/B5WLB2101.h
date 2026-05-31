#ifndef APP_SERVICES_B5WLB2101_H
#define APP_SERVICES_B5WLB2101_H

#include <cstdint>
#include "Drivers/Adc.h"
#include "Interfaces/ISensor.h"

class B5WLB2101: public Adc, public ISensor {
public:
	typedef enum {
		CH_1 = 0, CH_2 = 1, CH_3 = 2, CH_4 = 3
	} channelEnum;

	B5WLB2101(ADC_HandleTypeDef *hadc, channelEnum channel, uint32_t doneFlag, uint8_t id,
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
	uint8_t _id;
	const char *_name;
	float _alarmThreshold;
	float _lastValue = 0.0f;
	uint32_t _periodWindowMs;

	uint32_t _riseTime = 0;
	bool _wasAbove = false;
};

#endif // APP_SERVICES_B5WLB2101_H
