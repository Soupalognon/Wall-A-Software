#ifndef APP_INTERFACES_IADCGROUP_H
#define APP_INTERFACES_IADCGROUP_H

#include <cstdint>

class IAdcGroup {
public:
	virtual void bind() {
	}                                    // enregistre la tâche appelante comme cible ISR (no-op par défaut)
	virtual void trigger() = 0;          // démarre la conversion ADC (non-bloquant)
	virtual uint32_t doneFlag() const = 0; // flag utilisé par l'ISR dans xTaskNotifyFromISR
	virtual ~IAdcGroup() = default;
};

#endif // APP_INTERFACES_IADCGROUP_H
