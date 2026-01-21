/*
 * CommunicationTask.hpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#ifndef COMMUNICATIONTASK_HPP_
#define COMMUNICATIONTASK_HPP_

#pragma once
#include "FreeRTOS.h"
#include "task.h"

namespace Tasks {
class CommunicationTask {
public:
    static void create();
private:
    static void threadFunc(void* arg);
    static TaskHandle_t threadId_;
};
} // namespace Tasks

#endif /* COMMUNICATIONTASK_HPP_ */
