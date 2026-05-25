#ifndef APP_DRIVERS_INPUTCAPTURE_H
#define APP_DRIVERS_INPUTCAPTURE_H

#pragma once
#include <cstdint>
#include "Interfaces/IInputCaptureHAL.h"

class InputCapture : public IInputCaptureHAL {
public:
	explicit InputCapture(TIM_HandleTypeDef *htim) : _htim(htim) { }

	bool init() override;
	uint32_t getLastPulse(uint32_t channel) const override;  // résultat en µs (PSC=83 @ 84 MHz)
	bool hasNewPulse(uint32_t channel) const override;

	static void dispatchCallback(TIM_HandleTypeDef *htim);

private:
	TIM_HandleTypeDef *_htim = nullptr;

	uint32_t _riseTime[4]      = {};
	uint32_t _pulseWidth[4]    = {};
	bool     _firstCaptured[4] = {};
	bool     _hasNew[4]        = {};

	void handleChannel(int idx, uint32_t channel);

	static InputCapture *_instances[2];
	static int           _instanceCount;
};

#endif // APP_DRIVERS_INPUTCAPTURE_H
