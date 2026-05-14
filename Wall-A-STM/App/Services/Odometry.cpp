#include <Services/Odometry.h>
#include "Config.h"
#include "stm32f4xx_hal.h"
#include <cmath>

extern TIM_HandleTypeDef htim7;

ConcreteOdomHAL::ConcreteOdomHAL(IEncoderHAL* encL, IEncoderHAL* encR)
    : _encL(encL), _encR(encR)
{
}

uint32_t ConcreteOdomHAL::getDtFromTimer() {
    return TIM7->CNT;  // 16-bit, 1 tick = 10 µs (prescaler 839, APB1 timer 84 MHz)
}

void ConcreteOdomHAL::update() {
    uint32_t now_10us = getDtFromTimer();
    // uint16_t subtraction : wrap correct même si CNT a débordé (overflow toutes les ~655 ms >> 5 ms)
    uint16_t diff = static_cast<uint16_t>(now_10us) - static_cast<uint16_t>(_lastMicros);
    float dt = static_cast<float>(diff) * 1e-5f;  // 1 tick = 10 µs = 1e-5 s
    _dt = (dt > 1e-4f) ? dt : 0.005f;
    _lastMicros = now_10us;

    int32_t curL = _encL->getTicksLeft();
    int32_t curR = _encR->getTicksRight();
    int32_t dTickL = static_cast<int32_t>(static_cast<int16_t>(curL - _prevTicksL));
    int32_t dTickR = static_cast<int32_t>(static_cast<int16_t>(curR - _prevTicksR));
    _prevTicksL = curL;
    _prevTicksR = curR;

    float dL = Config::ENCODER_L_SIGN * dTickL * Config::D_PER_TICK;
    float dR = Config::ENCODER_R_SIGN * dTickR * Config::D_PER_TICK;

    _vL = dL / _dt;
    _vR = dR / _dt;
    _v  = (_vL + _vR) * 0.5f;
    _w  = (_vR - _vL) / Config::WHEEL_BASE_M;

    float dd     = (dL + dR) * 0.5f;
    float dTheta = (dR - dL) / Config::WHEEL_BASE_M;
    _angle += dTheta;

    if (_angle >  static_cast<float>(M_PI)) _angle -= 2.0f * static_cast<float>(M_PI);
    if (_angle < -static_cast<float>(M_PI)) _angle += 2.0f * static_cast<float>(M_PI);

    _x += dd * cosf(_angle);
    _y += dd * sinf(_angle);
}

float ConcreteOdomHAL::getX()      { return _x; }
float ConcreteOdomHAL::getY()      { return _y; }
float ConcreteOdomHAL::getAngle()  { return _angle; }
float ConcreteOdomHAL::getVLeft()  { return _vL; }
float ConcreteOdomHAL::getVRight() { return _vR; }
float ConcreteOdomHAL::getV()      { return _v; }
float ConcreteOdomHAL::getW()      { return _w; }
float ConcreteOdomHAL::getDt()     { return _dt; }
