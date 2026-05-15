#include "Tasks/MotionPlanner.h"
#include "Services/BusFormat.h"
#include "Tasks/ExternalComm.h"

#include "stm32f4xx_hal.h"

TaskHandle_t MotionPlanner::handle = nullptr;

bool doOnce = true;
uint32_t timer;

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

//		if (doOnce && (HAL_GetTick() - timer > 1000)) {
//			doOnce = false;
//			MoveCmd cmd { };
//			cmd.x = 1.0;
//			cmd.y = 0.0;
//			cmd.angle = 0;
//			xQueueOverwrite(self->_cmdMailbox, &cmd);
//			ExternalComm::log_info("Send it");
//		}

		MoveCmd cmd { };
		if (xQueueReceive(self->_cmdMailbox, &cmd, 0) == pdTRUE) {
			ExternalComm::log_info("cmd received");
			self->processCmd(cmd);
		}
	}
}

void MotionPlanner::processCmd(const MoveCmd &cmd) {
	Setpoint sp { };
	sp.mode = SetpointMode::POSE;
	sp.pose = { cmd.x, cmd.y, cmd.angle };
	xQueueOverwrite(_setpointMailbox, &sp);
}

void MotionPlanner::handleAlarm(uint32_t bitmask) {
	xQueueReset(_setpointMailbox);
	_bus->publish(Topic::ALERT, BusFormat::altAlarm(bitmask));
}
