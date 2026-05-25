#include "Services/ProximeterPololu5472.h"

ProximeterPololu5472::ProximeterPololu5472(IInputCaptureHAL *ic) : _ic(ic) { }

float ProximeterPololu5472::read(uint8_t ch) {
	return static_cast<float>(_ic->getLastPulse(ch * 4));
}

void ProximeterPololu5472::trigger() { }

uint32_t ProximeterPololu5472::doneFlag() const {
	return 0;
}
