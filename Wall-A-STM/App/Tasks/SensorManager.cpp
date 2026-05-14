#include "Tasks/SensorManager.h"
#include "Services/BusFormat.h"
#include "stm32f4xx_hal.h"

SensorManager::SensorSnapshot SensorManager::latestSnapshot{};

SensorManager::SensorManager(ISensor** sensors, uint8_t sensorCount,
                             TaskHandle_t motionPlannerHandle, IBus* bus)
    : _sensors(sensors)
    , _sensorCount(sensorCount)
    , _motionPlannerHandle(motionPlannerHandle)
    , _bus(bus)
{}

void SensorManager::task(void* param) {
    auto* self = static_cast<SensorManager*>(param);
    for (;;) {
        self->pollOnce();
        vTaskDelay(pdMS_TO_TICKS(Config::SENSOR_POLL_MS));
    }
}

void SensorManager::pollOnce() {
    uint32_t alarmMask = 0;

    for (uint8_t i = 0; i < _sensorCount && i < Config::MAX_SENSORS; ++i) {
        if (_sensors[i] == nullptr) continue;

        float value = _sensors[i]->read();
        bool  alarm = _sensors[i]->isAlarm();

        latestSnapshot.values[i] = value;
        latestSnapshot.alarms[i] = alarm;

        if (alarm) {
            alarmMask |= (1u << i);
        }
    }

    latestSnapshot.count     = _sensorCount;
    latestSnapshot.timestamp = HAL_GetTick();

    if (alarmMask != 0) {
        xTaskNotify(_motionPlannerHandle, alarmMask, eSetBits);
    }

    _bus->publish(Topic::HEALTH, BusFormat::hltSensors(_sensorCount, alarmMask));
}
