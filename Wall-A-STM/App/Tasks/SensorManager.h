#ifndef APP_TASKS_SENSORMANAGER_H
#define APP_TASKS_SENSORMANAGER_H

#include "Interfaces/ISensor.h"
#include "Interfaces/ISensorSource.h"
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
constexpr uint8_t ProximityCH1 = 7;
constexpr uint8_t ProximityCH2 = 8;
constexpr uint8_t ProximityCH3 = 9;
constexpr uint8_t ProximityCH4 = 10;
constexpr uint8_t PololuProxCH1 = 11;
constexpr uint8_t PololuProxCH2 = 12;
constexpr uint8_t PololuProxCH3 = 13;
constexpr uint8_t PololuProxCH4 = 14;
}

class SensorManager {
public:
	struct SensorSnapshot {
		float values[Config::MAX_SENSORS];
		uint32_t timestamps[Config::MAX_SENSORS];
		uint32_t alarmMask;
		uint8_t count;
	};
	static SensorSnapshot latestSnapshot;
	static const char* sensorNames[Config::MAX_SENSORS];

	struct SensorGroup {
		ISensorSource *source;
		ISensor **sensors;
		uint8_t sensorCount;
		uint32_t periodMs;
		uint32_t nextDueMs;
	};

	SensorManager(SensorGroup *groups, uint8_t groupCount, TaskHandle_t motionPlannerHandle,
		IBus *bus);

	static void task(void *param);
	void pollDueGroups();

private:
	SensorGroup *_groups;
	uint8_t _groupCount;
	TaskHandle_t _motionPlannerHandle;
	IBus *_bus;
};

#endif // APP_TASKS_SENSORMANAGER_H
