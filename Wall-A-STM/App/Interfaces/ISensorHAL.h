#ifndef APP_INTERFACES_ISENSORHAL_H
#define APP_INTERFACES_ISENSORHAL_H

class ISensorHAL {
public:
    virtual float read() = 0;  // raw sensor value
    virtual ~ISensorHAL() = default;
};

#endif // APP_INTERFACES_ISENSORHAL_H
