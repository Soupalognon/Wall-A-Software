#ifndef APP_INTERFACES_ISENSOR2_H
#define APP_INTERFACES_ISENSOR2_H

#include <cstdint>

class ISensor {
public:
	virtual uint8_t id() const = 0;
	virtual const char* name() const = 0;
	virtual float read() = 0;
	virtual bool isAlarm() = 0;

	virtual void bind() { }
	virtual void trigger() = 0;          // démarre l'acquisition (non-bloquant)
	virtual uint32_t doneFlag() const = 0; // flag ISR pour xTaskNotifyFromISR (0 = pas d'attente)
	virtual bool isActive() const = 0;

	virtual ~ISensor() = default;
};

#endif // APP_INTERFACES_ISENSOR2_H
