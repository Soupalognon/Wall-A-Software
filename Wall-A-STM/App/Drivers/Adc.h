#ifndef APP_DRIVERS_ADC_H
#define APP_DRIVERS_ADC_H

#include <cstdint>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Interfaces/IAdcHAL.h"

class Adc: public IAdcHAL {
public:
	Adc(ADC_HandleTypeDef *hadc, const uint32_t channel);

	void onConversionComplete();

	void bind(uint32_t doneFlag) override;

	bool isActive() const override {
		return _isActive;
	}

	uint16_t rawValue() override {
		return _rawValue;
	}

	uint32_t doneFlag() const override {
		return _doneFlag;
	}

	void start() override;

	// Routes an end-of-conversion interrupt to the active object of the relevant
	// peripheral. On a single hadc the channels are sequential (only one active), but
	// several hadc can convert in parallel, hence the resolution by (hadc, _isActive).
	static void dispatchCallback(ADC_HandleTypeDef *hadc);

private:
	uint32_t _doneFlag = 0;
	TaskHandle_t _notifyThreadId = nullptr;

	ADC_HandleTypeDef *_hadc;

	ADC_ChannelConfTypeDef _sConfig = { };
	uint16_t _rawValue;
	bool _isActive = false;

	// Intrusive registry of all instances (self-registration in the constructor)
	static Adc *s_head;
	Adc *_next = nullptr;

};

#endif /* APP_DRIVERS_ADC_H */
