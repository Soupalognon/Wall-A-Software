#include "Drivers/InputCapture.h"
#include "Tasks/ExternalComm.h"

InputCapture *InputCapture::s_head = nullptr;

InputCapture::InputCapture(TIM_HandleTypeDef *htim, uint32_t channel) :
	_htim(htim), _channel(channel) {
	switch (channel) {
	case TIM_CHANNEL_1:
		_activeChannel = HAL_TIM_ACTIVE_CHANNEL_1;
		break;
	case TIM_CHANNEL_2:
		_activeChannel = HAL_TIM_ACTIVE_CHANNEL_2;
		break;
	case TIM_CHANNEL_3:
		_activeChannel = HAL_TIM_ACTIVE_CHANNEL_3;
		break;
	case TIM_CHANNEL_4:
		_activeChannel = HAL_TIM_ACTIVE_CHANNEL_4;
		break;
	default:
		_activeChannel = HAL_TIM_ACTIVE_CHANNEL_CLEARED;
		break;
	}

	_next = s_head;
	s_head = this;
}

bool InputCapture::init() {
	HAL_StatusTypeDef rc = HAL_TIM_IC_Start_IT(_htim, _channel);
	if (rc != HAL_OK) {
		ExternalComm::log_error("InputCapture: Error starting channel (status=%d)", rc);
		return false;
	}
	return true;
}

uint32_t InputCapture::getLastPulse() {
	_hasNew = false;
	return _pulseWidth;
}

bool InputCapture::hasNewPulse() const {
	return _hasNew;
}

void InputCapture::onCapture() {
	uint32_t val = HAL_TIM_ReadCapturedValue(_htim, _channel);

	if (!_firstCaptured) {
		_riseTime = val;
		_firstCaptured = true;
	} else {
		uint32_t rise = _riseTime;
		// TIM3 is 16-bit on STM32F4
		_pulseWidth = (val >= rise) ? (val - rise) : (0xFFFF - rise + val + 1);
		_hasNew = true;
		_firstCaptured = false;
	}
}

bool InputCapture::initAll() {
	bool ok = true;
	for (InputCapture *ic = s_head; ic != nullptr; ic = ic->_next)
		ok &= ic->init();
	return ok;
}

void InputCapture::dispatchCallback(TIM_HandleTypeDef *htim) {
	for (InputCapture *ic = s_head; ic != nullptr; ic = ic->_next) {
		if (ic->_htim == htim && htim->Channel == ic->_activeChannel) {
			ic->onCapture();
			return;
		}
	}
}

extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	InputCapture::dispatchCallback(htim);
}
