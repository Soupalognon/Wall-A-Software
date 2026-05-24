#ifndef INTERNAL_TEMPERATURES_HPP_
#define INTERNAL_TEMPERATURES_HPP_

#include <cstdint>
#include "Drivers/AdcSequencer.h"

class InternalTemperature : public AdcSequencer {
public:
    typedef enum {
        PRIMARY_MOTOR = 0, SECONDARY_MOTOR = 1, POWER_SUPPLIES = 2,
    } channel;

    InternalTemperature(ADC_HandleTypeDef *hadc);
    float read(uint8_t ch) override;

private:
    static constexpr uint16_t RESISTANCE_REFERENCE = 10000;
    static constexpr float    B_REFERENCE = 3434.0f;
    static constexpr uint8_t  TEMPERATURE_REFERENCE = 25;

    uint16_t _rawBuf[3] = {};

    float voltageToCelsius(uint16_t adcVal);
};

#endif /* INTERNAL_TEMPERATURES_HPP_ */
