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

class MotionPlanner {
public:
	static TaskHandle_t handle;  // set by SystemInit after xTaskCreate; nullptr until then

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
