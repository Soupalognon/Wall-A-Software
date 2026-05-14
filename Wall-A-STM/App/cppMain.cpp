#include "SystemInit/SystemInit.h"
#include "FreeRTOS.h"
#include "task.h"

extern "C" void cppMain(void)
{
    SystemInit::boot();
    // boot() creates all application tasks; this task yields indefinitely
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
