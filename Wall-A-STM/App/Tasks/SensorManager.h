#ifndef APP_TASKS_SENSORMANAGER_H
#define APP_TASKS_SENSORMANAGER_H

#include "Interfaces/ISensor.h"
#include "Interfaces/IBus.h"
#include "Config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <cstdint>

class SensorManager {
public:
    struct SensorSnapshot {
        float    values[Config::MAX_SENSORS];
        bool     alarms[Config::MAX_SENSORS];
        uint8_t  count;
        uint32_t timestamp;
    };
    static SensorSnapshot latestSnapshot;

    SensorManager(ISensor** sensors, uint8_t sensorCount,
                  TaskHandle_t motionPlannerHandle, IBus* bus);

    static void task(void* param);
    void pollOnce();

private:
    ISensor**    _sensors;
    uint8_t      _sensorCount;
    TaskHandle_t _motionPlannerHandle;
    IBus*        _bus;
};

#endif // APP_TASKS_SENSORMANAGER_H
