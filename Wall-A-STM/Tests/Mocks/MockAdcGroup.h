#ifndef TESTS_MOCKS_MOCKADCGROUP_H
#define TESTS_MOCKS_MOCKADCGROUP_H

#include "Interfaces/IAdcGroup.h"

class MockAdcGroup : public IAdcGroup {
public:
    int bindCallCount    = 0;
    int triggerCallCount = 0;
    uint32_t flag        = 0;  // doneFlag returns this (default 0 = no ISR wait in tests)

    void bind()              override { ++bindCallCount; }
    void trigger()           override { ++triggerCallCount; }
    uint32_t doneFlag() const override { return flag; }
};

#endif // TESTS_MOCKS_MOCKADCGROUP_H
