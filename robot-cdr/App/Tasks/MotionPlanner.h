#ifndef APP_TASKS_MOTIONPLANNER_H
#define APP_TASKS_MOTIONPLANNER_H

#include "Interfaces/IBus.h"
#include "Tasks/OdoControl.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <cstdint>

struct MoveCmd { float x; float y; float angle; };

class MotionPlanner {
public:
    MotionPlanner(IBus* bus, QueueHandle_t cmdMailbox, QueueHandle_t setpointMailbox);
    static void task(void* param);

    // Exposed for unit testing
    void processCmd(const MoveCmd& cmd);
    void handleAlarm(uint32_t bitmask);

private:
    IBus*         _bus;
    QueueHandle_t _cmdMailbox;
    QueueHandle_t _setpointMailbox;
};

#endif // APP_TASKS_MOTIONPLANNER_H
