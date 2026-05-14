#ifndef APP_TASKS_SENSORMANAGER_H
#define APP_TASKS_SENSORMANAGER_H

#include <cstdint>

// Stub - full implementation in Story 3.1
class SensorManager {
public:
    struct SensorSnapshot {
        float values[15]{};
        uint8_t count{};
        uint32_t timestamp{};
    };
    static SensorSnapshot latestSnapshot;
};

#endif // APP_TASKS_SENSORMANAGER_H
