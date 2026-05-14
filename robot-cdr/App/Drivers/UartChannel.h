#ifndef APP_DRIVERS_UARTCHANNEL_H
#define APP_DRIVERS_UARTCHANNEL_H

#include "Interfaces/ICommChannel.h"
#include "main.h"

class UartChannel : public ICommChannel {
public:
    explicit UartChannel(UART_HandleTypeDef* h);

    void     transmit(const char* data, uint16_t len) override;
    uint16_t receive(char* buf, uint16_t maxLen, uint32_t timeoutMs) override;

    UART_HandleTypeDef* getInstance() { return _huart; };

private:
    UART_HandleTypeDef* _huart;
};

#endif // APP_DRIVERS_UARTCHANNEL_H
