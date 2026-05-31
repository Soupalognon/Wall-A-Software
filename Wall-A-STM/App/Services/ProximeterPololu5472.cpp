//#include "Services/ProximeterPololu5472.h"
//#include <cmath>
//
//ProximeterPololu5472::ProximeterPololu5472(IInputCaptureHAL *ic) :
//	_ic(ic) {
//}
//
//float ProximeterPololu5472::read(uint8_t ch) {
//	if (!_ic->hasNewPulse(ch))
//		return std::nanf("");
//
//	return static_cast<float>(_ic->getLastPulse(ch));
//}
//
//void ProximeterPololu5472::trigger() {
//}
//
//uint32_t ProximeterPololu5472::doneFlag() const {
////	for(uint8_t i=0; i<4;i++) {
////		if(!_ic->hasNewPulse(i))
////			return 1;
////	}
//
//	return 0;
//}
