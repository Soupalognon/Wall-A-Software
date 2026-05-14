#ifndef TESTS_STUBS_QUEUE_H
#define TESTS_STUBS_QUEUE_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* QueueSetHandle_t;
typedef void* QueueSetMemberHandle_t;

QueueHandle_t      xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
BaseType_t         xQueueOverwrite(QueueHandle_t xQueue, const void* pvItemToQueue);
BaseType_t         xQueueSend(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);
BaseType_t         xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);
BaseType_t         xQueueReset(QueueHandle_t xQueue);
BaseType_t         xQueuePeek(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);
void               vQueueDelete(QueueHandle_t xQueue);
QueueSetHandle_t   xQueueCreateSet(UBaseType_t uxEventQueueLength);
BaseType_t         xQueueAddToSet(QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet);
QueueSetMemberHandle_t xQueueSelectFromSet(QueueSetHandle_t xQueueSet, TickType_t xTicksToWait);
BaseType_t         xQueueSendFromISR(QueueHandle_t xQueue, const void* pvItemToQueue, BaseType_t* pxHigherPriorityTaskWoken);

#ifdef __cplusplus
}
#endif

#endif // TESTS_STUBS_QUEUE_H
