#ifndef APP_TASKS_MOTIONPLANNER_H
#define APP_TASKS_MOTIONPLANNER_H

#include "Interfaces/IBus.h"
#include "Tasks/OdoControl.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <cstdint>

enum class MoveCmdMode : uint8_t {
	POSE, VELOCITY, STOP
};
struct MoveCmd {
	MoveCmdMode mode;
	float x;
	float y;
	float angle;
	float v;
	float w;
};

namespace AlarmBits {
static constexpr uint32_t SENSOR = 0x0000FFFF; // bits 0-15: sensor index from SensorManager
static constexpr uint32_t OVERHEAT = 1u << 16;   // from Monitoring
static constexpr uint32_t OVERCURRENT = 1u << 17; // from Monitoring
}

class MotionPlanner {
public:
	static TaskHandle_t handle;

	MotionPlanner(IBus *bus, QueueHandle_t cmdMailbox, QueueHandle_t setpointMailbox);
	static void task(void *param);

	// Exposed for unit testing
	void processCmd(const MoveCmd &cmd);
	void handleAlarm(uint32_t bitmask);

private:
	IBus *_bus;
	QueueHandle_t _cmdMailbox;
	QueueHandle_t _setpointMailbox;
};

#endif // APP_TASKS_MOTIONPLANNER_H
