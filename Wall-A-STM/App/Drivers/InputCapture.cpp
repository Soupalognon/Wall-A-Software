#include "Drivers/InputCapture.h"
#include "main.h"
#include "Tasks/ExternalComm.h"

InputCapture *InputCapture::_instances = nullptr;

InputCapture::InputCapture(TIM_HandleTypeDef *htim) : _htim(htim) {
	_instances = this;
}

bool InputCapture::init() {
	const uint32_t channels[CHANNEL_SIZE] = {
	TIM_CHANNEL_1, TIM_CHANNEL_3, TIM_CHANNEL_2, TIM_CHANNEL_4 };

	for (int i = 0; i < CHANNEL_SIZE; i++) {
		HAL_StatusTypeDef rc = HAL_TIM_IC_Start_IT(_htim, channels[i]);
		if (rc != HAL_OK) {
			ExternalComm::log_error("InputCapture: Error starting CH%d (status=%d)", i + 1, rc);
			return false;
		}
	}

//	ExternalComm::log_info("Input Capture started");

	return true;
}

uint32_t InputCapture::getLastPulse(uint8_t channel) {
	_hasNew[channel] = false;
	return _pulseWidth[channel];
}

bool InputCapture::hasNewPulse(uint8_t channel) const {
	return _hasNew[channel];
}

void InputCapture::handleChannel(int idx, uint32_t channel) {
	uint32_t val = HAL_TIM_ReadCapturedValue(_htim, channel);

	if (!_firstCaptured[idx]) {
		_riseTime[idx] = val;
		_firstCaptured[idx] = true;
	} else {
		uint32_t rise = _riseTime[idx];
		// TIM3 est 16-bit sur STM32F4
		_pulseWidth[idx] = (val >= rise) ? (val - rise) : (0xFFFF - rise + val + 1);
		_hasNew[idx] = true;
//		HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
		_firstCaptured[idx] = false;
	}
}

void InputCapture::dispatchCallback(TIM_HandleTypeDef *htim) {
	InputCapture *ic = _instances;
	if (ic->_htim->Instance != htim->Instance)
		return;
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		ic->handleChannel(0, TIM_CHANNEL_1);
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
		ic->handleChannel(2, TIM_CHANNEL_2);
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
		ic->handleChannel(1, TIM_CHANNEL_3);
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		ic->handleChannel(3, TIM_CHANNEL_4);
}

extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	InputCapture::dispatchCallback(htim);
}
