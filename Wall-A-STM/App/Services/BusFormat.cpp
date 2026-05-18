#include "Services/BusFormat.h"
#include <cstdint>
#include <cstdio>

const char* BusFormat::telOdoPose(uint32_t timestamp, float x, float y, float angle) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "TEL ODO time:%ld x:%.2f y:%.2f w:%.2f\r\n", timestamp, x, y, angle);
    return buf;
}

const char* BusFormat::telOdoVelocity(uint32_t timestamp, float v, float w) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "TEL ODO time:%ld v:%.2f w:%.2f\r\n", timestamp, v, w);
    return buf;
}

const char* BusFormat::telOdoMotorVoltage(uint32_t timestamp, float voltLeft, float voltRight) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "TEL ODO time:%ld voltLeft:%.2f voltRight:%.2f\r\n", timestamp, voltLeft, voltRight);
    return buf;
}

const char* BusFormat::telOdoWheelSpeed(uint32_t timestamp, float vLeft, float vRight) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "TEL ODO time:%ld vLeft:%.3f, vRight:%.3f\r\n", timestamp, vLeft, vRight);
    return buf;
}

const char* BusFormat::altProximity(float dist) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "ALT PROXIMITY %.2f\r\n", dist);
    return buf;
}

const char* BusFormat::logInfo(const char* msg) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "LOG INFO %s\r\n", msg);
    return buf;
}

const char* BusFormat::logWarn(const char* msg) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "LOG WARN %s\r\n", msg);
    return buf;
}

const char* BusFormat::hltTemp(float t) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "HLT TEMP %.2f\r\n", t);
    return buf;
}

const char* BusFormat::evtArrival() {
    return "ALT ARRIVAL\r\n";
}

const char* BusFormat::altAlarm(uint32_t bitmask) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT ALARM 0x%08lX\r\n",
             static_cast<unsigned long>(bitmask));
    return buf;
}

const char* BusFormat::altStall() { return "ALT STALL\r\n"; }

const char* BusFormat::altEncoderFault(const char* side) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT ENCODER_FAULT %s\r\n", side);
    return buf;
}

const char* BusFormat::altInitFailed(const char* side) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT INIT %s\r\n", side);
    return buf;
}

const char* BusFormat::altStale(const char* module) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT STALE %s\r\n", module);
    return buf;
}

const char* BusFormat::hltSensors(uint8_t count, uint32_t alarmMask) {
    static char buf[48];
    snprintf(buf, sizeof(buf), "HLT SENSORS N=%u ALM=0x%08lX\r\n",
             static_cast<unsigned>(count),
             static_cast<unsigned long>(alarmMask));
    return buf;
}
