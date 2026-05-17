#ifndef APP_CONTROLLERS_PID_H
#define APP_CONTROLLERS_PID_H

class Pid {
public:
    Pid(float kp, float ki, float kd, float iMax);
    float compute(float error, float dt);
    void  reset();
    void setGains(float P, float I, float D);

private:
    float _kp, _ki, _kd, _iMax;
    float _integral  = 0.0f;
    float _prevError = 0.0f;
};

#endif // APP_CONTROLLERS_PID_H
