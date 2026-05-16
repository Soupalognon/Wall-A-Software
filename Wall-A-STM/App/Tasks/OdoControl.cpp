#include "Tasks/OdoControl.h"
#include "Tasks/ExternalComm.h"
#include "Services/BusFormat.h"
#include "stm32f4xx_hal.h"

OdoControl::OdoSnapshot OdoControl::latestSnapshot { };

OdoControl::OdoControl(IOdomHAL *odom, IMotorHAL *motor, IBus *bus, QueueHandle_t mailbox) :
	_odom(odom), _motor(motor), _bus(bus), _mailbox(mailbox) {
}

void OdoControl::task(void *param) {
	auto *self = static_cast<OdoControl*>(param);
	TickType_t lastWake = xTaskGetTickCount();
	const TickType_t period = pdMS_TO_TICKS(1000 / Config::ODO_FREQ_HZ);

	if (self->_motor->begin() || self->_odom->begin()) {
		ExternalComm::log_error("OdoControl: Init FAILED. Stop task");
		self->_bus->publish(Topic::ALERT, BusFormat::altInitFailed("OdoControl"));
		vTaskDelete(NULL);
	}

	ExternalComm::log_info("OdoControl: Init OK");
	for (;;) {
		vTaskDelayUntil(&lastWake, period);

		self->routine();
	}
}

void OdoControl::routine() {
	++_tickCount;

	_odom->update();

	Setpoint sp { };
	if (xQueuePeek(_mailbox, &sp, 0) == pdTRUE) {
		//		self->tickPose();
		tickVelocity(sp);
	} else {
		_motor->setMotors(0.0f, 0.0f);
		//			return;
	}

	if (_tickCount % Config::TELEM_DIVIDER == 0) {
		latestSnapshot = { _odom->getX(), _odom->getY(), _odom->getAngle(), _odom->getVLeft(),
			_odom->getVRight(), _odom->getV(), _odom->getW(), _motor->isError(), HAL_GetTick() };
		//		_bus->publish(Topic::TELEMETRY, BusFormat::telOdoVelocity(rawV, rawW));
		//		ExternalComm::log_info("rawV: %.2f, rawW: %.2f / v: %.2f, w: %.2f", rawV, rawW, _odom->getV(), _odom->getW());
		//		ExternalComm::log_info("leftDuty: %.2f, rightDuty: %.2f / v: %.2f, w: %.2f", leftDuty, rightDuty, _odom->getV(), _odom->getW());
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

	float rawV = v;
	float rawW = w;

	auto clamp = [](float val, float lo, float hi) {
		return val < lo ? lo : (val > hi ? hi : val);
	};
	v = clamp(v, -Config::MAX_DUTY, Config::MAX_DUTY);
	w = clamp(w, -Config::MAX_DUTY, Config::MAX_DUTY);

	float leftDuty = clamp(v - w, -1.0f, 1.0f);
	float rightDuty = clamp(v + w, -1.0f, 1.0f);
	_motor->setMotors(leftDuty, rightDuty);

	// ─── Stall detection ───────────────────────────────────────────────────
	static constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;

	float avgDuty = (fabsf(leftDuty) + fabsf(rightDuty)) * 0.5f;
	if (avgDuty > Config::STALL_DUTY_THRESHOLD
		&& fabsf(_odom->getV()) < Config::STALL_SPEED_THRESHOLD) {
		//TODO:
		//Not working! Has encoders are attached to motor and have not distinct wheels
		//The getV is correlated to duty
		//Will need to find another system
		if (++_stallCount >= STALL_TICKS) {
			_motor->setMotors(0.0f, 0.0f);
			_bus->publish(Topic::ALERT, BusFormat::altStall());
			_pidSpeed.reset();
			_pidAngle.reset();
			_stallCount = 0;
			return;
		}
	} else {
		_stallCount = 0;
	}

	// ─── Encoder fault detection ───────────────────────────────────────────
	if (avgDuty > Config::STALL_DUTY_THRESHOLD) {
		if (_odom->getVLeft() == 0.0f) {
			if (++_encFaultCountL >= STALL_TICKS) {
				_bus->publish(Topic::ALERT, BusFormat::altEncoderFault("LEFT"));
				_encFaultCountL = 0;
			}
		} else {
			_encFaultCountL = 0;
		}
		if (_odom->getVRight() == 0.0f) {
			if (++_encFaultCountR >= STALL_TICKS) {
				_bus->publish(Topic::ALERT, BusFormat::altEncoderFault("RIGHT"));
				_encFaultCountR = 0;
			}
		} else {
			_encFaultCountR = 0;
		}
	} else {
		_encFaultCountL = 0;
		_encFaultCountR = 0;
	}

	if (_tickCount % Config::TELEM_DIVIDER == 0) {
		_bus->publish(Topic::TELEMETRY,
			BusFormat::telOdoPose(_odom->getX(), _odom->getY(), _odom->getAngle()));

		ExternalComm::log_info("rawV: %ld, rawW: %ld / v: %ld, w: %ld", (int32_t) (rawV * 1000.0),
			(int32_t) (rawW * 1000.0), (int32_t) (v * 1000.0), (int32_t) (w * 1000.0));
	}
}

void OdoControl::tickVelocity(Setpoint sp) {
	float dt = _odom->getDt();

	float dv = sp.velocity.v - _odom->getV();
	float dw = sp.velocity.w - _odom->getW();

	float v = _pidSpeed.compute(dv, dt);
	float w = _pidAngle.compute(dw, dt);

//	float rawV = v;
//	float rawW = w;

	auto clamp = [](float val, float lo, float hi) {
		return val < lo ? lo : (val > hi ? hi : val);
	};
	v = clamp(v, -Config::MAX_DUTY, Config::MAX_DUTY);
	w = clamp(w, -Config::MAX_DUTY, Config::MAX_DUTY);

	float leftDuty = clamp(v - w, -1.0f, 1.0f);
	float rightDuty = clamp(v + w, -1.0f, 1.0f);
	_motor->setMotors(leftDuty, rightDuty);

	if (_tickCount % Config::TELEM_DIVIDER == 0) {
		//		_bus->publish(Topic::TELEMETRY, BusFormat::telOdoVelocity(rawV, rawW));
		//		ExternalComm::log_info("rawV: %.2f, rawW: %.2f / v: %.2f, w: %.2f", rawV, rawW, _odom->getV(), _odom->getW());
		//		ExternalComm::log_info("leftDuty: %.2f, rightDuty: %.2f / v: %.2f, w: %.2f", leftDuty, rightDuty, _odom->getV(), _odom->getW());
	}
}
