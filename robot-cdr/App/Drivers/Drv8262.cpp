#include <Drivers/Drv8262.h>
#include "main.h"
#include "Tasks/ExternalComm.h"
#include <algorithm>
#include <cmath>

extern TIM_HandleTypeDef htim1;

void Drv8262::init() {
	ExternalComm::log_info("Drv8262: Initialisation...");

	HAL_StatusTypeDef rc1 = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	if (rc1 != HAL_OK) {
		ExternalComm::log_error("Drv8262: Error starting PWM CH1 (status=%d). Block thread", rc1);
		while (1)
			;
	}

	HAL_StatusTypeDef rc2 = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	if (rc2 != HAL_OK) {
		ExternalComm::log_error("Drv8262: Error starting PWM CH3 (status=%d). Block thread", rc2);
		while (1)
			;
	}

	stop();
	HAL_Delay(100);
	enable(true);
	HAL_Delay(10);

	if (isError()) {
		ExternalComm::log_error("Drv8262: Error on init. Block thread");
		while (1)
			;
	}

	ExternalComm::log_info("Drv8262: Init OK");
}

static inline uint32_t dutyToCCR(float duty) {
	duty = std::max(-1.0f, std::min(1.0f, duty));
	const uint32_t maxCCR = 65535;
	return static_cast<uint32_t>(std::fabs(duty) * maxCCR);
}

static void setLeftDirection(float duty) {
	if (duty > 0.01f)
		HAL_GPIO_WritePin(WHEEL_DIR1_GPIO_Port, WHEEL_DIR1_Pin, GPIO_PIN_SET);
	else if (duty < -0.01f)
		HAL_GPIO_WritePin(WHEEL_DIR1_GPIO_Port, WHEEL_DIR1_Pin, GPIO_PIN_RESET);
}

static void setRightDirection(float duty) {
	if (duty > 0.01f)
		HAL_GPIO_WritePin(WHEEL_DIR2_GPIO_Port, WHEEL_DIR2_Pin, GPIO_PIN_SET);
	else if (duty < -0.01f)
		HAL_GPIO_WritePin(WHEEL_DIR2_GPIO_Port, WHEEL_DIR2_Pin, GPIO_PIN_RESET);
}

void Drv8262::setLeftDuty(float duty) {
	setLeftDirection(duty);
	TIM1->CCR1 = dutyToCCR(duty);
}

void Drv8262::setRightDuty(float duty) {
	setRightDirection(duty);
	TIM1->CCR3 = dutyToCCR(duty);
}

void Drv8262::enable(bool en) {
	GPIO_PinState state = en ? GPIO_PIN_SET : GPIO_PIN_RESET;
	HAL_GPIO_WritePin(PRI_MOTOR_SLEEP_GPIO_Port, PRI_MOTOR_SLEEP_Pin, state);
	ExternalComm::log_info("Drv8262: PRI_MOTOR_SLEEP = %s", en ? "HIGH" : "LOW");
}

void Drv8262::reset() {}

void Drv8262::stop() {
	setLeftDuty(0.0f);
	setRightDuty(0.0f);
}

void Drv8262::setMotors(float leftDuty, float rightDuty) {
	setLeftDuty(leftDuty);
	setRightDuty(rightDuty);
}

bool Drv8262::isError() {
	return !HAL_GPIO_ReadPin(PRI_MOTOR_NFAULT_GPIO_Port, PRI_MOTOR_NFAULT_Pin);
}
