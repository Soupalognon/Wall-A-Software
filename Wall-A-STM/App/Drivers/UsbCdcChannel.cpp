#include "Drivers/UsbCdcChannel.h"
#include <stm32f4xx_hal.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

UsbCdcChannel* UsbCdcChannel::_instance = nullptr;

HAL_StatusTypeDef UsbCdcChannel::transmit(const char *data, uint16_t len) {
	taskENTER_CRITICAL();
	for (uint16_t i = 0; i < len; i++) {
		size_t next = (_txHead + 1) % TX_BUF_SIZE;
		if (next == _txTail) break;
		_txRingBuf[_txHead] = (uint8_t)data[i];
		_txHead = next;
	}
	_pumpTx();
	taskEXIT_CRITICAL();
	return HAL_OK;
}

void UsbCdcChannel::_pumpTx() {
	if (_txBusy || _txHead == _txTail) return;
	size_t len = 0;
	size_t tail = _txTail;
	while (tail != _txHead && len < TX_BUF_SIZE) {
		_txStagingBuf[len++] = _txRingBuf[tail];
		tail = (tail + 1) % TX_BUF_SIZE;
	}
	_txBusy = true;
	if (CDC_Transmit_FS(_txStagingBuf, (uint16_t)len) == USBD_OK) {
		_txTail = tail;  // avance seulement si le transfert a démarré
	} else {
		_txBusy = false; // permet un retry au prochain transmit()
	}
}

void UsbCdcChannel::onTxComplete() {
	if (!_instance) return;
	_instance->_txBusy = false;
	_instance->_pumpTx();
}

void UsbCdcChannel::onRxData(uint8_t *buf, uint32_t len) {
	if (!_instance || !_instance->_rxQueue) return;
	BaseType_t woken = pdFALSE;
	for (uint32_t i = 0; i < len; i++)
		xQueueSendFromISR(_instance->_rxQueue, &buf[i], &woken);
	portYIELD_FROM_ISR(woken);
}


extern "C" void USB_CDC_RxHandler(uint8_t *Buf, uint32_t Len) {
	UsbCdcChannel::onRxData(Buf, Len);
}

extern "C" void UsbCdcChannel_onTxComplete() {
	UsbCdcChannel::onTxComplete();
}
