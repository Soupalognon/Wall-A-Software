#ifndef TESTS_MOCKS_MOCKMOTORHAL_H
#define TESTS_MOCKS_MOCKMOTORHAL_H

#include "Interfaces/IMotorHAL.h"

class MockMotorHAL : public IMotorHAL {
public:
    float lastLeft  = 0.0f;
    float lastRight = 0.0f;
    int   callCount = 0;

    void setMotors(float left, float right) override {
        lastLeft  = left;
        lastRight = right;
        ++callCount;
    }

    void reset() {
        lastLeft  = 0.0f;
        lastRight = 0.0f;
        callCount = 0;
    }
};

#endif // TESTS_MOCKS_MOCKMOTORHAL_H
