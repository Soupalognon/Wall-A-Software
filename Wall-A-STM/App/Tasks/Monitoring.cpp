#include "Tasks/Monitoring.h"

Monitoring::Monitoring(IBus *bus, InternalTemperature *internalTemp) :
	_bus(bus), _internalTemperatures(internalTemp) {
}

void Monitoring::task(void *param) {
	auto *self = static_cast<Monitoring*>(param);

	self->_internalTemperatures->setNotifyThread(xTaskGetCurrentTaskHandle());

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000 / Config::MONITORING_POLL_HZ));
		self->checkOnce();
	}
}

void Monitoring::checkOnce() {
	uint32_t now = HAL_GetTick();

	_internalTemperatures->startConversion();
	BaseType_t notified = xTaskNotifyWait(0, InternalTemperature::ADC_DONE_FLAG, nullptr, pdMS_TO_TICKS(100));

	if (notified == pdTRUE) {
		float t0 = _internalTemperatures->getTemperature(InternalTemperature::PRIMARY_MOTOR);
		float t1 = _internalTemperatures->getTemperature(InternalTemperature::SECONDARY_MOTOR);
		float t2 = _internalTemperatures->getTemperature(InternalTemperature::POWER_SUPPLIES);
		ExternalComm::log_info(
			"Temperatures [C] - Pri motor driver: %.1f  Sec motor driver: %.1f  Power supplies: %.1f",
			t0, t1, t2);
	}

	OdoControl::OdoSnapshot odoSnap;
	taskENTER_CRITICAL();
	odoSnap = OdoControl::latestSnapshot;
	taskEXIT_CRITICAL();
	if ((now - odoSnap.timestamp) > Config::MONITORING_STALE_MS) {
		_bus->publish(Topic::ALERT, BusFormat::altStale("ODO"));
	}

	ExternalComm::log_info(
		"x:%.2f, y:%.2f, angle:%.2f, vLeft:%.2f, vRight:%.2f, v:%.2f, w:%.2f, motorError:%d",
		odoSnap.x, odoSnap.y, odoSnap.angle, odoSnap.vLeft, odoSnap.vRight, odoSnap.v, odoSnap.w,
		odoSnap.motorError);

//    SensorManager::SensorSnapshot sensorSnap;
//    taskENTER_CRITICAL();
//    sensorSnap = SensorManager::latestSnapshot;
//    taskEXIT_CRITICAL();
//    if (now - sensorSnap.timestamp > Config::MONITORING_STALE_MS)
//        _bus->publish(Topic::ALERT, BusFormat::altStale("SENSOR"));
//
//    ExternalComm::CommSnapshot commSnap;
//    taskENTER_CRITICAL();
//    commSnap = ExternalComm::latestSnapshot;
//    taskEXIT_CRITICAL();
//    if (now - commSnap.timestamp > Config::MONITORING_STALE_MS)
//        _bus->publish(Topic::ALERT, BusFormat::altStale("COMM"));
}

void Monitoring::getMotorsCurrentConsumptions() {

}
