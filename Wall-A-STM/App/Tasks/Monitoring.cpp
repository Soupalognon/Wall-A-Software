#include "Tasks/Monitoring.h"

Monitoring::Monitoring(IBus* bus) : _bus(bus) {}

void Monitoring::task(void* param) {
    auto* self = static_cast<Monitoring*>(param);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(Config::MONITORING_POLL_MS));
        self->checkOnce();
    }
}

void Monitoring::checkOnce() {
    uint32_t now = HAL_GetTick();

    OdoControl::OdoSnapshot odoSnap;
    taskENTER_CRITICAL();
    odoSnap = OdoControl::latestSnapshot;
    taskEXIT_CRITICAL();
    if (now - odoSnap.timestamp > Config::MONITORING_STALE_MS)
        _bus->publish(Topic::ALERT, BusFormat::altStale("ODO"));

    SensorManager::SensorSnapshot sensorSnap;
    taskENTER_CRITICAL();
    sensorSnap = SensorManager::latestSnapshot;
    taskEXIT_CRITICAL();
    if (now - sensorSnap.timestamp > Config::MONITORING_STALE_MS)
        _bus->publish(Topic::ALERT, BusFormat::altStale("SENSOR"));

    ExternalComm::CommSnapshot commSnap;
    taskENTER_CRITICAL();
    commSnap = ExternalComm::latestSnapshot;
    taskEXIT_CRITICAL();
    if (now - commSnap.timestamp > Config::MONITORING_STALE_MS)
        _bus->publish(Topic::ALERT, BusFormat::altStale("COMM"));
}
