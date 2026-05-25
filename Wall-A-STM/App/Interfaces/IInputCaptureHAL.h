#ifndef APP_INTERFACES_IINPUTCAPTUREHAL_H
#define APP_INTERFACES_IINPUTCAPTUREHAL_H

#include <cstdint>
#include <stm32f4xx_hal.h>

class IInputCaptureHAL {
public:
	virtual ~IInputCaptureHAL() = default;
	virtual bool init() = 0;
	virtual uint32_t getLastPulse(uint32_t channel) const = 0;
	virtual bool hasNewPulse(uint32_t channel) const = 0;
};

#endif // APP_INTERFACES_IINPUTCAPTUREHAL_H
