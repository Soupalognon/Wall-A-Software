#ifndef APP_SERVICES_BUSFORMAT_H
#define APP_SERVICES_BUSFORMAT_H

#include <cstdint>

class BusFormat {
public:
	static const char* telOdoPose(uint32_t timestamp, float x, float y, float angle);
	static const char* telOdoVelocity(uint32_t timestamp, float v, float w);
	static const char* telOdoMotorVoltage(uint32_t timestamp, float voltLeft, float voltRight);
	static const char* telOdoWheelSpeed(uint32_t timestamp, float vLeft, float vRight);
	static const char* altProximity(float dist);
	static const char* hltTemp(float t);
	static const char* evtArrival();
	static const char* altAlarm(uint32_t bitmask);
	static const char* altStall();
	static const char* altEncoderFault(const char *side);
	static const char* altInitFailed(const char *side);
	static const char* altStale(const char *module);
	static const char* altSensorAlarm(uint32_t timestamp, const char* sensorName, float value);
	static const char* hltSensors(uint32_t timestamp, uint8_t count, uint32_t alarmMask);
	static const char* hltSensorValue(uint32_t timestamp, const char* sensorName, float value);
};

#endif // APP_SERVICES_BUSFORMAT_H
