#include "Tasks/MotionPlanner.h"
#include "Services/BusFormat.h"

MotionPlanner::MotionPlanner(IBus* bus, QueueHandle_t cmdMailbox, QueueHandle_t setpointMailbox)
    : _bus(bus), _cmdMailbox(cmdMailbox), _setpointMailbox(setpointMailbox) {}

void MotionPlanner::task(void* param) {
    auto* self = static_cast<MotionPlanner*>(param);
    for (;;) {
        uint32_t notifyVal = 0;
        xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, pdMS_TO_TICKS(50));

        if (notifyVal != 0) {
            self->handleAlarm(notifyVal);
        }

        MoveCmd cmd{};
        if (xQueueReceive(self->_cmdMailbox, &cmd, 0) == pdTRUE) {
            self->processCmd(cmd);
        }
    }
}

void MotionPlanner::processCmd(const MoveCmd& cmd) {
    Setpoint sp{};
    sp.mode = SetpointMode::POSE;
    sp.pose = { cmd.x, cmd.y, cmd.angle };
    xQueueOverwrite(_setpointMailbox, &sp);
}

void MotionPlanner::handleAlarm(uint32_t bitmask) {
    xQueueReset(_setpointMailbox);
    _bus->publish(Topic::ALERT, BusFormat::altAlarm(bitmask));
}
