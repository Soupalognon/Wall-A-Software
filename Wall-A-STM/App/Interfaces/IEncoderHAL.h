#ifndef APP_INTERFACES_IENCODERHAL_H
#define APP_INTERFACES_IENCODERHAL_H

#include <cstdint>
#include <stm32f4xx_hal.h>

class IEncoderHAL {
public:
    virtual ~IEncoderHAL() = default;
    virtual bool init() = 0;
    virtual int32_t getTicks()  = 0;
private:
    virtual void reset() = 0;
};

#endif // APP_INTERFACES_IENCODERHAL_H
