#ifndef TESTS_MOCKS_FAKEINPUTCAPTUREHAL_H
#define TESTS_MOCKS_FAKEINPUTCAPTUREHAL_H

#include "Interfaces/IInputCaptureHAL.h"

// Fake Input Capture HAL for host tests: feed a known pulse width, count delegations.
class FakeInputCaptureHAL : public IInputCaptureHAL {
public:
    uint32_t pulse    = 0;      // largeur d'impulsion renvoyée par getLastPulse() (µs)
    bool     newPulse = false;  // valeur renvoyée par hasNewPulse()
    int initCalls     = 0;

    bool init()              override { ++initCalls; return true; }
    uint32_t getLastPulse()  override { return pulse; }
    bool hasNewPulse() const override { return newPulse; }
};

#endif // TESTS_MOCKS_FAKEINPUTCAPTUREHAL_H
