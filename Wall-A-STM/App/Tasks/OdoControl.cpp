#include "Tasks/OdoControl.h"
#include "Tasks/ExternalComm.h"
#include "Services/BusFormat.h"
#include "stm32f4xx_hal.h"

OdoControl::OdoSnapshot OdoControl::latestSnapshot { };

OdoControl *OdoControl::_instance = nullptr;

OdoControl::OdoControl(IOdomHAL *odom, IMotorHAL *motor, IBus *bus, QueueHandle_t mailbox) :
	_odom(odom), _motor(motor), _bus(bus), _mailbox(mailbox) {
	_instance = this;
}

void OdoControl::reset() {
	_motor->setMotors(0.0f, 0.0f);
	_pidSpeed.reset();
	_pidAngle.reset();
	_spFilteredV = 0.0f;
	_spFilteredW = 0.0f;
	_leftDuty = 0.0f;
	_rightDuty = 0.0f;
}

void OdoControl::task(void *param) {
	auto *self = static_cast<OdoControl*>(param);
	TickType_t lastWake = xTaskGetTickCount();
	const uint32_t periodMs = 1000 / Config::ODO_FREQ_HZ;
	const TickType_t period = pdMS_TO_TICKS(periodMs);

	if (self->_motor->begin() || self->_odom->begin()) {
		ExternalComm::log_error("OdoControl: Init FAILED. Stop task");
		self->_bus->publish(Topic::ALERT, BusFormat::altInitFailed("OdoControl"));
		vTaskDelete(NULL);
	}

	ExternalComm::log_info("OdoControl: Init OK");
	uint32_t timer = HAL_GetTick();
	for (;;) {
		vTaskDelayUntil(&lastWake, period);

		self->routine();

		if ((HAL_GetTick() - timer) > 2 * periodMs) {
			ExternalComm::log_warn("OdoControl: Task is slowing down => %ld ms",
				HAL_GetTick() - timer);
		}
		timer = HAL_GetTick();
	}
}

void OdoControl::routine() {
	++_tickCount;

	_odom->update();

	Setpoint sp { };
	if (xQueuePeek(_mailbox, &sp, 0) == pdTRUE) {
		if (sp.mode == SetpointMode::VELOCITY)
			tickVelocity(sp);
		else if (sp.mode == SetpointMode::POSE)
			tickPose(sp);
	} else {
		reset();
	}

	if (_tickCount % Config::TELEM_DIVIDER == 0) {
		latestSnapshot = { _odom->getX(), _odom->getY(), _odom->getAngle(), _odom->getV(),
			_odom->getW(), _odom->getVLeft(), _odom->getVRight(), convertDutyToVolt(_leftDuty),
			convertDutyToVolt(_rightDuty), _motor->isError(), HAL_GetTick() };
	}
}

void OdoControl::tickVelocity(Setpoint sp) {
	float dt = _odom->getDt();

	// Setpoint smoothing (first-order low-pass, same scheme as VEL_EMA_ALPHA).
	// Commands arrive in steps (~10Hz) while this loop runs at ODO_FREQ_HZ; filtering
	// the setpoint turns each step into a ramp so FF and P no longer kick the motor.
	_spFilteredV = Config::SPEED_EMA_ALPHA * sp.velocity.v
		+ (1.0f - Config::SPEED_EMA_ALPHA) * _spFilteredV;
	_spFilteredW = Config::ANGLE_EMA_ALPHA * sp.velocity.w
		+ (1.0f - Config::ANGLE_EMA_ALPHA) * _spFilteredW;

	float dv = _spFilteredV - _odom->getV();
	float dw = _spFilteredW - _odom->getW();

	// Feedforward: directly maps target velocity to an estimated duty cycle (open-loop).
	// This removes most of the steady-state error before the PID even acts,
	// allowing much lower PID gains and avoiding integral windup.
	float v = _spFilteredV * Config::FF_GAIN_V + _pidSpeed.compute(dv, dt);
	float w = _spFilteredW * Config::FF_GAIN_W + _pidAngle.compute(dw, dt);

	auto clamp = [](float val, float lo, float hi) {
		return val < lo ? lo : (val > hi ? hi : val);
	};
	v = clamp(v, -Config::MAX_DUTY, Config::MAX_DUTY);
	w = clamp(w, -Config::MAX_DUTY, Config::MAX_DUTY);

	_leftDuty = clamp(v - w, -1.0f, 1.0f);
	_rightDuty = clamp(v + w, -1.0f, 1.0f);
	_motor->setMotors(_leftDuty, _rightDuty);

	if (Config::ENABLE_HIGH_SPEED_DEBUG) {
		_bus->publish(Topic::TELEMETRY,
			BusFormat::telOdoVelocity(HAL_GetTick(), _odom->getV(), _odom->getW()));
//		_bus->publish(Topic::TELEMETRY, BusFormat::telOdoVelocity(now, odoSnap.v, odoSnap.w));
//		_bus->publish(Topic::TELEMETRY,
//			BusFormat::telOdoPose(now, odoSnap.x, odoSnap.y, odoSnap.angle));
//		_bus->publish(Topic::TELEMETRY,
//			BusFormat::telOdoWheelSpeed(now, odoSnap.vLeft, odoSnap.vRight));
//		_bus->publish(Topic::TELEMETRY,
//			BusFormat::telOdoMotorVoltage(now, odoSnap.voltLeft, odoSnap.voltRight));
	}
}

void OdoControl::tickPose(Setpoint sp) {
	float dt = _odom->getDt();

	float dx = sp.pose.x - _odom->getX();
	float dy = sp.pose.y - _odom->getY();
	float errDist = sqrtf(dx * dx + dy * dy);

//	if (errDist < Config::ARRIVAL_THRESHOLD) {
//		_motor->setMotors(0.0f, 0.0f);
//		_bus->publish(Topic::ALERT, BusFormat::evtArrival());
//		_hasSetpoint = false;
//		_pidSpeed.reset();
//		_pidAngle.reset();
//		_stallCount = 0;
//		_encFaultCountL = 0;
//		_encFaultCountR = 0;
//
//		xQueueReset(_mailbox);	//TODO: To remove after!!!! Not correct
//		return;
//	}

	float rawErrAngle = atan2f(dy, dx) - _odom->getAngle();
	float errAngle = atan2f(sinf(rawErrAngle), cosf(rawErrAngle));  // normalize to [-π, π]

	float v = _pidSpeed.compute(errDist, dt);
	float w = _pidAngle.compute(errAngle, dt);

	auto clamp = [](float val, float lo, float hi) {
		return val < lo ? lo : (val > hi ? hi : val);
	};
	v = clamp(v, -Config::MAX_DUTY, Config::MAX_DUTY);
	w = clamp(w, -Config::MAX_DUTY, Config::MAX_DUTY);

	_leftDuty = clamp(v - w, -1.0f, 1.0f);
	_rightDuty = clamp(v + w, -1.0f, 1.0f);
	_motor->setMotors(_leftDuty, _rightDuty);

	if (Config::ENABLE_HIGH_SPEED_DEBUG) {
		_bus->publish(Topic::TELEMETRY,
			BusFormat::telOdoPose(HAL_GetTick(), _odom->getX(), _odom->getY(), _odom->getAngle()));
	}
}

void OdoControl::setPidGains(float P, float I, float D) {
	if (_instance == nullptr)
		return;
	ExternalComm::log_info("OdoControl: Set PID gains (speed + angle)");
	_instance->_pidSpeed.setGains(P, I, D);
	_instance->_pidAngle.setGains(P, I, D);
}

void OdoControl::setPidSpeedGains(float P, float I, float D) {
	if (_instance == nullptr)
		return;
	ExternalComm::log_info("OdoControl: Set PID speed gains");
	_instance->_pidSpeed.setGains(P, I, D);
}

void OdoControl::setPidAngleGains(float P, float I, float D) {
	if (_instance == nullptr)
		return;
	ExternalComm::log_info("OdoControl: Set PID angle gains");
	_instance->_pidAngle.setGains(P, I, D);
}

float OdoControl::convertDutyToVolt(float duty) {
	return duty * Config::MOTOR_SUPPLY_VOLTAGE;
}
