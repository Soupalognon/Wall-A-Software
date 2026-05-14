#ifndef TESTS_MOCKS_MOCKODOMHAL_H
#define TESTS_MOCKS_MOCKODOMHAL_H

#include "Interfaces/IOdomHAL.h"

class MockOdomHAL : public IOdomHAL {
public:
    float x = 0.0f, y = 0.0f, angle = 0.0f;
    float vL = 0.0f, vR = 0.0f, v = 0.0f, w = 0.0f;
    float dt = 0.005f;
    int   updateCallCount = 0;

    void  update()    override { ++updateCallCount; }
    float getX()      override { return x; }
    float getY()      override { return y; }
    float getAngle()  override { return angle; }
    float getVLeft()  override { return vL; }
    float getVRight() override { return vR; }
    float getV()      override { return v; }
    float getW()      override { return w; }
    float getDt()     override { return dt; }
};

#endif // TESTS_MOCKS_MOCKODOMHAL_H
