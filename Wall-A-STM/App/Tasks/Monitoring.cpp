#include "Tasks/Monitoring.h"
#include "Tasks/MotionPlanner.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"

Monitoring::Monitoring(IBus *bus, IKernelHAL *kernel) :
	_bus(bus), _kernel(kernel) {
}

void Monitoring::task(void *param) {
	auto *self = static_cast<Monitoring*>(param);

	ExternalComm::log_info("Monitoring: Init OK");
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000 / Config::TASK_MONITORING_FREQ_HZ));
		self->checkOnce();
	}
}

void Monitoring::checkOnce() {
	uint32_t now;

	//-----------------------------------------------------------------
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

	//-----------------------------------------------------------------
	now = HAL_GetTick();
	SensorManager::SensorSnapshot sensorSnap;
	taskENTER_CRITICAL();
	sensorSnap = SensorManager::latestSnapshot;
	taskEXIT_CRITICAL();

//	uint32_t lastSensorUpdate = 0;
//	for (uint8_t i = 0; i < Config::MAX_SENSORS; i++) {
//		if (sensorSnap.timestamps[i] > lastSensorUpdate)
//			lastSensorUpdate = sensorSnap.timestamps[i];
//	}
//	if (abs(now - lastSensorUpdate) > Config::MONITORING_STALE_MS) {
//		_bus->publish(Topic::ALERT, BusFormat::altStale("SENSORS"));
//	}

	_bus->publish(Topic::HEALTH,
		BusFormat::hltSensors(now, sensorSnap.count, sensorSnap.alarmMask));
	for (uint8_t i = 0; i < Config::MAX_SENSORS; i++) {
		if (sensorSnap.timestamps[i] == _lastSensorTs[i])
			continue;
		_lastSensorTs[i] = sensorSnap.timestamps[i];

		if (sensorSnap.alarmMask & (1u << i)) {
			_bus->publish(Topic::ALERT,
				BusFormat::altSensorAlarm(sensorSnap.timestamps[i], SensorManager::sensorNames[i],
					sensorSnap.values[i]));
		}
		_bus->publish(Topic::HEALTH,
			BusFormat::hltSensorValue(sensorSnap.timestamps[i], SensorManager::sensorNames[i],
				sensorSnap.values[i]));
	}

	//-----------------------------------------------------------------
	checkRtos(HAL_GetTick());
}

void Monitoring::checkRtos(uint32_t now) {
	RtosTaskInfo info[Config::MAX_RTOS_TASKS] = { };
	uint8_t n = _kernel->tasks(info, Config::MAX_RTOS_TASKS);

	// Stack: alert any task whose free margin dropped below the threshold.
	for (uint8_t i = 0; i < n; i++) {
		if (info[i].stackFreeWords < Config::RTOS_STACK_WARN_WORDS)
			_bus->publish(Topic::ALERT, BusFormat::altStackLow(info[i].name, info[i].stackFreeWords));
	}

	// Heap: current free fluctuates, so re-arm once it recovers above the threshold.
	uint32_t heapFree = _kernel->heapFreeBytes();
	if (heapFree < Config::RTOS_HEAP_WARN_BYTES) {
		if (!_heapAlarmed) {
			_heapAlarmed = true;
			_bus->publish(Topic::ALERT, BusFormat::altHeapLow(heapFree));
		}
	} else {
		_heapAlarmed = false;
	}

	// Throttled periodic HEALTH summary (~1 Hz at the 10 Hz task rate).
	if (++_rtosDivCounter >= Config::RTOS_HEALTH_DIVIDER) {
		_rtosDivCounter = 0;
		_bus->publish(Topic::HEALTH,
			BusFormat::hltRtosHeap(now, heapFree, _kernel->heapMinFreeBytes()));
		for (uint8_t i = 0; i < n; i++)
			_bus->publish(Topic::HEALTH, BusFormat::hltRtosTask(now, info[i].name, info[i].stackFreeWords));
	}
}
