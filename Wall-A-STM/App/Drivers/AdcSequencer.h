#ifndef APP_DRIVERS_ADCSEQUENCER_H
#define APP_DRIVERS_ADCSEQUENCER_H

#include <cstdint>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Interfaces/IAnalogSource.h"
#include "Interfaces/IAdcGroup.h"

class AdcSequencer : public IAnalogSource, public IAdcGroup {
public:
    void bind()    override;
    void trigger() override;
    void wait()    override;
    ADC_HandleTypeDef* getInstance();
    void onConversionComplete();

protected:
    AdcSequencer(ADC_HandleTypeDef *hadc, uint32_t doneFlag,
                 const uint32_t *channels, uint8_t maxChannel,
                 uint16_t *rawValues);

    uint16_t* rawValues() { return _rawValues; }

private:
    ADC_HandleTypeDef *_hadc;
    uint32_t           _doneFlag;
    const uint32_t    *_channels;
    uint8_t            _maxChannel;
    uint16_t          *_rawValues;
    TaskHandle_t       _notifyThreadId = nullptr;
    uint8_t            _conversionIndex = 0;

    void configureAndStart(uint8_t index);
};

#endif // APP_DRIVERS_ADCSEQUENCER_H
