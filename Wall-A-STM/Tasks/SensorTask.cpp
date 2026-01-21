/*
 * SensorTask.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "SensorTask.hpp"
#include "B5W_LB2101.hpp"
#include "PololuIR.hpp"
#include "PowerManager.hpp"
#include "MovingAverage.hpp"
#include "Logger.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace Libs::Utils;

namespace Tasks {

TaskHandle_t SensorTask::threadId_ = nullptr;

void SensorTask::create() {
    constexpr uint16_t stackWords = 768 / sizeof(StackType_t); // 768 bytes
    const UBaseType_t priority = tskIDLE_PRIORITY + 2;
    const BaseType_t rc = xTaskCreate(threadFunc,
                                      "SensorTask",
                                      stackWords,
                                      nullptr,
                                      priority,
                                      &threadId_);
    if (rc == pdPASS) {
        Logger::info("SensorTask: Tâche créée - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("SensorTask: ERREUR création (xTaskCreate failed)");
    }
}

void SensorTask::threadFunc(void*) {
    Logger::info("SensorTask::threadFunc: DÉMARRAGE");
    DriversCustom::Distance::B5W_LB2101 b5w;
    DriversCustom::Distance::PololuIR pol;
    b5w.init();
    pol.init();
    DriversCustom::Power::PowerManager::init();

    Libs::Filter::MovingAverage avgB5W(8);
    Libs::Filter::MovingAverage avgPol(8);

    const uint32_t pollMs = 50;
    uint32_t loopCount = 0;
    
    while (true) {
        loopCount++;
        
        if (loopCount % 20 == 0) {
            Logger::info("SensorTask: En cours - iteration %lu", loopCount);
        }
        
        float d1 = b5w.readMeters();
        float d2 = pol.readMeters();
        avgB5W.push(d1);
        avgPol.push(d2);
        // Optionally publish to queue or global telemetry

        vTaskDelay(pdMS_TO_TICKS(pollMs));
    }
}

} // namespace Tasks

