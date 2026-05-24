#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Services/BusFormat.h"
#include "stm32f4xx_hal.h"

SensorManager::SensorSnapshot SensorManager::latestSnapshot { };
const char* SensorManager::sensorNames[Config::MAX_SENSORS] { };

SensorManager::SensorManager(ISensor **sensors, uint8_t sensorCount,
	TaskHandle_t motionPlannerHandle, IBus *bus, IAdcGroup **adcGroups, uint8_t adcGroupCount) :
	_sensors(sensors), _sensorCount(sensorCount), _motionPlannerHandle(motionPlannerHandle), _bus(
		bus), _adcGroups(adcGroups), _adcGroupCount(adcGroupCount) {
	for (uint8_t i = 0; i < sensorCount && i < Config::MAX_SENSORS; ++i)
		sensorNames[i] = (sensors[i] != nullptr) ? sensors[i]->name() : "";
}

void SensorManager::task(void *param) {
	auto *self = static_cast<SensorManager*>(param);

	for (uint8_t i = 0; i < self->_adcGroupCount; ++i)
		self->_adcGroups[i]->bind();

	ExternalComm::log_info("SensorManager: Init OK");
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000 / Config::SENSOR_FREQ_HZ));

		self->pollOnce();
	}
}

void SensorManager::pollOnce() {
	for (uint8_t i = 0; i < _adcGroupCount; ++i)
		_adcGroups[i]->trigger();
	for (uint8_t i = 0; i < _adcGroupCount; ++i)
		_adcGroups[i]->wait();

	for (uint8_t i = 0; i < _sensorCount && i < Config::MAX_SENSORS; ++i) {
		if (_sensors[i] == nullptr)
			continue;

		float value = _sensors[i]->read();
		bool alarm = _sensors[i]->isAlarm();

		latestSnapshot.values[i] = value;

		if (alarm) {
			latestSnapshot.alarmMask |= (1u << i);
		}
	}

	latestSnapshot.count = _sensorCount;
	latestSnapshot.timestamp = HAL_GetTick();

// if (alarmMask != 0) {
// 	xTaskNotify(_motionPlannerHandle, alarmMask, eSetBits);
// }
}
