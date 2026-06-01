#ifndef APP_INTERFACES_IADCHAL_H
#define APP_INTERFACES_IADCHAL_H

#include <cstdint>

class IAdcHAL {
public:
	virtual ~IAdcHAL() = default;

	virtual void bind(uint32_t doneFlag) = 0; // registers the current task + the ISR notification flag
	virtual void start() = 0;              // starts a conversion (non-blocking)
	virtual uint16_t rawValue() = 0;       // last raw converted value
	virtual bool isActive() const = 0;     // true while the conversion is in progress
	virtual uint32_t doneFlag() const = 0; // xTaskNotifyFromISR flag (0 = no wait)
};

#endif // APP_INTERFACES_IADCHAL_H
