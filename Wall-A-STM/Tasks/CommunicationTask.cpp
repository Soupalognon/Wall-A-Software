/*
 * CommunicationTask.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "CommunicationTask.hpp"
#include "Logger.hpp"
#include "main.h" // for USB / Ethernet handles if needed
#include "FreeRTOS.h"
#include "task.h"

using namespace Libs::Utils;

namespace Tasks {

TaskHandle_t CommunicationTask::threadId_ = nullptr;

void CommunicationTask::create() {
    constexpr uint16_t stackWords = 1024 / sizeof(StackType_t); // 1024 bytes
    const UBaseType_t priority = tskIDLE_PRIORITY + 2;
    const BaseType_t rc = xTaskCreate(threadFunc,
                                      "CommTask",
                                      stackWords,
                                      nullptr,
                                      priority,
                                      &threadId_);
    if (rc == pdPASS) {
        Logger::info("CommunicationTask: Tâche créée - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("CommunicationTask: ERREUR création (xTaskCreate failed)");
    }
}

void CommunicationTask::threadFunc(void*) {
    Logger::info("CommunicationTask::threadFunc: DÉMARRAGE");
    uint32_t loopCount = 0;
    
    while (true) {
        loopCount++;
        
        if (loopCount % 10 == 0) {
            Logger::info("CommunicationTask: En cours - iteration %lu", loopCount);
        }
        
        // TODO: setup USB CDC / lwIP sockets if needed. Use non-blocking APIs.
        // Example: poll USB CDC for commands and dispatch
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

} // namespace Tasks
