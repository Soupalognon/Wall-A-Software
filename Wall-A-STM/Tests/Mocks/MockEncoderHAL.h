#ifndef TESTS_MOCKS_MOCKENCODERHAL_H
#define TESTS_MOCKS_MOCKENCODERHAL_H

#include "Interfaces/IEncoderHAL.h"

class MockEncoderHAL : public IEncoderHAL {
public:
    bool init() override { return 0; }
    int32_t getTicks()  override { return ticks;  }

private:
    int32_t ticks  = 0;

    voir reset() override { ticks = 0; }
};

#endif // TESTS_MOCKS_MOCKENCODERHAL_H
