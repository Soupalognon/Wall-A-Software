#ifndef APP_TASKS_MONITORING_H
#define APP_TASKS_MONITORING_H

#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Interfaces/IBus.h"
#include "Services/BusFormat.h"
#include "Config.h"
#include <FreeRTOS.h>
#include <task.h>
#include "stm32f4xx_hal.h"

class Monitoring {
public:
    Monitoring(IBus* bus);
    static void task(void* param);
    void checkOnce();

private:
    IBus* _bus;
};

#endif // APP_TASKS_MONITORING_H
