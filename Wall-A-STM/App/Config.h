#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <cstdint>
#include "FreeRTOS.h"

#define ENABLE_HIGH_SPEED_TUNING false

namespace Config {

struct ComQueuePolicy {
	bool log;
	bool tel;
	bool alt;
	bool hlt;
};

#if (ENABLE_HIGH_SPEED_TUNING == true)
// Set a field to true to allow that topic on the channel, false to suppress it.
static constexpr ComQueuePolicy UART_POLICY = { true, false, false, false };
static constexpr ComQueuePolicy USB_POLICY = { true, false, false, false };
static constexpr ComQueuePolicy ETH_POLICY  = { false,  false,  false, false };
#else
static constexpr ComQueuePolicy UART_POLICY = { true, false, true, false };
static constexpr ComQueuePolicy USB_POLICY = { false, true, true, true };
static constexpr ComQueuePolicy ETH_POLICY = { false, false, false, false };
#endif

// Task frequency
static constexpr uint32_t ODO_FREQ_HZ = 200;
static constexpr uint32_t SENSOR_FREQ_HZ = 10;
static constexpr uint32_t MONITORING_FREQ_HZ = 10;

// Fixed array bounds (NFR-02 - no dynamic allocation)
static constexpr uint8_t MAX_SENSORS = 15; //WARN: If you touch this value you must modify "Wall-A-STM\App\Tasks\MotionPlanner.h" --> AlarmBits --> SENSOR table
static constexpr uint8_t MAX_ACTUATORS = 10;

// Physical odometry constants (adapt to real robot geometry)
static constexpr float WHEEL_RADIUS_M = 0.0381f;
static constexpr float WHEEL_BASE_M = 0.200f;
static constexpr int32_t TICKS_PER_REV = 256 * 4; //*4 because software config is increasing resolution (CubeMx config)
static constexpr int32_t MOTO_REDUCTION_RATIO = 14;
static constexpr float D_PER_TICK = (2.0f * 3.14159265f * WHEEL_RADIUS_M)
	/ (static_cast<float>(TICKS_PER_REV) * static_cast<float>(MOTO_REDUCTION_RATIO));

// OdoControl tuning constants
static constexpr float MOTOR_SUPPLY_VOLTAGE = 24.0f;
static constexpr float ARRIVAL_THRESHOLD = 0.02f;
static constexpr float PID_I_MAX_SPEED = 1.0f;
static constexpr float PID_I_MAX_ANGLE = 0.5f;
static constexpr float MAX_DUTY = 1.0f;
static constexpr float VEL_EMA_ALPHA = 0.1f;	//(0=max smooth, 1=no filter)
static constexpr float SPEED_EMA_ALPHA = 0.15f;	//setpoint smoothing (0=max smooth, 1=no filter)
static constexpr float ANGLE_EMA_ALPHA = 0.05f;	//setpoint smoothing (0=max smooth, 1=no filter)
static constexpr float FF_GAIN_V = 0.58f;        // duty per (m/s)  — estimated from open-loop data
static constexpr float FF_GAIN_W = FF_GAIN_V * WHEEL_BASE_M / 2.0f; // duty per (rad/s) — derived from FF_GAIN_V
static constexpr int8_t ENCODER_L_SIGN = 1;
static constexpr int8_t ENCODER_R_SIGN = -1;
static constexpr int8_t MOTOR_L_SIGN = +1;
static constexpr int8_t MOTOR_R_SIGN = +1;
static constexpr uint8_t TELEM_DIVIDER = 10;

// PID defaults — speed (linear velocity)
static constexpr float PID_KP_DEFAULT = 0.3f;
static constexpr float PID_KI_DEFAULT = 0.1f;
static constexpr float PID_KD_DEFAULT = 0.0f;

// PID defaults — angle (angular velocity)
static constexpr float PID_KP_ANGLE_DEFAULT = 0.6f;
static constexpr float PID_KI_ANGLE_DEFAULT = 0.1f;
static constexpr float PID_KD_ANGLE_DEFAULT = 0.01f;

// Sensor alarm thresholds
static constexpr float PROXIMITY_ALARM_M = 0.20f;
static constexpr float TEMP_ALARM_C = 60.0f;
static constexpr float CURRENT_ALARM_MA = 4000.0f;

// Monitoring stale threshold (ms)
static constexpr uint32_t MONITORING_STALE_MS = 500;

// Command watchdog — reset setpoint to 0 if no command received within timeout
static constexpr bool CMD_WATCHDOG_ENABLED = true;
static constexpr uint32_t CMD_WATCHDOG_TIMEOUT_MS = 1000;

// FreeRTOS stack sizes (32-bit words on ARM Cortex-M)
static constexpr uint16_t STACK_ODO_CONTROL = 512;
static constexpr uint16_t STACK_MOTION_PLANNER = 256;
static constexpr uint16_t STACK_SENSOR_MANAGER = 512;
static constexpr uint16_t STACK_MONITORING = 1024;
static constexpr uint16_t STACK_EXTCOMM_RX = 512;
static constexpr uint16_t STACK_EXTCOMM_TX = 256;

// FreeRTOS task priorities (higher number = higher priority)
static constexpr UBaseType_t PRIO_ODO_CONTROL = 6;
static constexpr UBaseType_t PRIO_MOTION_PLANNER = 5;
static constexpr UBaseType_t PRIO_EXTCOMM_RX = 5;
static constexpr UBaseType_t PRIO_EXTCOMM_TX = 4;
static constexpr UBaseType_t PRIO_SENSOR_MANAGER = 3;
static constexpr UBaseType_t PRIO_MONITORING = 2;

}

#endif // APP_CONFIG_H
