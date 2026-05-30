#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Services/BusFormat.h"
#include "stm32f4xx_hal.h"

SensorManager::SensorSnapshot SensorManager::latestSnapshot { };
const char *SensorManager::sensorNames[Config::MAX_SENSORS] { };

SensorManager::SensorManager(SensorGroup *groups, uint8_t groupCount,
	TaskHandle_t motionPlannerHandle, IBus *bus) :
	_groups(groups), _groupCount(groupCount), _motionPlannerHandle(motionPlannerHandle), _bus(bus) {

	uint8_t total = 0;
	for (uint8_t g = 0; g < _groupCount; ++g) {
		for (uint8_t s = 0; s < _groups[g].sensorCount; ++s) {
			ISensor *sensor = _groups[g].sensors[s];
			if (sensor == nullptr)
				continue;
			uint8_t id = sensor->id();
			if (id < Config::MAX_SENSORS) {
				sensorNames[id] = sensor->name();
				++total;
			}
		}
	}
	latestSnapshot.count = total;
}

void SensorManager::task(void *param) {
	auto *self = static_cast<SensorManager*>(param);

	uint32_t now = HAL_GetTick();
	for (uint8_t i = 0; i < self->_groupCount; ++i) {
		self->_groups[i].source->bind();
		self->_groups[i].nextDueMs = now;
	}

	ExternalComm::log_info("SensorManager: Init OK");
	for (;;) {
		self->pollDueGroups();

		now = HAL_GetTick();
		uint32_t next = self->_groups[0].nextDueMs;
		for (uint8_t i = 1; i < self->_groupCount; ++i) {
			if (self->_groups[i].nextDueMs < next)	//Find smallest wakeup
				next = self->_groups[i].nextDueMs;
		}

		int32_t sleepMs = (int32_t) (next - now);
		if (sleepMs > 0)
			vTaskDelay(pdMS_TO_TICKS(sleepMs));
	}
}

void SensorManager::pollDueGroups() {
	uint32_t now = HAL_GetTick();
	for (uint8_t i = 0; i < _groupCount; ++i) {
		SensorGroup &g = _groups[i];

		if (g.nextDueMs > now)
			continue;

		g.source->trigger();
		uint32_t flag = g.source->doneFlag();
		while (flag != 0) {
			uint32_t bits = 0;
			xTaskNotifyWait(0, flag, &bits, portMAX_DELAY);
			flag &= ~bits;
		}

		for (uint8_t s = 0; s < g.sensorCount; ++s) {
			ISensor *sensor = g.sensors[s];
			if (sensor == nullptr)
				continue;

			uint8_t id = sensor->id();
			if (id >= Config::MAX_SENSORS)
				continue;

			float value = sensor->read();
			bool alarm = sensor->isAlarm();

			latestSnapshot.values[id] = value;
			latestSnapshot.timestamps[id] = HAL_GetTick();
			if (alarm)
				latestSnapshot.alarmMask |= (1u << id);
			else
				latestSnapshot.alarmMask &= ~(1u << id);

			if (s == 0)
				ExternalComm::log_info("val=%.1f", value);
		}

		now = HAL_GetTick();
		g.nextDueMs += g.periodMs;
		while (g.nextDueMs <= now)
			g.nextDueMs += g.periodMs;
	}
}
