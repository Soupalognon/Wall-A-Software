#include "Drivers/InputCapture.h"
#include "main.h"
#include "Tasks/ExternalComm.h"

InputCapture *InputCapture::_instances[2] = {};
int           InputCapture::_instanceCount = 0;

bool InputCapture::init()
{
	const uint32_t channels[4] = {
		TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
	};

	for (int i = 0; i < 4; i++) {
		HAL_StatusTypeDef rc = HAL_TIM_IC_Start_IT(_htim, channels[i]);
		if (rc != HAL_OK) {
			ExternalComm::log_error("InputCapture: Error starting CH%d (status=%d)", i + 1, rc);
			return false;
		}
	}

	if (_instanceCount < 2)
		_instances[_instanceCount++] = this;

	return true;
}

uint32_t InputCapture::getLastPulse(uint32_t channel) const
{
	// Convertit TIM_CHANNEL_x (0, 4, 8, 12) en index 0..3
	return _pulseWidth[channel / 4];
}

bool InputCapture::hasNewPulse(uint32_t channel) const
{
	return _hasNew[channel / 4];
}

void InputCapture::handleChannel(int idx, uint32_t channel)
{
	uint32_t val = HAL_TIM_ReadCapturedValue(_htim, channel);

	if (!_firstCaptured[idx]) {
		_riseTime[idx]      = val;
		_firstCaptured[idx] = true;
	} else {
		uint32_t rise = _riseTime[idx];
		// TIM3 est 16-bit sur STM32F4
		_pulseWidth[idx]    = (val >= rise) ? (val - rise) : (0xFFFF - rise + val + 1);
		_hasNew[idx]        = true;
		_firstCaptured[idx] = false;
	}
}

void InputCapture::dispatchCallback(TIM_HandleTypeDef *htim)
{
	for (int i = 0; i < _instanceCount; i++) {
		InputCapture *ic = _instances[i];
		if (ic->_htim->Instance != htim->Instance)
			continue;

		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) ic->handleChannel(0, TIM_CHANNEL_1);
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) ic->handleChannel(1, TIM_CHANNEL_2);
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) ic->handleChannel(2, TIM_CHANNEL_3);
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) ic->handleChannel(3, TIM_CHANNEL_4);
	}
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	InputCapture::dispatchCallback(htim);
}
