#ifndef APP_DRIVERS_UARTCHANNEL_H
#define APP_DRIVERS_UARTCHANNEL_H

#include "main.h"
#include <FreeRTOS.h>
#include <Interfaces/ICom.h>
#include <queue.h>
#include <cstddef>

class UartChannel: public ICommChannel {
public:
	explicit UartChannel(UART_HandleTypeDef *h);

	HAL_StatusTypeDef transmit(const char *data, uint16_t len) override;

	UART_HandleTypeDef* getInstance() {
		return _huart;
	}

	void startReceive(QueueHandle_t rxQueue) override;

	static void onTxComplete(UART_HandleTypeDef *huart);
	static void onRxComplete(UART_HandleTypeDef *huart);

private:
	static constexpr size_t TX_BUF_SIZE = 512;
	static constexpr size_t MAX_INSTANCES = 4;

	UART_HandleTypeDef *_huart;

	QueueHandle_t _rxQueue = nullptr;
	uint8_t _rxIsrBuf[1] = { };

	uint8_t _txRingBuf[TX_BUF_SIZE];
	uint8_t _txStagingBuf[TX_BUF_SIZE];
	volatile size_t _txHead = 0;
	volatile size_t _txTail = 0;
	volatile bool _txBusy = false;

	void _pumpTx();

	static UartChannel *_instances[MAX_INSTANCES];
	static size_t _instanceCount;
};

#endif // APP_DRIVERS_UARTCHANNEL_H
