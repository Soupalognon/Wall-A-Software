#ifndef APP_DRIVERS_DRV8262_H
#define APP_DRIVERS_DRV8262_H

#pragma once
#include <cstdint>
#include "Interfaces/IMotorHAL.h"

class Drv8262 : public IMotorHAL {
public:
	Drv8262() = default;

	bool begin();
	void setLeftDuty(float duty);
	void setRightDuty(float duty);
	void enable(bool en);
	void reset();
	void stop();
	void setMotors(float leftDuty, float rightDuty);
	bool isError();

private:
//	typedef enum {
//		PRIMARY_MOTOR = 0, SECONDARY_MOTOR = 1, POWER_SUPPLIES = 2,
//	} channel;
};

#endif // APP_DRIVERS_DRV8262_H
