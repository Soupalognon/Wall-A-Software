#include <Services/Odometry.h>
#include "Config.h"
#include "stm32f4xx_hal.h"
#include <cmath>
#include "Tasks/ExternalComm.h"

extern TIM_HandleTypeDef htim7;

Odometry::Odometry(IEncoderHAL* encL, IEncoderHAL* encR)
    : _encL(encL), _encR(encR)
{
}

bool Odometry::begin() {
	if(_encL->init()) return 1;
	if(_encR->init()) return 1;

	HAL_TIM_Base_Start(&htim7);  // TIM7 : compteur 10 µs pour getDtFromTimer()

	return 0;
}

uint32_t Odometry::getDtFromTimer() {
    return TIM7->CNT;  // 16-bit, 1 tick = 10 µs (prescaler 839, APB1 timer 84 MHz)
}

void Odometry::update() {
    uint32_t now_10us = getDtFromTimer();
    // uint16_t subtraction : wrap correct même si CNT a débordé (overflow toutes les ~655 ms >> 5 ms)
    uint16_t diff = static_cast<uint16_t>(now_10us) - static_cast<uint16_t>(_lastMicros);
    float dt = static_cast<float>(diff) * 1e-5f;  // 1 tick = 10 µs = 1e-5 s
    _dt = (dt > 1e-4f) ? dt : 0.005f;
    _lastMicros = now_10us;

    int32_t curL = _encL->getTicks();
    int32_t curR = _encR->getTicks();
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

float Odometry::getX()      { return _x; }
float Odometry::getY()      { return _y; }
float Odometry::getAngle()  { return _angle; }
float Odometry::getVLeft()  { return _vL; }
float Odometry::getVRight() { return _vR; }
float Odometry::getV()      { return _v; }
float Odometry::getW()      { return _w; }
float Odometry::getDt()     { return _dt; }
