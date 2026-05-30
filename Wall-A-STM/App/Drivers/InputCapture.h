#ifndef APP_DRIVERS_INPUTCAPTURE_H
#define APP_DRIVERS_INPUTCAPTURE_H

#pragma once
#include <cstdint>
#include "Interfaces/IInputCaptureHAL.h"

class InputCapture : public IInputCaptureHAL {
public:
	explicit InputCapture(TIM_HandleTypeDef *htim);

	bool init() override;
	uint32_t getLastPulse(uint8_t channel) override;  // résultat en µs (PSC=83 @ 84 MHz)
	bool hasNewPulse(uint8_t channel) const override;

	static void dispatchCallback(TIM_HandleTypeDef *htim);

private:
	TIM_HandleTypeDef *_htim = nullptr;

	static constexpr uint8_t CHANNEL_SIZE = 4;

	uint32_t _riseTime[CHANNEL_SIZE]      = {};
	uint32_t _pulseWidth[CHANNEL_SIZE]    = {};
	bool     _firstCaptured[CHANNEL_SIZE] = {};
	bool     _hasNew[CHANNEL_SIZE]        = {};

	void handleChannel(int idx, uint32_t channel);

	static InputCapture *_instances;
};

#endif // APP_DRIVERS_INPUTCAPTURE_H
