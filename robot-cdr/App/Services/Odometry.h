#ifndef APP_SERVICES_CONCRETEODOMHAL_H
#define APP_SERVICES_CONCRETEODOMHAL_H

#include "Interfaces/IOdomHAL.h"
#include "Interfaces/IEncoderHAL.h"
#include <cstdint>

class ConcreteOdomHAL : public IOdomHAL {
public:
    ConcreteOdomHAL(IEncoderHAL* encL, IEncoderHAL* encR);

    void  update()    override;
    float getX()      override;
    float getY()      override;
    float getAngle()  override;
    float getVLeft()  override;
    float getVRight() override;
    float getV()      override;
    float getW()      override;
    float getDt()     override;

private:
    IEncoderHAL* _encL;
    IEncoderHAL* _encR;
    int32_t  _prevTicksL = 0;
    int32_t  _prevTicksR = 0;
    float    _x     = 0.0f;
    float    _y     = 0.0f;
    float    _angle = 0.0f;
    float    _vL    = 0.0f;
    float    _vR    = 0.0f;
    float    _v     = 0.0f;
    float    _w     = 0.0f;
    float    _dt    = 0.005f;
    uint32_t _lastMicros = 0;  // stores HAL_GetTick() value (ms); name kept for legacy, unit is ms

    uint32_t getDtFromTimer();
};

#endif // APP_SERVICES_CONCRETEODOMHAL_H
