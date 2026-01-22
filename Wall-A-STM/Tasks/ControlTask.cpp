/*
 * ControlTask.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "ControlTask.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"
#include "cmsis_os2.h"

using namespace Libs::Utils;

namespace Tasks {

osThreadId_t ControlTask::threadId_ = nullptr;

void ControlTask::create() {
    const osThreadAttr_t attr = {
        .name = "ControlTask",
        .stack_size = 1024,
        .priority = osPriorityNormal,
    };
    
    threadId_ = osThreadNew(threadFunc, nullptr, &attr);
    if (threadId_ != nullptr) {
        Logger::info("ControlTask: Tâche créée - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("ControlTask: ERREUR création (osThreadNew failed)");
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
        osDelay(ctrlMs);
    }
}

} // namespace Tasks
