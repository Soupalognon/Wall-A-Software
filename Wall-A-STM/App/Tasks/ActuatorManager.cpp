#include "Tasks/ActuatorManager.h"
#include "Services/BusFormat.h"
#include "Interfaces/IBus.h"

ActuatorManager::ActuatorManager(IActuator** actuators, uint8_t actuatorCount, IBus* bus)
    : _actuators(actuators), _actuatorCount(actuatorCount), _bus(bus) {}

void ActuatorManager::commandById(uint8_t id, const char* cmd) {
    for (uint8_t i = 0; i < _actuatorCount && i < Config::MAX_ACTUATORS; ++i) {
        if (_actuators[i] == nullptr) continue;
        if (_actuators[i]->id() == id) {
            _actuators[i]->command(cmd);
            return;
        }
    }
    _bus->publish(Topic::LOG, BusFormat::logWarn("unknown actuator id"));
}
