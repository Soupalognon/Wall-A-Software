#include "Tasks/Monitoring.h"

Monitoring::Monitoring(IBus *bus, InternalTemperature *internalTemp,
	MotorCurrentSense *motorCurrentSense) :
	_bus(bus), _internalTemperatures(internalTemp), _motorCurrentSense(motorCurrentSense) {
}

void Monitoring::task(void *param) {
	auto *self = static_cast<Monitoring*>(param);

	self->_internalTemperatures->setNotifyThread(xTaskGetCurrentTaskHandle());
	self->_motorCurrentSense->setNotifyThread(xTaskGetCurrentTaskHandle());

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000 / Config::MONITORING_POLL_HZ));
		self->checkOnce();
	}
}

void Monitoring::checkOnce() {
//	///////////////////////////////////////////////////////////////////////
	//ADC reading
	_internalTemperatures->startConversion();
	_motorCurrentSense->startConversion();

	uint32_t flags = 0;
	xTaskNotifyWait(0, InternalTemperature::ADC_DONE_FLAG | MotorCurrentSense::ADC_DONE_FLAG,
		&flags, 0);

	if (flags & InternalTemperature::ADC_DONE_FLAG) {
		float t0 = _internalTemperatures->getTemperature(InternalTemperature::PRIMARY_MOTOR);
		float t1 = _internalTemperatures->getTemperature(InternalTemperature::SECONDARY_MOTOR);
		float t2 = _internalTemperatures->getTemperature(InternalTemperature::POWER_SUPPLIES);
		if (t0 > 40.0 || t1 > 40.0 || t2 > 40.0) {
			ExternalComm::log_info(
				"Temperatures [C] - Pri motor driver: %.1f, Sec motor driver: %.1f, Power supplies: %.1f",
				t0, t1, t2);
		}
	}

	if (flags & MotorCurrentSense::ADC_DONE_FLAG) {
//		float c0 = _motorCurrentSense->getCurrentSense(MotorCurrentSense::PRIMARY_MOTOR_LEFT);
//		float c1 = _motorCurrentSense->getCurrentSense(MotorCurrentSense::PRIMARY_MOTOR_RIGHT);
//		float c2 = _motorCurrentSense->getCurrentSense(MotorCurrentSense::SECONDARY_MOTOR_LEFT);
//		float c3 = _motorCurrentSense->getCurrentSense(MotorCurrentSense::SECONDARY_MOTOR_RIGHT);
//		if (c0 > 100.0 || c1 > 100.0) {
//			ExternalComm::log_info("Current [mA] - Pri left: %.3f, Pri right: %.3f", c0, c1);
//		}
//		if (c2 > 100.0 || c3 > 100.0) {
//			ExternalComm::log_info("Current [mA] - Sec left: %.3f, Sec right: %.3f", c2, c3);
//		}
	}
//	///////////////////////////////////////////////////////////////////////

//	TaskStatus_t tasks[10];
//	uint32_t totalRunTime;
//	UBaseType_t count = uxTaskGetSystemState(tasks, 10, &totalRunTime);
//	ExternalComm::log_info("%ld", count);
//	for (UBaseType_t i = 0; i < count; i++) {
//	    uint32_t pct = totalRunTime ? (tasks[i].ulRunTimeCounter * 100UL / totalRunTime) : 0;
//	    ExternalComm::log_info("%-12s stk:%4u cpu:%2lu%%",
//	        tasks[i].pcTaskName,
//	        tasks[i].usStackHighWaterMark,
//	        pct);
//	}

//	///////////////////////////////////////////////////////////////////////
//	uint32_t now = HAL_GetTick();
	OdoControl::OdoSnapshot odoSnap;
	taskENTER_CRITICAL();
	odoSnap = OdoControl::latestSnapshot;
	taskEXIT_CRITICAL();
//	if (abs(now - odoSnap.timestamp) > Config::MONITORING_STALE_MS) {
//		_bus->publish(Topic::ALERT, BusFormat::altStale("ODO"));
//	}
	if (odoSnap.motorError) {
		_bus->publish(Topic::ALERT, "Motor Error! Stop command");
	}
	if (odoSnap.v || odoSnap.w) {
//		ExternalComm::log_info(
//			"x:%.2f, y:%.2f, angle:%.2f, vLeft:%.2f, vRight:%.2f, v:%.2f, w:%.2f, motorError:%d",
//			odoSnap.x, odoSnap.y, odoSnap.angle, odoSnap.vLeft, odoSnap.vRight, odoSnap.v,
//			odoSnap.w, odoSnap.motorError);
//		ExternalComm::log_info("vLeft:%.2f, vRight:%.2f, v:%.2f, w:%.2f", odoSnap.vLeft,
//			odoSnap.vRight, odoSnap.v, odoSnap.w);
	}

//	///////////////////////////////////////////////////////////////////////
//    SensorManager::SensorSnapshot sensorSnap;
//    taskENTER_CRITICAL();
//    sensorSnap = SensorManager::latestSnapshot;
//    taskEXIT_CRITICAL();
//    if (now - sensorSnap.timestamp > Config::MONITORING_STALE_MS)
//        _bus->publish(Topic::ALERT, BusFormat::altStale("SENSOR"));
//
//    ExternalComm::CommSnapshot commSnap;
//    taskENTER_CRITICAL();
//    commSnap = ExternalComm::latestSnapshot;
//    taskEXIT_CRITICAL();
//    if (now - commSnap.timestamp > Config::MONITORING_STALE_MS)
//        _bus->publish(Topic::ALERT, BusFormat::altStale("COMM"));
}

void Monitoring::getMotorsCurrentConsumptions() {

}
