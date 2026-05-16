#include "Tasks/MotionPlanner.h"
#include "Services/BusFormat.h"
#include "Tasks/ExternalComm.h"

#include "stm32f4xx_hal.h"

TaskHandle_t MotionPlanner::handle = nullptr;

MotionPlanner::MotionPlanner(IBus *bus, QueueHandle_t cmdMailbox, QueueHandle_t setpointMailbox) :
	_bus(bus), _cmdMailbox(cmdMailbox), _setpointMailbox(setpointMailbox) {
}

void MotionPlanner::task(void *param) {
	auto *self = static_cast<MotionPlanner*>(param);

	ExternalComm::log_info("MotionPlanner: Init OK");

	for (;;) {
		uint32_t notifyVal = 0;
		xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, pdMS_TO_TICKS(50));

		if (notifyVal != 0) {
			self->handleAlarm(notifyVal);
		}

		MoveCmd cmd { };
		if (xQueueReceive(self->_cmdMailbox, &cmd, 0) == pdTRUE) {
			ExternalComm::log_info("Motion Planner: command received");
			self->processCmd(cmd);
		}
	}
}

void MotionPlanner::processCmd(const MoveCmd &cmd) {
	Setpoint sp { };

	if (cmd.mode == MoveCmdMode::POSE) {
		sp.mode = SetpointMode::POSE;
		sp.pose = { cmd.x, cmd.y, cmd.angle };
	} else if (cmd.mode == MoveCmdMode::VELOCITY) {
		sp.mode = SetpointMode::POSE;
		sp.velocity = { cmd.v, cmd.w };
	} else if (cmd.mode == MoveCmdMode::STOP) {
		xQueueReset(_setpointMailbox);
		return;
	} else {
		ExternalComm::log_error("Motion Planner: Unknown command mode. Abort command");
		return;
	}
	xQueueOverwrite(_setpointMailbox, &sp);
}

void MotionPlanner::handleAlarm(uint32_t bitmask) {
	xQueueReset(_setpointMailbox);
	_bus->publish(Topic::ALERT, BusFormat::altAlarm(bitmask));
}
