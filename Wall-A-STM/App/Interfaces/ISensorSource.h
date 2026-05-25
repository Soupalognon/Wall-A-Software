#ifndef APP_INTERFACES_ISENSORSOURCE_H
#define APP_INTERFACES_ISENSORSOURCE_H

#include <cstdint>

class ISensorSource {
public:
	virtual void bind() { }              // enregistre la tâche appelante comme cible ISR (no-op par défaut)
	virtual void trigger() = 0;          // démarre l'acquisition (non-bloquant)
	virtual uint32_t doneFlag() const = 0; // flag ISR pour xTaskNotifyFromISR (0 = pas d'attente)
	virtual ~ISensorSource() = default;
};

#endif // APP_INTERFACES_ISENSORSOURCE_H
