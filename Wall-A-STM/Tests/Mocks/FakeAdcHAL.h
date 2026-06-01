#ifndef TESTS_MOCKS_FAKEADCHAL_H
#define TESTS_MOCKS_FAKEADCHAL_H

#include "Interfaces/IAdcHAL.h"

// Fake ADC HAL for host tests: feed a known raw count, count delegations.
class FakeAdcHAL : public IAdcHAL {
public:
    uint16_t raw      = 0;   // valeur brute renvoyée par rawValue()
    uint32_t flag     = 0;   // flag reçu au dernier bind() (== doneFlag())
    int bindCalls     = 0;
    int startCalls    = 0;

    void bind(uint32_t doneFlag) override { ++bindCalls; flag = doneFlag; }
    void start()             override { ++startCalls; }
    uint16_t rawValue()      override { return raw; }
    bool isActive() const    override { return false; }
    uint32_t doneFlag() const override { return flag; }
};

#endif // TESTS_MOCKS_FAKEADCHAL_H
