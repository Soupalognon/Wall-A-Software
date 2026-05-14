#ifndef TESTS_STUBS_HALSTUB_H
#define TESTS_STUBS_HALSTUB_H

#include <cstdint>

// Controlled tick counter for tests — set before each test that checks timestamps
uint32_t& getMockTick();  // defined in FreeRTOSStub.cpp — one instance across all TUs
inline uint32_t HAL_GetTick() { return getMockTick(); }
inline void setMockTick(uint32_t v) { getMockTick() = v; }

// Minimal TIM peripheral stub
struct TIM_TypeDef {
    uint32_t CNT = 0;
};

struct TIM_HandleTypeDef {
    TIM_TypeDef* Instance = nullptr;
};

inline TIM_TypeDef& getMockTIM7() {
    static TIM_TypeDef tim7;
    return tim7;
}

// TIM7 register pointer (mirrors STM32 peripheral layout)
inline TIM_TypeDef* const TIM7 = &getMockTIM7();

#endif // TESTS_STUBS_HALSTUB_H
