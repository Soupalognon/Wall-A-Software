#include <Drivers/Encoder.h>
#include "main.h"
#include "Tasks/ExternalComm.h"

bool Encoder::init() {
	HAL_StatusTypeDef rc1 = HAL_TIM_Encoder_Start(_htim, TIM_CHANNEL_ALL);
	if (rc1 != HAL_OK) {
		ExternalComm::log_error("Encoder: Error starting Encoder (status=%d)", rc1);
		return 1;
	}

	reset();

	return 0;
}

int32_t Encoder::getTicks() {
	int16_t current = (int16_t) __HAL_TIM_GET_COUNTER(_htim);
	int16_t delta = current - _oldValue;
	_oldValue = current;
	_position += delta;
	return _position;
}

void Encoder::reset() {
	_oldValue = (int16_t) __HAL_TIM_GET_COUNTER(_htim);
	_position = 0;
}
