#include "Drivers/UartChannel.h"

UartChannel::UartChannel(UART_HandleTypeDef *h) :
	_huart(h) {
}

HAL_StatusTypeDef UartChannel::transmit(const char *data, uint16_t len) {
	return HAL_UART_Transmit(_huart, reinterpret_cast<const uint8_t*>(data), len, 100);
}

uint16_t UartChannel::receive(char *buf, uint16_t maxLen, uint32_t /*timeoutMs*/) {
	uint16_t n = 0;
	while (n < maxLen - 1) {
		uint8_t b;
		if (HAL_UART_Receive(_huart, &b, 1, 0) != HAL_OK)
			break;
		buf[n++] = static_cast<char>(b);
		if (b == '\n')
			break;
	}
	if (n > 0)
		buf[n] = '\0';
	return n;
}
