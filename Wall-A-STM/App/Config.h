#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <cstdint>
#include "FreeRTOS.h"

namespace Config {

    // Task frequency
    static constexpr uint32_t ODO_FREQ_HZ           = 200;

    // Fixed array bounds (NFR-02 - no dynamic allocation)
    static constexpr uint8_t  MAX_SENSORS            = 15;
    static constexpr uint8_t  MAX_ACTUATORS          = 10;

    // PID defaults
    static constexpr float    PID_KP_DEFAULT         = 1.0f;
    static constexpr float    PID_KI_DEFAULT         = 0.1f;
    static constexpr float    PID_KD_DEFAULT         = 0.05f;

    // Physical odometry constants (adapt to real robot geometry)
    static constexpr float    WHEEL_RADIUS_M         = 0.033f;
    static constexpr float    WHEEL_BASE_M           = 0.200f;
    static constexpr int32_t  TICKS_PER_REV          = 1000;
    static constexpr float    D_PER_TICK             = (2.0f * 3.14159265f * WHEEL_RADIUS_M)
                                                       / static_cast<float>(TICKS_PER_REV);

    // OdoControl tuning constants
    static constexpr float    ARRIVAL_THRESHOLD      = 0.02f;
    static constexpr float    PID_I_MAX_SPEED        = 0.5f;
    static constexpr float    PID_I_MAX_ANGLE        = 0.5f;
    static constexpr float    MAX_DUTY               = 0.8f;
    static constexpr float    STALL_DUTY_THRESHOLD   = 0.5f;
    static constexpr float    STALL_SPEED_THRESHOLD  = 0.05f;
    static constexpr uint32_t STALL_TIME_MS          = 500;
    static constexpr int8_t   ENCODER_L_SIGN         = +1;
    static constexpr int8_t   ENCODER_R_SIGN         = +1;
    static constexpr int8_t   MOTOR_L_SIGN           = +1;
    static constexpr int8_t   MOTOR_R_SIGN           = +1;
    static constexpr uint8_t  TELEM_DIVIDER          = 10;

    // SensorManager polling rate
    static constexpr uint32_t SENSOR_POLL_MS         = 50;

    // Monitoring stale threshold (ms)
    static constexpr uint32_t MONITORING_STALE_MS    = 500;

    // FreeRTOS stack sizes (32-bit words on ARM Cortex-M)
    static constexpr uint16_t STACK_ODO_CONTROL      = 512;
    static constexpr uint16_t STACK_MOTION_PLANNER   = 256;
    static constexpr uint16_t STACK_SENSOR_MANAGER   = 256;
    static constexpr uint16_t STACK_MONITORING       = 256;
    static constexpr uint16_t STACK_EXTCOMM_RX       = 256;
    static constexpr uint16_t STACK_EXTCOMM_TX       = 256;

    // FreeRTOS task priorities (higher number = higher priority)
    static constexpr UBaseType_t PRIO_ODO_CONTROL    = 5;
    static constexpr UBaseType_t PRIO_MOTION_PLANNER = 4;
    static constexpr UBaseType_t PRIO_EXTCOMM_RX     = 4;
    static constexpr UBaseType_t PRIO_EXTCOMM_TX     = 3;
    static constexpr UBaseType_t PRIO_SENSOR_MANAGER = 2;
    static constexpr UBaseType_t PRIO_MONITORING     = 1;

}

#endif // APP_CONFIG_H
