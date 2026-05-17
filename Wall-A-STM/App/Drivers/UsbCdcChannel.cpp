#include "Drivers/UsbCdcChannel.h"

HAL_StatusTypeDef UsbCdcChannel::transmit(const char *data, uint16_t len) {
	USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*) _usb->pClassData;
	if (hcdc->TxState == 0) {
		uint8_t status = CDC_Transmit_FS(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), len);
		if (status == USBD_OK)
			return HAL_OK;

		while (((USBD_CDC_HandleTypeDef*) _usb->pClassData)->TxState != 0) {
		}
		return HAL_ERROR;
	}
	return HAL_BUSY;
}

uint16_t UsbCdcChannel::receive(char* /*buf*/, uint16_t /*maxLen*/, uint32_t /*timeoutMs*/) {
	return 0;
}
