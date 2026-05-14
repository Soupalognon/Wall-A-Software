#ifndef TESTS_STUBS_TASK_H
#define TESTS_STUBS_TASK_H

#include "FreeRTOS.h"

typedef enum { eSetBits = 0, eIncrement, eSetValueWithOverwrite, eSetValueWithoutOverwrite, eNoAction } eNotifyAction;

typedef void (*TaskFunction_t)(void*);

#ifdef __cplusplus
extern "C" {
#endif

void         vTaskDelay(TickType_t xTicksToDelay);
void         vTaskDelayUntil(TickType_t* pxPreviousWakeTime, TickType_t xTimeIncrement);
TickType_t   xTaskGetTickCount(void);
BaseType_t   xTaskCreate(TaskFunction_t pvTaskCode, const char* pcName,
                         uint16_t usStackDepth, void* pvParameters,
                         UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask);
BaseType_t   xTaskNotify(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction);
BaseType_t   xTaskNotifyWait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
                             uint32_t* pulNotificationValue, TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif

#endif // TESTS_STUBS_TASK_H
