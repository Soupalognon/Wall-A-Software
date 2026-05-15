#ifndef TESTS_MOCKS_MOCKACTUATORHAL_H
#define TESTS_MOCKS_MOCKACTUATORHAL_H

#include "Interfaces/IActuatorHAL.h"
#include <cstdint>

class MockActuatorHAL : public IActuatorHAL {
public:
    float    lastValue = 0.0f;
    uint32_t callCount = 0;

    void set(float value) override {
        lastValue = value;
        ++callCount;
    }

    void reset() { lastValue = 0.0f; callCount = 0; }
};

#endif // TESTS_MOCKS_MOCKACTUATORHAL_H
