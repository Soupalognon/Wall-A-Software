// Provides static member definitions normally found in SystemInit.cpp
// (SystemInit.cpp includes STM32-specific headers, so it's excluded from host test builds)
#include "Stubs/HalStub.h"
#include "Tasks/ExternalComm.h"

ExternalComm::CommSnapshot ExternalComm::latestSnapshot{};
