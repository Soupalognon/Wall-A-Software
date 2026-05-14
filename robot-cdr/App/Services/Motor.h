#ifndef APP_SERVICES_MOTOR_H
#define APP_SERVICES_MOTOR_H

#include <Drivers/Drv8262.h>
#include "Interfaces/IMotorHAL.h"
#include "Config.h"

class Motor : public IMotorHAL {
public:
    explicit Motor(Drv8262& drv);
    void setMotors(float left, float right) override;

private:
    Drv8262& _drv;
};

#endif // APP_SERVICES_MOTOR_H
