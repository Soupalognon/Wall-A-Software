#include <Services/Motor.h>

Motor::Motor(Drv8262& drv) : _drv(drv) {}

void Motor::setMotors(float left, float right) {
    _drv.setMotors(Config::MOTOR_L_SIGN * left,
                   Config::MOTOR_R_SIGN * right);
}
