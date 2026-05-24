#ifndef APP_TASKS_SENSORMANAGER_H
#define APP_TASKS_SENSORMANAGER_H

#include "Interfaces/ISensor.h"
#include "Interfaces/IAdcGroup.h"
#include "Interfaces/IBus.h"
#include "Config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <cstdint>

namespace SensorType {
constexpr uint8_t PrimaryMotorTemp = 0;
constexpr uint8_t SecondaryMotorTemp = 1;
constexpr uint8_t PowerSupplyTemp = 2;
constexpr uint8_t PrimaryMotorCurrentL = 3;
constexpr uint8_t PrimaryMotorCurrentR = 4;
constexpr uint8_t SecondaryMotorCurrentL = 5;
constexpr uint8_t SecondaryMotorCurrentR = 6;
}

class SensorManager {
public:
	struct SensorSnapshot {
		float values[Config::MAX_SENSORS];
		bool alarms[Config::MAX_SENSORS];
		uint8_t count;
		uint32_t timestamp;

		float operator[](uint8_t idx) const {
			return values[idx];
		}
		bool alarm(uint8_t idx) const {
			return alarms[idx];
		}
	};
	static SensorSnapshot latestSnapshot;

	SensorManager(ISensor **sensors, uint8_t sensorCount, TaskHandle_t motionPlannerHandle,
		IBus *bus, IAdcGroup **adcGroups = nullptr, uint8_t adcGroupCount = 0);

	static void task(void *param);
	void pollOnce();

private:
	ISensor **_sensors;
	uint8_t _sensorCount;
	TaskHandle_t _motionPlannerHandle;
	IBus *_bus;
	IAdcGroup **_adcGroups;
	uint8_t _adcGroupCount;
};

#endif // APP_TASKS_SENSORMANAGER_H
