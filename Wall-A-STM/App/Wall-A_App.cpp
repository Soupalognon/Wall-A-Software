/*
 * Wall-A_App.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "Wall-A_App.hpp"
#include "StateMachine.hpp"
#include "MotorTask.hpp"
#include "SensorTask.hpp"
#include "ControlTask.hpp"
#include "CommunicationTask.hpp"
#include "Logger.hpp"

extern "C" void WallAApp_Init() {
    App::WallAApp::init();
}

namespace App {

void WallAApp::init() {
    Libs::Utils::Logger::init();
    Libs::Utils::Logger::info("\n\n=== WallAApp initialization starting ===");
    Libs::Utils::Logger::info("Initializing StateMachine...");
    StateMachine::init();
    Libs::Utils::Logger::info("StateMachine initialized");

    // Create tasks (CMSIS-RTOS v2)
    Libs::Utils::Logger::info("Creating MotorTask...");
    Tasks::MotorTask::create();
    Libs::Utils::Logger::info("Creating SensorTask...");
    Tasks::SensorTask::create();
    Libs::Utils::Logger::info("Creating ControlTask...");
    Tasks::ControlTask::create();
    Libs::Utils::Logger::info("Creating CommunicationTask...");
    Tasks::CommunicationTask::create();

    Libs::Utils::Logger::info("=== RobotApp initialization complete ===");
    Libs::Utils::Logger::info("DefaultTask: finishing - passing control to higher priority tasks");
    // DefaultTask terminates, allowing higher-priority tasks to run
}

void WallAApp::shutdown() {
    // TODO: implement safe shutdown if needed
    Libs::Utils::Logger::info("RobotApp shutdown");
}

} // namespace App


