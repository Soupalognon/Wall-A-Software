#include "Services/BusFormat.h"
#include <cstdint>
#include <cstdio>

const char* BusFormat::telOdo(float x, float y, float angle) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "TEL ODO x:%.2f y:%.2f w:%.2f\n", x, y, angle);
    return buf;
}

const char* BusFormat::altProximity(float dist) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "ALT PROXIMITY %.2f\n", dist);
    return buf;
}

const char* BusFormat::logInfo(const char* msg) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "LOG INFO %s\n", msg);
    return buf;
}

const char* BusFormat::logWarn(const char* msg) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "LOG WARN %s\n", msg);
    return buf;
}

const char* BusFormat::hltTemp(float t) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "HLT TEMP %.2f\n", t);
    return buf;
}

const char* BusFormat::evtArrival() {
    return "ALT ARRIVAL\n";
}

const char* BusFormat::altAlarm(uint32_t bitmask) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT ALARM 0x%08lX\n",
             static_cast<unsigned long>(bitmask));
    return buf;
}

const char* BusFormat::altStall() { return "ALT STALL\n"; }

const char* BusFormat::altEncoderFault(const char* side) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT ENCODER_FAULT %s\n", side);
    return buf;
}

const char* BusFormat::altInitFailed(const char* side) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT INIT %s\n", side);
    return buf;
}

const char* BusFormat::altStale(const char* module) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT STALE %s\n", module);
    return buf;
}

const char* BusFormat::hltSensors(uint8_t count, uint32_t alarmMask) {
    static char buf[48];
    snprintf(buf, sizeof(buf), "HLT SENSORS N=%u ALM=0x%08lX\n",
             static_cast<unsigned>(count),
             static_cast<unsigned long>(alarmMask));
    return buf;
}
