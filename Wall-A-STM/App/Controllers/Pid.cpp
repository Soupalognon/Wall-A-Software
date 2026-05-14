#include "Controllers/Pid.h"

Pid::Pid(float kp, float ki, float kd, float iMax)
    : _kp(kp), _ki(ki), _kd(kd), _iMax(iMax) {}

float Pid::compute(float error, float dt) {
    _integral += error * dt;
    if (_integral >  _iMax) _integral =  _iMax;
    if (_integral < -_iMax) _integral = -_iMax;
    float derivative = (dt > 1e-9f) ? (error - _prevError) / dt : 0.0f;
    _prevError = error;
    return _kp * error + _ki * _integral + _kd * derivative;
}

void Pid::reset() {
    _integral  = 0.0f;
    _prevError = 0.0f;
}
