/*
 * CommunicationTask.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "CommunicationTask.hpp"
#include "Logger.hpp"
#include "main.h" // for USB / Ethernet handles if needed
#include "cmsis_os2.h"

using namespace Libs::Utils;

namespace Tasks {

osThreadId_t CommunicationTask::threadId_ = nullptr;

void CommunicationTask::create() {
    const osThreadAttr_t attr = {
        .name = "CommTask",
        .stack_size = 1024,
        .priority = osPriorityNormal,
    };
    
    threadId_ = osThreadNew(threadFunc, nullptr, &attr);
    if (threadId_ != nullptr) {
        Logger::info("CommunicationTask: Tâche créée - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("CommunicationTask: ERREUR création (osThreadNew failed)");
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
        osDelay(50);
    }
}

} // namespace Tasks
