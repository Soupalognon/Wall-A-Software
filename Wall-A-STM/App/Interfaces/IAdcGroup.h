#ifndef APP_INTERFACES_IADCGROUP_H
#define APP_INTERFACES_IADCGROUP_H

class IAdcGroup {
public:
    virtual void bind() {}       // enregistre la tâche appelante comme cible ISR (no-op par défaut)
    virtual void trigger() = 0;  // démarre la conversion ADC (non-bloquant)
    virtual void wait()    = 0;  // bloque jusqu'à completion via xTaskNotifyWait
    virtual ~IAdcGroup() = default;
};

#endif // APP_INTERFACES_IADCGROUP_H
