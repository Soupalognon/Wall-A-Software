#include "Tasks/Monitoring.h"
#include "Tasks/MotionPlanner.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"

Monitoring::Monitoring(IBus *bus) :
	_bus(bus) {
}

void Monitoring::task(void *param) {
	auto *self = static_cast<Monitoring*>(param);

	ExternalComm::log_info("Monitoring: Init OK");
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000 / Config::MONITORING_FREQ_HZ));
		self->checkOnce();
	}
}

void Monitoring::checkOnce() {
	uint32_t now = HAL_GetTick();

	OdoControl::OdoSnapshot odoSnap;
	taskENTER_CRITICAL();
	odoSnap = OdoControl::latestSnapshot;
	taskEXIT_CRITICAL();

	if (abs(now - odoSnap.timestamp) > Config::MONITORING_STALE_MS) {
		_bus->publish(Topic::ALERT, BusFormat::altStale("ODO"));
	}
	if (odoSnap.motorError) {
		_bus->publish(Topic::ALERT, "Motor Error!");
	}

	SensorManager::SensorSnapshot sensorSnap;
	taskENTER_CRITICAL();
	sensorSnap = SensorManager::latestSnapshot;
	taskEXIT_CRITICAL();

	ExternalComm::log_info(
		"Temperatures [C] - Pri motor driver: %.1f, Sec motor driver: %.1f, Power supplies: %.1f",
		sensorSnap[SensorType::PrimaryMotorTemp], sensorSnap[SensorType::SecondaryMotorTemp],
		sensorSnap[SensorType::PowerSupplyTemp]);

	_bus->publish(Topic::TELEMETRY, BusFormat::telOdoVelocity(now, odoSnap.v, odoSnap.w));
	_bus->publish(Topic::TELEMETRY,
		BusFormat::telOdoPose(now, odoSnap.x, odoSnap.y, odoSnap.angle));
	_bus->publish(Topic::TELEMETRY,
		BusFormat::telOdoWheelSpeed(now, odoSnap.vLeft, odoSnap.vRight));
	_bus->publish(Topic::TELEMETRY,
		BusFormat::telOdoMotorVoltage(now, odoSnap.voltLeft, odoSnap.voltRight));
}
