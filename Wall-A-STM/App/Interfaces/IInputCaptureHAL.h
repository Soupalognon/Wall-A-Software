#ifndef APP_INTERFACES_IINPUTCAPTUREHAL_H
#define APP_INTERFACES_IINPUTCAPTUREHAL_H

#include <cstdint>

class IInputCaptureHAL {
public:
	virtual ~IInputCaptureHAL() = default;
	virtual bool init() = 0;
	virtual uint32_t getLastPulse() = 0;   // µs (PSC=83 @ 84 MHz)
	virtual bool hasNewPulse() const = 0;
};

#endif // APP_INTERFACES_IINPUTCAPTUREHAL_H
