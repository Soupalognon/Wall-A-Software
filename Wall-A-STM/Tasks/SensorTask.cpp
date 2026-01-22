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
#include "cmsis_os2.h"

using namespace Libs::Utils;

namespace Tasks {

osThreadId_t SensorTask::threadId_ = nullptr;

void SensorTask::create() {
    const osThreadAttr_t attr = {
        .name = "SensorTask",
        .stack_size = 1024,
        .priority = osPriorityNormal,
    };
    
    threadId_ = osThreadNew(threadFunc, nullptr, &attr);
    if (threadId_ != nullptr) {
        Logger::info("SensorTask: Tâche créée - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("SensorTask: ERREUR création (osThreadNew failed)");
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

        osDelay(pollMs);
    }
}

} // namespace Tasks

