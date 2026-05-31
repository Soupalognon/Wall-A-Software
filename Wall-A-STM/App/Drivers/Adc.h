#ifndef DRIVERS_ADC_H_
#define DRIVERS_ADC_H_

#include <cstdint>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

class Adc {
public:
	ADC_HandleTypeDef* getInstance();
	void onConversionComplete();
	bool isActive() const {
		return _isActive;
	}

	// Route une interruption de fin de conversion vers l'objet actif du périphérique
	// concerné. Sur un même hadc les canaux sont séquentiels (un seul actif), mais
	// plusieurs hadc peuvent converser en parallèle, d'où la résolution par (hadc, _isActive).
	static void dispatchCallback(ADC_HandleTypeDef *hadc);

protected:
	uint32_t _doneFlag;
	TaskHandle_t _notifyThreadId = nullptr;

	Adc(ADC_HandleTypeDef *hadc, const uint32_t channel, uint32_t doneFlag);

	uint16_t rawValue() {
		return _rawValue;
	}

	void start();

private:
	ADC_HandleTypeDef *_hadc;

	ADC_ChannelConfTypeDef _sConfig = { };
	uint16_t _rawValue;
	bool _isActive = false;

	// Registre intrusif de toutes les instances (auto-enregistrement au constructeur)
	static Adc *s_head;
	Adc *_next = nullptr;

};

#endif /* DRIVERS_ADC_H_ */
