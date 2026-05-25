#include "Services/B5WLB2101.h"

static constexpr uint32_t CHANNELS[] = {
	ADC_CHANNEL_12, ADC_CHANNEL_10, ADC_CHANNEL_13, ADC_CHANNEL_9
};

B5WLB2101::B5WLB2101(ADC_HandleTypeDef *hadc) :
	AdcSequencer(hadc, 0x04, CHANNELS, 4, _rawBuf) {
}

float B5WLB2101::read(uint8_t ch) {
	return (rawValues()[ch] / 4096.0f) * 3.3f;
}
