/*
 * ControlTask.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "ControlTask.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace Libs::Utils;

namespace Tasks {

TaskHandle_t ControlTask::threadId_ = nullptr;

void ControlTask::create() {
    constexpr uint16_t stackWords = 1024 / sizeof(StackType_t); // 1024 bytes
    const UBaseType_t priority = tskIDLE_PRIORITY + 2;
    const BaseType_t rc = xTaskCreate(threadFunc,
                                      "ControlTask",
                                      stackWords,
                                      nullptr,
                                      priority,
                                      &threadId_);
    if (rc == pdPASS) {
        Logger::info("ControlTask: Tâche créée - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("ControlTask: ERREUR création (xTaskCreate failed)");
    }
}

void ControlTask::threadFunc(void*) {
    Logger::info("ControlTask::threadFunc: DÉMARRAGE");
    const uint32_t ctrlMs = 20;
    uint32_t loopCount = 0;
    
    while (true) {
        loopCount++;
        
        if (loopCount % 25 == 0) {
            Logger::info("ControlTask: En cours - iteration %lu", loopCount);
        }
        
        // High-level behaviors / state machine updates
        if (App::StateMachine::getState() == App::RobotState::RUNNING) {
            // TODO: run navigation/behaviour code
        }
        vTaskDelay(pdMS_TO_TICKS(ctrlMs));
    }
}

} // namespace Tasks
