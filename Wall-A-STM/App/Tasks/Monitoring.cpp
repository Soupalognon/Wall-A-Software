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
	uint32_t now;

	//─────────────────────────────────────────────────────────────────
	now = HAL_GetTick();
	OdoControl::OdoSnapshot odoSnap;
	taskENTER_CRITICAL();
	odoSnap = OdoControl::latestSnapshot;
	taskEXIT_CRITICAL();

	if (abs(now - odoSnap.timestamp) > Config::MONITORING_STALE_MS) {
		_bus->publish(Topic::ALERT, BusFormat::altStale("ODO_CONTROL"));
	}
	if (odoSnap.motorError) {
		_bus->publish(Topic::ALERT, "Motor Error!");
	}

	_bus->publish(Topic::TELEMETRY, BusFormat::telOdoVelocity(now, odoSnap.v, odoSnap.w));
	_bus->publish(Topic::TELEMETRY,
		BusFormat::telOdoPose(now, odoSnap.x, odoSnap.y, odoSnap.angle));
	_bus->publish(Topic::TELEMETRY,
		BusFormat::telOdoWheelSpeed(now, odoSnap.vLeft, odoSnap.vRight));
	_bus->publish(Topic::TELEMETRY,
		BusFormat::telOdoMotorVoltage(now, odoSnap.voltLeft, odoSnap.voltRight));

	//─────────────────────────────────────────────────────────────────
	now = HAL_GetTick();
	SensorManager::SensorSnapshot sensorSnap;
	taskENTER_CRITICAL();
	sensorSnap = SensorManager::latestSnapshot;
	taskEXIT_CRITICAL();

	if (abs(now - sensorSnap.timestamp) > Config::MONITORING_STALE_MS) {
		_bus->publish(Topic::ALERT, BusFormat::altStale("SENSORS"));
	}
	if (sensorSnap.alarmMask != 0) {
		for (uint8_t i = 0; i < Config::MAX_SENSORS; i++) {
			if (sensorSnap.alarmMask & (1u << i)) {
				_bus->publish(Topic::ALERT, BusFormat::altSensorAlarm(SensorManager::sensorNames[i], sensorSnap.values[i]));
			}
		}
	}

	_bus->publish(Topic::HEALTH, BusFormat::hltSensors(sensorSnap.count, sensorSnap.alarmMask));
	for (uint8_t i = 0; i < sensorSnap.count && i < Config::MAX_SENSORS; i++) {
		_bus->publish(Topic::HEALTH,
			BusFormat::hltSensorValue(SensorManager::sensorNames[i], sensorSnap.values[i]));
	}
}
