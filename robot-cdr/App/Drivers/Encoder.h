#ifndef APP_DRIVERS_ENCODER_H
#define APP_DRIVERS_ENCODER_H

#pragma once
#include <cstdint>
#include <stm32f4xx_hal.h>

class Encoder {
public:
	Encoder() = default;
	void init(TIM_HandleTypeDef *htim);
	int32_t refresh(); // extended 32-bit position, must be called regularly (delta < 32767 ticks between calls)

private:
	int16_t _oldValue = 0;
	int32_t _position = 0;
	TIM_HandleTypeDef *_htim = nullptr;

	void reset();
};

#endif // APP_DRIVERS_ENCODER_H
