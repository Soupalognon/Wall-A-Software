#include <gtest/gtest.h>
#include "Controllers/Pid.h"

TEST(PidTest, Proportional_OutputScalesWithError) {
    Pid pid{2.0f, 0.0f, 0.0f, 10.0f};
    float out = pid.compute(3.0f, 0.005f);
    EXPECT_FLOAT_EQ(6.0f, out);
}

TEST(PidTest, Integral_AccumulatesOverTime) {
    Pid pid{0.0f, 1.0f, 0.0f, 100.0f};
    float out1 = pid.compute(1.0f, 0.1f);  // integral = 0.1
    float out2 = pid.compute(1.0f, 0.1f);  // integral = 0.2
    float out3 = pid.compute(1.0f, 0.1f);  // integral = 0.3
    EXPECT_FLOAT_EQ(0.1f, out1);
    EXPECT_FLOAT_EQ(0.2f, out2);
    EXPECT_FLOAT_EQ(0.3f, out3);
    EXPECT_GT(out3, out1);
}

TEST(PidTest, Integral_ClampedAtIMax) {
    Pid pid{0.0f, 1.0f, 0.0f, 0.5f};
    for (int i = 0; i < 100; ++i)
        pid.compute(1.0f, 0.1f);
    // integral is clamped at 0.5; kp=kd=0 so output = ki * integral
    float out = pid.compute(0.0f, 0.005f);
    EXPECT_LE(out, 0.5f + 1e-5f);
    EXPECT_GE(out, 0.5f - 1e-5f);
}

TEST(PidTest, Derivative_DampsOnErrorDecrease) {
    Pid pid{0.0f, 0.0f, 1.0f, 10.0f};
    pid.compute(5.0f, 0.1f);           // first call: derivative = (5-0)/0.1 = 50
    float out2 = pid.compute(3.0f, 0.1f);  // second call: derivative = (3-5)/0.1 = -20
    EXPECT_LT(out2, 0.0f);
}

TEST(PidTest, Reset_ClearsIntegralAndPrevError) {
    Pid pid{1.0f, 1.0f, 1.0f, 10.0f};
    pid.compute(5.0f, 0.01f);
    pid.reset();
    float out = pid.compute(2.0f, 0.01f);

    Pid fresh{1.0f, 1.0f, 1.0f, 10.0f};
    float expected = fresh.compute(2.0f, 0.01f);
    EXPECT_FLOAT_EQ(expected, out);
}

TEST(PidTest, ZeroError_ZeroOutput) {
    Pid pid{5.0f, 5.0f, 5.0f, 10.0f};
    float out = pid.compute(0.0f, 0.005f);
    EXPECT_FLOAT_EQ(0.0f, out);
}
