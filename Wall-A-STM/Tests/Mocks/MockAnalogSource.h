#ifndef TESTS_MOCKS_MOCKANANALOGSOURCE_H
#define TESTS_MOCKS_MOCKANANALOGSOURCE_H

#include "Interfaces/IAnalogSource.h"

class MockAnalogSource : public IAnalogSource {
public:
    float values[4] = {};

    float read(uint8_t ch) override {
        if (ch < 4) return values[ch];
        return 0.0f;
    }
};

#endif // TESTS_MOCKS_MOCKANANALOGSOURCE_H
