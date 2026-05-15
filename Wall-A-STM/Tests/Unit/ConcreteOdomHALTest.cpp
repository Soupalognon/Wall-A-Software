#include <gtest/gtest.h>
#include <cmath>
#include "Services/Odometry.h"
#include "Mocks/MockEncoderHAL.h"
#include "Config.h"

// HAL_GetTick() returns 0 in stubs -> dt fallback = 0.005f

static constexpr float kPi = static_cast<float>(M_PI);

// --- Helper: run N identical update steps ---
static void stepN(Odometry& odom, MockEncoderHAL& encL, MockEncoderHAL& encR,
                  int32_t dTickL, int32_t dTickR, int steps) {
    for (int i = 0; i < steps; ++i) {
        encL.leftTicks  += dTickL;
        encR.rightTicks += dTickR;
        odom.update();
    }
}

// Test 1: zero ticks -> position stays at origin
TEST(ConcreteOdomHALTest, ZeroTicksNoDisplacement) {
    MockEncoderHAL encL, encR;
    Odometry odom(&encL, &encR);

    odom.update();

    EXPECT_FLOAT_EQ(odom.getX(),     0.0f);
    EXPECT_FLOAT_EQ(odom.getY(),     0.0f);
    EXPECT_FLOAT_EQ(odom.getAngle(), 0.0f);
}

// Test 2: equal forward ticks both sides -> straight ahead (x > 0, y = 0, angle = 0)
TEST(ConcreteOdomHALTest, EqualForwardTicksMovesStraight) {
    MockEncoderHAL encL, encR;
    Odometry odom(&encL, &encR);

    const int32_t N = 200;
    encL.leftTicks  = N;
    encR.rightTicks = N;
    odom.update();

    float expected_dist = Config::ENCODER_L_SIGN * N * Config::D_PER_TICK;

    EXPECT_NEAR(odom.getX(),     expected_dist, 1e-5f);
    EXPECT_NEAR(odom.getY(),     0.0f,          1e-5f);
    EXPECT_NEAR(odom.getAngle(), 0.0f,          1e-5f);
}

// Test 3: left tick only (right=0) -> robot turns right (angle < 0)
TEST(ConcreteOdomHALTest, LeftTickOnlyTurnsRight) {
    MockEncoderHAL encL, encR;
    Odometry odom(&encL, &encR);

    const int32_t N = 300;
    encL.leftTicks  = N;
    encR.rightTicks = 0;
    odom.update();

    // dL > 0, dR = 0 -> dTheta = (0 - dL) / WHEEL_BASE < 0
    EXPECT_LT(odom.getAngle(), 0.0f);
}

// Test 4: angle normalization stays within [-pi, +pi] after large rotation
TEST(ConcreteOdomHALTest, AngleNormalizationStaysInBounds) {
    MockEncoderHAL encL, encR;
    Odometry odom(&encL, &encR);

    // Accumulate right-only ticks to generate large positive angle (> pi)
    // dTheta per step = dR / WHEEL_BASE = (dTickR * D_PER_TICK) / WHEEL_BASE
    // We want total > pi, so use enough steps; keep per-step delta small (< 32767)
    const int32_t dR = 500;
    const int steps  = 15;   // 15 * 500 * D_PER_TICK / WHEEL_BASE >> pi
    stepN(odom, encL, encR, 0, dR, steps);

    float angle = odom.getAngle();
    EXPECT_LE(angle,  kPi);
    EXPECT_GE(angle, -kPi);
}

// Test 5: reversed left cable (negative ticks = ENCODER_L_SIGN=-1 equivalent) changes trajectory
TEST(ConcreteOdomHALTest, SignFlipChangesTrajectory) {
    // Case A: both sides forward (N ticks) -> straight ahead
    MockEncoderHAL encLA, encRA;
    Odometry odomA(&encLA, &encRA);
    const int32_t N = 300;
    encLA.leftTicks  = N;
    encRA.rightTicks = N;
    odomA.update();
    float xA = odomA.getX();
    EXPECT_GT(xA, 0.0f);

    // Case B: left reversed (-N ticks with SIGN=+1 = same as +N ticks with SIGN=-1)
    MockEncoderHAL encLB, encRB;
    Odometry odomB(&encLB, &encRB);
    encLB.leftTicks  = -N;
    encRB.rightTicks =  N;
    odomB.update();

    // Different trajectory: right side only goes forward -> turns left, less forward displacement
    EXPECT_LT(odomB.getX(), xA);
}
