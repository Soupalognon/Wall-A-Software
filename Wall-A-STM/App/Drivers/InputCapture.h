#ifndef APP_DRIVERS_INPUTCAPTURE_H
#define APP_DRIVERS_INPUTCAPTURE_H

#pragma once
#include <cstdint>
#include "stm32f4xx_hal.h"
#include "Interfaces/IInputCaptureHAL.h"

class InputCapture : public IInputCaptureHAL {
public:
	InputCapture(TIM_HandleTypeDef *htim, uint32_t channel);

	bool init() override;
	uint32_t getLastPulse() override;  // us (PSC=83 @ 84 MHz)
	bool hasNewPulse() const override;

	// Routes a capture interrupt to the instance whose (htim, active channel)
	// matches. On a single htim each channel is a distinct instance, hence the
	// resolution by (_htim, _activeChannel).
	static void dispatchCallback(TIM_HandleTypeDef *htim);

	// Initializes all registered instances (walks the linked list).
	// Call from cppMain(), after HAL_Init / MX_TIMx_Init. Returns false if
	// at least one init failed.
	static bool initAll();

private:
	TIM_HandleTypeDef *_htim;
	uint32_t _channel;                       // TIM_CHANNEL_x : Start_IT / ReadCapturedValue
	HAL_TIM_ActiveChannel _activeChannel;    // HAL_TIM_ACTIVE_CHANNEL_x : matching dispatch

	uint32_t _riseTime    = 0;
	uint32_t _pulseWidth  = 0;
	bool     _firstCaptured = false;
	bool     _hasNew        = false;

	void onCapture();

	// Intrusive registry of all instances (self-registration in the constructor)
	static InputCapture *s_head;
	InputCapture *_next = nullptr;
};

#endif // APP_DRIVERS_INPUTCAPTURE_H
