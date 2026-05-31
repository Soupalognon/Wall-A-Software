#ifndef APP_SERVICES_PROXIMETERPOLOLU5472_H
#define APP_SERVICES_PROXIMETERPOLOLU5472_H

#include "Drivers/Adc.h"
#include "Interfaces/ISensor.h"

//class ProximeterPololu5472: public Adc, public ISensor {
//public:
//	typedef enum {
//		CH_1 = 0, CH_2 = 1, CH_3 = 2, CH_4 = 3
//		} channelEnum;
//
//	explicit ProximeterPololu5472(IInputCaptureHAL *ic);
//
//	float read(uint8_t ch) override;         // retourne la largeur d'impulsion en µs
//	void trigger() override;                // no-op : l'ISR InputCapture alimente les données
//	uint32_t doneFlag() const override;         // retourne 0 : aucune attente requise
//
//private:
//	IInputCaptureHAL *_ic;
//};

#endif // APP_SERVICES_PROXIMETERPOLOLU5472_H
