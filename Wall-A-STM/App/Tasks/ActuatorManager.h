#ifndef APP_TASKS_ACTUATORMANAGER_H
#define APP_TASKS_ACTUATORMANAGER_H

#include "Interfaces/IActuatorManager.h"
#include "Interfaces/IActuator.h"
#include "Interfaces/IBus.h"
#include "Config.h"
#include <cstdint>

class ActuatorManager : public IActuatorManager {
public:
    ActuatorManager(IActuator** actuators, uint8_t actuatorCount, IBus* bus);
    void commandById(uint8_t id, const char* cmd) override;
    void setBus(IBus* bus) { _bus = bus; }

private:
    IActuator** _actuators;
    uint8_t     _actuatorCount;
    IBus*       _bus;
};

#endif // APP_TASKS_ACTUATORMANAGER_H
