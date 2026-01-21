/*
 * MotorTask.hpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#ifndef MOTORTASK_HPP_
#define MOTORTASK_HPP_

#pragma once
#include "FreeRTOS.h"
#include "task.h"

namespace Tasks {

class MotorTask {
public:
    static void create();
    static void start(); // used if you want direct start
private:
    static void threadFunc(void* arg);
    static TaskHandle_t threadId_;
};

} // namespace Tasks

#endif /* MOTORTASK_HPP_ */
