#ifndef APP_INTERFACES_IMOTORHAL_H
#define APP_INTERFACES_IMOTORHAL_H

class IMotorHAL {
public:
    virtual ~IMotorHAL() = default;
    virtual void setMotors(float left, float right) = 0;
};

#endif // APP_INTERFACES_IMOTORHAL_H
