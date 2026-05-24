#include "Drivers/InternalTemperature.h"
#include <math.h>

static constexpr uint32_t CHANNELS[] = { ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6 };

InternalTemperature::InternalTemperature(ADC_HandleTypeDef *hadc)
    : AdcSequencer(hadc, 0x01, CHANNELS, 3, _rawBuf) {}

float InternalTemperature::read(uint8_t ch) {
    return voltageToCelsius(rawValues()[ch]);
}

float InternalTemperature::voltageToCelsius(uint16_t adcVal) {
    float voltage = (adcVal / 4096.0f) * 3.3f;
    float res = voltage * RESISTANCE_REFERENCE / (3.3f - voltage);
    float tempK =
        1.0f
            / (1.0f / (273.15f + TEMPERATURE_REFERENCE)
                + logf(res / RESISTANCE_REFERENCE) / B_REFERENCE);
    return tempK - 273.15f;
}
