#ifndef APP_DRIVERS_USBCDCCHANNEL_H
#define APP_DRIVERS_USBCDCCHANNEL_H

#include "Interfaces/ICommChannel.h"
#include "usbd_cdc_if.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <cstddef>

class UsbCdcChannel: public ICommChannel {
public:
	UsbCdcChannel(USBD_HandleTypeDef* hUsbDeviceFS): _usb(hUsbDeviceFS) {
		_instance = this;
	}
	HAL_StatusTypeDef transmit(const char *data, uint16_t len) override;


	void startReceive(QueueHandle_t rxQueue) override { _rxQueue = rxQueue; }

	static void onTxComplete();
	static void onRxData(uint8_t *buf, uint32_t len);

private:
	static constexpr size_t TX_BUF_SIZE = 2048;

	USBD_HandleTypeDef *_usb;
	QueueHandle_t _rxQueue = nullptr;

	uint8_t _txRingBuf[TX_BUF_SIZE];
	uint8_t _txStagingBuf[TX_BUF_SIZE];
	volatile size_t _txHead = 0;
	volatile size_t _txTail = 0;
	volatile bool _txBusy = false;

	void _pumpTx();

	static UsbCdcChannel* _instance;
};

#endif // APP_DRIVERS_USBCDCCHANNEL_H
