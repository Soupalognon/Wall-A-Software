#ifndef APP_DRIVERS_USBCDCCHANNEL_H
#define APP_DRIVERS_USBCDCCHANNEL_H

#include "Interfaces/ICommChannel.h"
#include "usbd_cdc_if.h"

class UsbCdcChannel : public ICommChannel {
public:
    void     transmit(const char* data, uint16_t len) override;
    uint16_t receive(char* buf, uint16_t maxLen, uint32_t timeoutMs) override;
};

#endif // APP_DRIVERS_USBCDCCHANNEL_H
