#ifndef APP_SYSTEMINIT_SYSTEMINIT_H
#define APP_SYSTEMINIT_SYSTEMINIT_H

#include <FreeRTOS.h>
#include <task.h>

extern TaskHandle_t motionPlannerHandle;

class SystemInit {
public:
    static void boot();
};

#endif // APP_SYSTEMINIT_SYSTEMINIT_H
