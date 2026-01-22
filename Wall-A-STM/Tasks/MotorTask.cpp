/*
 * MotorTask.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "MotorTask.hpp"
#include "Drv8262.hpp"
#include "Encoder.hpp"
#include "PID.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"
#include "cmsis_os2.h"

using namespace DriversCustom::Motor;
using namespace DriversCustom::Encoder;
using namespace Libs::Controller;
using namespace Libs::Utils;

namespace Tasks {

osThreadId_t MotorTask::threadId_ = nullptr;

void MotorTask::create() {
    Logger::info("MotorTask: Création de la tâche");
    
    const osThreadAttr_t attr = {
        .name = "MotorTask",
        .stack_size = 2048,
        .priority = osPriorityAboveNormal,
    };
    
    threadId_ = osThreadNew(threadFunc, nullptr, &attr);
    if (threadId_ != nullptr) {
        Logger::info("MotorTask: Tâche créée avec succès - ThreadID: %p", (void*)threadId_);
    } else {
        Logger::error("MotorTask: ERREUR - Echec de la création de la tâche (osThreadNew failed)");
    }
}

void MotorTask::start() {
    // not needed - create starts it
}

static Drv8262 drv;
static DriversCustom::Encoder::Encoder encLeft;
static DriversCustom::Encoder::Encoder encRight;
static PID leftPid(0.1f, 0.01f, 0.0f);
static PID rightPid(0.1f, 0.01f, 0.0f);

void MotorTask::threadFunc(void* ) {
    Logger::info("MotorTask::threadFunc: DÉMARRAGE");
    
    Logger::info("MotorTask: Initialisation du driver Drv8262...");
    drv.init();
    Logger::info("MotorTask: Drv8262 initialisé");
    
    Logger::info("MotorTask: Initialisation encodeur gauche...");
    encLeft.init();
    Logger::info("MotorTask: Encodeur gauche initialisé");
    
    Logger::info("MotorTask: Initialisation encodeur droit...");
    encRight.init();
    Logger::info("MotorTask: Encodeur droit initialisé");

    Logger::info("MotorTask: Démarrage - Moteurs en avant");
    
    // Mettre les moteurs en avant à 10%
    drv.setMotors(0.1f, 0.1f);
    Logger::info("MotorTask: setMotors appelé, entrée dans la boucle main");
    
    const uint32_t periodMs = 1000 / 200; // default 200Hz
    uint32_t loopCount = 0;
    bool motorsEnabled = true;
    
    while (true) {
        loopCount++;
        
    //     // Log CHAQUE itération (1 sur 10) pour voir si on boucle vraiment
        if (loopCount % 10 == 0) {
            Logger::info("MotorTask: Avant StateMachine check - iteration %lu", loopCount);
            if (motorsEnabled) {
                drv.setMotors(-0.1f, -0.1f);
                motorsEnabled = false;
            } else {
                drv.setMotors(0.1f, 0.1f);
                motorsEnabled = true;
            }
        }
        
    //     App::RobotState state = App::StateMachine::getState();
    //     if (state == App::RobotState::EMERGENCY_STOP) {
    //         Logger::warn("MotorTask: EMERGENCY_STOP détecté!");
    //         drv.setLeftDuty(0.0f);
    //         drv.setRightDuty(0.0f);
    //         osDelay(10);
    //         continue;
    //     }

    //     // Les moteurs continuent d'avancer indéfiniment
    //     drv.setMotors(0.1f, 0.1f);  // Avant à 30%
        
    //     if (loopCount % 10 == 0) {
    //         Logger::info("MotorTask: Avant osDelay - iteration %lu, period=%lu ms", loopCount, periodMs);
    //     }

        osDelay(100);
        
    //     if (loopCount % 10 == 0) {
    //         Logger::info("MotorTask: APRÈS osDelay - iteration %lu", loopCount);
    //     }
    }
}

} // namespace Tasks

