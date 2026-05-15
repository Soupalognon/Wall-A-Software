#ifndef APP_INTERFACES_IACTUATORHAL_H
#define APP_INTERFACES_IACTUATORHAL_H

class IActuatorHAL {
public:
    virtual void set(float value) = 0;
    virtual ~IActuatorHAL() = default;
};

#endif // APP_INTERFACES_IACTUATORHAL_H
