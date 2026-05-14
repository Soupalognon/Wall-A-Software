#ifndef APP_INTERFACES_IENCODERHAL_H
#define APP_INTERFACES_IENCODERHAL_H

#include <cstdint>

class IEncoderHAL {
public:
    virtual ~IEncoderHAL() = default;
    virtual int32_t getTicksLeft()  = 0;
    virtual int32_t getTicksRight() = 0;
};

#endif // APP_INTERFACES_IENCODERHAL_H
