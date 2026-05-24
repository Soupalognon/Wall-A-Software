#ifndef TESTS_MOCKS_MOCKADCGROUP_H
#define TESTS_MOCKS_MOCKADCGROUP_H

#include "Interfaces/IAdcGroup.h"

class MockAdcGroup : public IAdcGroup {
public:
    int bindCallCount    = 0;
    int triggerCallCount = 0;
    int waitCallCount    = 0;

    void bind()    override { ++bindCallCount; }
    void trigger() override { ++triggerCallCount; }
    void wait()    override { ++waitCallCount; }
};

#endif // TESTS_MOCKS_MOCKADCGROUP_H
