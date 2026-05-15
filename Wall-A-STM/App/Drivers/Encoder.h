#ifndef APP_DRIVERS_ENCODER_H
#define APP_DRIVERS_ENCODER_H

#pragma once
#include <cstdint>
#include "Interfaces/IEncoderHAL.h"

class Encoder: public IEncoderHAL {
public:
	Encoder(TIM_HandleTypeDef *htim) : _htim(htim) {};
	bool init();
	int32_t getTicks(); // extended 32-bit position, must be called regularly (delta < 32767 ticks between calls)

private:
	int16_t _oldValue = 0;
	int32_t _position = 0;
	TIM_HandleTypeDef *_htim = nullptr;

	void reset();
};

#endif // APP_DRIVERS_ENCODER_H
