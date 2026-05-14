#ifndef TESTS_MOCKS_MOCKENCODERHAL_H
#define TESTS_MOCKS_MOCKENCODERHAL_H

#include "Interfaces/IEncoderHAL.h"

class MockEncoderHAL : public IEncoderHAL {
public:
    int32_t leftTicks  = 0;
    int32_t rightTicks = 0;

    int32_t getTicksLeft()  override { return leftTicks;  }
    int32_t getTicksRight() override { return rightTicks; }
};

#endif // TESTS_MOCKS_MOCKENCODERHAL_H
