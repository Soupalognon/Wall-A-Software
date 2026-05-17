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

enum class SetpointMode : uint8_t {
	POSE, VELOCITY
};

struct PoseTarget {
	float x, y, angle;
};
struct VelocityTarget {
	float v, w;
};

struct Setpoint {
	SetpointMode mode = SetpointMode::POSE;
	union {
		PoseTarget pose;
		VelocityTarget velocity;
	};
};

class OdoControl {
public:
	struct OdoSnapshot {
		float x { };
		float y { };
		float angle { };
		float vLeft { };
		float vRight { };
		float v { };
		float w { };
		bool motorError { };
		uint32_t timestamp { };
	};
	static OdoSnapshot latestSnapshot;

	OdoControl(IOdomHAL *odom, IMotorHAL *motor, IBus *bus, QueueHandle_t mailbox);

	static void task(void *param);

	static void setPidGains(float P, float I, float D);

private:
	IOdomHAL *_odom;
	IMotorHAL *_motor;
	IBus *_bus;
	QueueHandle_t _mailbox;

	Pid _pidSpeed { Config::PID_KP_DEFAULT, Config::PID_KI_DEFAULT, Config::PID_KD_DEFAULT,
		Config::PID_I_MAX_SPEED };
	Pid _pidAngle { Config::PID_KP_DEFAULT, Config::PID_KI_DEFAULT, Config::PID_KD_DEFAULT,
		Config::PID_I_MAX_ANGLE };

	uint32_t _tickCount = 0;
	uint32_t _stallCount = 0;
	uint32_t _encFaultCountL = 0;
	uint32_t _encFaultCountR = 0;

	static OdoControl* _instance;

	void routine();
	void tickPose(Setpoint sp);
	void tickVelocity(Setpoint sp);
};

#endif // APP_TASKS_ODOCONTROL_H
