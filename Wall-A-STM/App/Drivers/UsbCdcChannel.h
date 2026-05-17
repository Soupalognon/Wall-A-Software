#ifndef APP_DRIVERS_USBCDCCHANNEL_H
#define APP_DRIVERS_USBCDCCHANNEL_H

#include "Interfaces/ICommChannel.h"
#include "usbd_cdc_if.h"

class UsbCdcChannel: public ICommChannel {
public:
	UsbCdcChannel(USBD_HandleTypeDef* hUsbDeviceFS): _usb(hUsbDeviceFS) {

	}
	HAL_StatusTypeDef transmit(const char *data, uint16_t len) override;
	uint16_t receive(char *buf, uint16_t maxLen, uint32_t timeoutMs) override;

private:
	USBD_HandleTypeDef *_usb;
};

#endif // APP_DRIVERS_USBCDCCHANNEL_H
