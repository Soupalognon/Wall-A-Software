#ifndef APP_INTERFACES_IODOMHAL_H
#define APP_INTERFACES_IODOMHAL_H

class IOdomHAL {
public:
    virtual ~IOdomHAL() = default;
    virtual void  update()    = 0;
    virtual float getX()      = 0;
    virtual float getY()      = 0;
    virtual float getAngle()  = 0;
    virtual float getVLeft()  = 0;
    virtual float getVRight() = 0;
    virtual float getV()      = 0;
    virtual float getW()      = 0;
    virtual float getDt()     = 0;
};

#endif // APP_INTERFACES_IODOMHAL_H
