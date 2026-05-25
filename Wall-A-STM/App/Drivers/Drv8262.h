#ifndef APP_DRIVERS_DRV8262_H
#define APP_DRIVERS_DRV8262_H

#pragma once
#include <cstdint>
#include "stm32f4xx_hal.h"
#include "Interfaces/IMotorHAL.h"

class Drv8262: public IMotorHAL {
public:
	explicit Drv8262(TIM_HandleTypeDef *htim) : _htim(htim) {};

	bool begin();
	void setLeftDuty(float duty);
	void setRightDuty(float duty);
	void enable(bool en);
	void reset();
	void stop();
	void setMotors(float leftDuty, float rightDuty);
	bool isError();

private:
	TIM_HandleTypeDef *_htim;
};

#endif // APP_DRIVERS_DRV8262_H
