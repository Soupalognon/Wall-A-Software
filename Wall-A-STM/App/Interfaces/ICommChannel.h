#ifndef APP_INTERFACES_ICOMMCHANNEL_H
#define APP_INTERFACES_ICOMMCHANNEL_H

#include <cstdint>
#include "stm32f4xx_hal.h"
#include <FreeRTOS.h>
#include <queue.h>

class ICommChannel {
public:
	virtual HAL_StatusTypeDef transmit(const char *data, uint16_t len) = 0;
	virtual void startReceive(QueueHandle_t) {}
	virtual ~ICommChannel() = default;
};

#endif // APP_INTERFACES_ICOMMCHANNEL_H
