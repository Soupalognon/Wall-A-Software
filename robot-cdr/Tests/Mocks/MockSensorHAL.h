#ifndef TESTS_MOCKS_MOCKSENSORHAL_H
#define TESTS_MOCKS_MOCKSENSORHAL_H

#include "Interfaces/ISensorHAL.h"

class MockSensorHAL : public ISensorHAL {
public:
    float returnValue = 0.0f;

    float read() override { return returnValue; }
};

#endif // TESTS_MOCKS_MOCKSENSORHAL_H
