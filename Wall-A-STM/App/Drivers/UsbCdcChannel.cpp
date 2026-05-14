#include "Drivers/UsbCdcChannel.h"

void UsbCdcChannel::transmit(const char* data, uint16_t len) {
    CDC_Transmit_FS(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), len);
}

uint16_t UsbCdcChannel::receive(char* /*buf*/, uint16_t /*maxLen*/, uint32_t /*timeoutMs*/) {
    return 0;
}
