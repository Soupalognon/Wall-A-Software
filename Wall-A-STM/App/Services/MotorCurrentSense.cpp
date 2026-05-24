#include "Services/MotorCurrentSense.h"

static constexpr uint32_t CHANNELS[] = {
    ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_6, ADC_CHANNEL_8 };

MotorCurrentSense::MotorCurrentSense(ADC_HandleTypeDef *hadc)
    : AdcSequencer(hadc, 0x02, CHANNELS, 4, _rawBuf) {}

float MotorCurrentSense::read(uint8_t ch) {
    if (ch == channel::PRIMARY_MOTOR_LEFT || ch == channel::PRIMARY_MOTOR_RIGHT)
        return voltageToCurrentPrimary(rawValues()[ch]);
    else
        return voltageToCurrentSecondary(rawValues()[ch]);
}

float MotorCurrentSense::voltageToCurrentPrimary(uint16_t adcVal) {
    float voltage = (adcVal / 4096.0f) * 3.3f;
    voltage *= 1000.0f;
    voltage /= PRI_GAIN_FACTOR;
    return voltage / PRI_RESISTANCE_REFERENCE;
}

float MotorCurrentSense::voltageToCurrentSecondary(uint16_t adcVal) {
    float voltage = (adcVal / 4096.0f) * 3.3f;
    return voltage / SEC_RESISTANCE_REFERENCE;
}
