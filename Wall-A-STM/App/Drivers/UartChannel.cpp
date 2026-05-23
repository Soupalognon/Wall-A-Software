#include "Drivers/UartChannel.h"
#include <FreeRTOS.h>
#include <task.h>

UartChannel *UartChannel::_instances[MAX_INSTANCES] = { };
size_t UartChannel::_instanceCount = 0;

UartChannel::UartChannel(UART_HandleTypeDef *h) :
	_huart(h) {
	if (_instanceCount < MAX_INSTANCES)
		_instances[_instanceCount++] = this;
}

HAL_StatusTypeDef UartChannel::transmit(const char *data, uint16_t len) {
	taskENTER_CRITICAL();
	for (uint16_t i = 0; i < len; i++) {
		size_t next = (_txHead + 1) % TX_BUF_SIZE;
		if (next == _txTail)
			break;
		_txRingBuf[_txHead] = (uint8_t) data[i];
		_txHead = next;
	}
	_pumpTx();
	taskEXIT_CRITICAL();
	return HAL_OK;
}

void UartChannel::_pumpTx() {
	if (_txBusy || _txHead == _txTail)
		return;
	size_t len = 0;
	size_t tail = _txTail;
	while (tail != _txHead && len < TX_BUF_SIZE) {
		_txStagingBuf[len++] = _txRingBuf[tail];
		tail = (tail + 1) % TX_BUF_SIZE;
	}
	_txBusy = true;
	if (HAL_UART_Transmit_IT(_huart, _txStagingBuf, (uint16_t) len) == HAL_OK) {
		_txTail = tail;
	} else {
		_txBusy = false;
	}
}

void UartChannel::onTxComplete(UART_HandleTypeDef *huart) {
	for (size_t i = 0; i < _instanceCount; i++) {
		if (_instances[i]->_huart == huart) {
			_instances[i]->_txBusy = false;
			_instances[i]->_pumpTx();
			return;
		}
	}
}

void UartChannel::startReceive(QueueHandle_t rxQueue) {
	_rxQueue = rxQueue;
	HAL_UART_Receive_IT(_huart, _rxIsrBuf, 1);
}

void UartChannel::onRxComplete(UART_HandleTypeDef *huart) {
	for (size_t i = 0; i < _instanceCount; i++) {
		UartChannel *ch = _instances[i];
		if (ch->_huart == huart && ch->_rxQueue) {
			BaseType_t woken = pdFALSE;
			xQueueSendFromISR(ch->_rxQueue, ch->_rxIsrBuf, &woken);
			HAL_UART_Receive_IT(huart, ch->_rxIsrBuf, 1);
			portYIELD_FROM_ISR(woken);
			return;
		}
	}
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	UartChannel::onTxComplete(huart);
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	UartChannel::onRxComplete(huart);
}

