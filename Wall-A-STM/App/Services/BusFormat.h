#ifndef APP_SERVICES_BUSFORMAT_H
#define APP_SERVICES_BUSFORMAT_H

#include <cstdint>

class BusFormat {
public:
    static const char* telOdo(float x, float y, float angle);
    static const char* altProximity(float dist);
    static const char* logInfo(const char* msg);
    static const char* hltTemp(float t);
    static const char* evtArrival();
    static const char* altAlarm(uint32_t bitmask);
    static const char* altStall();
    static const char* altEncoderFault(const char* side);
    static const char* hltSensors(uint8_t count, uint32_t alarmMask);
};

#endif // APP_SERVICES_BUSFORMAT_H
