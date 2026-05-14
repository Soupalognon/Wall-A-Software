#ifndef APP_TASKS_STUBACTUATORMANAGER_H
#define APP_TASKS_STUBACTUATORMANAGER_H

#include "Interfaces/IActuatorManager.h"

class StubActuatorManager : public IActuatorManager {
public:
    void commandById(uint8_t /*id*/, const char* /*cmd*/) override {}
};

#endif // APP_TASKS_STUBACTUATORMANAGER_H
