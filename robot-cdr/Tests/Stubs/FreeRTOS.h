#ifndef TESTS_STUBS_FREERTOS_H
#define TESTS_STUBS_FREERTOS_H

#include <cstdint>
#include <cstddef>
#include <exception>

// FreeRTOS types for host build
typedef void*    QueueHandle_t;
typedef void*    TaskHandle_t;
typedef uint32_t UBaseType_t;
typedef int32_t  BaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE   ((BaseType_t)1)
#define pdFALSE  ((BaseType_t)0)
#define pdPASS   pdTRUE
#define portMAX_DELAY ((TickType_t)0xFFFFFFFF)
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

// taskENTER/EXIT_CRITICAL are no-ops on host (single-threaded tests)
#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()

// Thrown by vTaskDelay stub to exit task for loops after one iteration in tests
struct TaskDelayEscape : public std::exception {
    const char* what() const noexcept override { return "TaskDelayEscape"; }
};

// Real FreeRTOS.h pulls in task/queue declarations indirectly; stubs replicate this
#include "task.h"
#include "queue.h"

#endif // TESTS_STUBS_FREERTOS_H
