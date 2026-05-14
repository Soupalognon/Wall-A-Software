#ifndef APP_TASKS_ODOCONTROL_H
#define APP_TASKS_ODOCONTROL_H

#include "Interfaces/IOdomHAL.h"
#include "Interfaces/IMotorHAL.h"
#include "Interfaces/IBus.h"
#include "Controllers/Pid.h"
#include "Config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <cmath>
#include <cstdint>

enum class SetpointMode : uint8_t { POSE, VELOCITY };

struct PoseTarget     { float x, y, angle; };
struct VelocityTarget { float v, w; };

struct Setpoint {
    SetpointMode mode = SetpointMode::POSE;
    union {
        PoseTarget     pose;
        VelocityTarget velocity;
    };
};

class OdoControl {
public:
    struct OdoSnapshot {
        float    x{};
        float    y{};
        float    angle{};
        float    vLeft{};
        float    vRight{};
        float    rawV{};
        float    rawW{};
        uint32_t timestamp{};
    };
    static OdoSnapshot latestSnapshot;

    OdoControl(IOdomHAL* odom, IMotorHAL* motor, IBus* bus, QueueHandle_t mailbox);

    static void task(void* param);
    void        tick();

private:
    IOdomHAL*     _odom;
    IMotorHAL*    _motor;
    IBus*         _bus;
    QueueHandle_t _mailbox;

    Pid _pidSpeed{ Config::PID_KP_DEFAULT, Config::PID_KI_DEFAULT,
                   Config::PID_KD_DEFAULT, Config::PID_I_MAX_SPEED };
    Pid _pidAngle{ Config::PID_KP_DEFAULT, Config::PID_KI_DEFAULT,
                   Config::PID_KD_DEFAULT, Config::PID_I_MAX_ANGLE };

    bool     _hasSetpoint    = false;
    uint32_t _tickCount      = 0;
    uint32_t _stallCount     = 0;
    uint32_t _encFaultCountL = 0;
    uint32_t _encFaultCountR = 0;
};

#endif // APP_TASKS_ODOCONTROL_H
