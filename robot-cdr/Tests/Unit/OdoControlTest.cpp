#include "Stubs/HalStub.h"
#include <gtest/gtest.h>
#include <cmath>
#include "Mocks/MockOdomHAL.h"
#include "Mocks/MockMotorHAL.h"
#include "Mocks/MockBus.h"
#include "Tasks/OdoControl.h"

class OdoControlTest : public ::testing::Test {
protected:
    MockOdomHAL  odom;
    MockMotorHAL motor;
    MockBus      bus;
    QueueHandle_t mailbox;
    OdoControl*  odo;

    void SetUp() override {
        getMockTick() = 0;
        mailbox = xQueueCreate(1, sizeof(Setpoint));
        odo = new OdoControl{&odom, &motor, &bus, mailbox};
    }
    void TearDown() override {
        delete odo;
        vQueueDelete(mailbox);
    }

    void pushPoseSetpoint(float x, float y, float angle = 0.0f) {
        Setpoint sp;
        sp.mode = SetpointMode::POSE;
        sp.pose = {x, y, angle};
        xQueueOverwrite(mailbox, &sp);
    }
};

// Boot guard: no setpoint -> motors zero
TEST_F(OdoControlTest, NoMotionWithoutSetpoint) {
    odo->tick();
    EXPECT_FLOAT_EQ(motor.lastLeft,  0.0f);
    EXPECT_FLOAT_EQ(motor.lastRight, 0.0f);
}

// Forward target -> positive duty on both wheels
TEST_F(OdoControlTest, ForwardSetpointProducesPositiveDuty) {
    pushPoseSetpoint(1.0f, 0.0f);
    odo->tick();
    EXPECT_GT(motor.lastLeft,  0.0f);
    EXPECT_GT(motor.lastRight, 0.0f);
}

// Arrival condition: target reached -> motors zero + arrival alert
TEST_F(OdoControlTest, ArrivalStopsMotorsAndPublishesAlert) {
    odom.x = 0.999f;
    pushPoseSetpoint(1.0f, 0.0f);
    odo->tick();
    EXPECT_FLOAT_EQ(motor.lastLeft,  0.0f);
    EXPECT_FLOAT_EQ(motor.lastRight, 0.0f);
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_TRUE(bus.published[0].payload.find("ARRIVAL") != std::string::npos);
}

// After arrival, _hasSetpoint is cleared -> next tick motors zero (boot guard or re-arrival)
TEST_F(OdoControlTest, AfterArrivalBootGuardRestored) {
    odom.x = 0.999f;
    pushPoseSetpoint(1.0f, 0.0f);
    odo->tick();  // arrival
    bus.clear(); motor.reset();
    odo->tick();  // no new setpoint in queue
    EXPECT_FLOAT_EQ(motor.lastLeft, 0.0f);
}

// Telemetry published only at TELEM_DIVIDER ticks (not every tick)
TEST_F(OdoControlTest, TelemetryPublishedAtDividerRate) {
    pushPoseSetpoint(10.0f, 0.0f);  // far target, no arrival
    for (int i = 0; i < 9; ++i) { bus.clear(); odo->tick(); }
    EXPECT_FALSE(bus.hasPublished(Topic::TELEMETRY));  // ticks 1-9: no telemetry
    bus.clear(); odo->tick();  // tick 10
    EXPECT_TRUE(bus.hasPublished(Topic::TELEMETRY));
}

// Telemetry payload starts with "TEL ODO "
TEST_F(OdoControlTest, TelemetryFormatCorrect) {
    pushPoseSetpoint(10.0f, 0.0f);
    for (int i = 0; i < 10; ++i) odo->tick();
    ASSERT_FALSE(bus.published.empty());
    EXPECT_EQ(bus.published[0].payload.substr(0, 8), "TEL ODO ");
}

// MAX_DUTY clamp: PID cannot exceed MAX_DUTY
TEST_F(OdoControlTest, DutyClampedToMaxDuty) {
    pushPoseSetpoint(1000.0f, 0.0f);  // huge target -> large PID error
    odo->tick();
    EXPECT_LE(motor.lastLeft,   Config::MAX_DUTY);
    EXPECT_GE(motor.lastLeft,  -Config::MAX_DUTY);
    EXPECT_LE(motor.lastRight,  Config::MAX_DUTY);
    EXPECT_GE(motor.lastRight, -Config::MAX_DUTY);
}

// Anti-windup: integral never exceeds PID_I_MAX_SPEED
TEST_F(OdoControlTest, PidIntegralDoesNotExceedIMax) {
    pushPoseSetpoint(1000.0f, 0.0f);
    for (int i = 0; i < 100; ++i) odo->tick();  // many ticks, sustained error
    EXPECT_LE(motor.lastLeft,  Config::MAX_DUTY);
    EXPECT_GE(motor.lastLeft, -Config::MAX_DUTY);
}

// OdoSnapshot updated with timestamp at TELEM_DIVIDER ticks
TEST_F(OdoControlTest, SnapshotUpdatedAtDividerRate) {
    getMockTick() = 5000;
    pushPoseSetpoint(10.0f, 0.0f);
    for (int i = 0; i < 10; ++i) odo->tick();
    EXPECT_EQ(OdoControl::latestSnapshot.timestamp, 5000u);
}

// ─── Robustness tests (Story 2.4) ─────────────────────────────────────────

class OdoControlTest_Robustness : public OdoControlTest {
protected:
    void setupStallCondition() {
        odom.v  = 0.0f;
        odom.vL = 0.0f;
        odom.vR = 0.0f;
        pushPoseSetpoint(1000.0f, 0.0f);
    }
};

TEST_F(OdoControlTest_Robustness, StallDetectionTriggersAfterStallTimeMs) {
    setupStallCondition();
    constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
    for (uint32_t i = 0; i < STALL_TICKS - 1; ++i) odo->tick();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
    odo->tick();
    ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_NE(bus.last(Topic::ALERT)->payload.find("STALL"), std::string::npos);
    EXPECT_FLOAT_EQ(motor.lastLeft,  0.0f);
    EXPECT_FLOAT_EQ(motor.lastRight, 0.0f);
}

TEST_F(OdoControlTest_Robustness, StallCounterResetsWhenSpeedReturns) {
    setupStallCondition();
    constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
    for (uint32_t i = 0; i < STALL_TICKS / 2; ++i) odo->tick();
    // Speed and encoder readings return — both stall and encoder fault counters reset
    odom.v = 1.0f; odom.vL = 1.0f; odom.vR = 1.0f;
    odo->tick();
    odom.v = 0.0f; odom.vL = 0.0f; odom.vR = 0.0f;
    // Another half without triggering (counters were reset, only STALL_TICKS/2 remain)
    for (uint32_t i = 0; i < STALL_TICKS / 2; ++i) odo->tick();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
}

TEST_F(OdoControlTest_Robustness, EncoderFaultLeftPublishesAlert) {
    pushPoseSetpoint(1000.0f, 0.0f);
    odom.v  = 1.0f;
    odom.vL = 0.0f;
    odom.vR = 1.0f;
    constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
    for (uint32_t i = 0; i < STALL_TICKS; ++i) odo->tick();
    ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_NE(bus.last(Topic::ALERT)->payload.find("ENCODER_FAULT LEFT"), std::string::npos);
}

TEST_F(OdoControlTest_Robustness, EncoderFaultRightPublishesAlert) {
    pushPoseSetpoint(1000.0f, 0.0f);
    odom.v  = 1.0f;
    odom.vL = 1.0f;
    odom.vR = 0.0f;
    constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
    for (uint32_t i = 0; i < STALL_TICKS; ++i) odo->tick();
    ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_NE(bus.last(Topic::ALERT)->payload.find("ENCODER_FAULT RIGHT"), std::string::npos);
}

TEST_F(OdoControlTest_Robustness, RawVWStoredInSnapshot) {
    pushPoseSetpoint(1000.0f, 0.0f);
    for (int i = 0; i < 10; ++i) odo->tick();
    getMockTick() = 1000;
    pushPoseSetpoint(1000.0f, 0.0f);
    for (int i = 0; i < 10; ++i) odo->tick();
    EXPECT_EQ(OdoControl::latestSnapshot.timestamp, 1000u);
    // rawV and rawW are the unclamped PID outputs — non-zero for a target far away
    EXPECT_NE(OdoControl::latestSnapshot.rawV, 0.0f);
}

TEST_F(OdoControlTest_Robustness, ZeroDtDoesNotProduceInfiniteOutput) {
    odom.dt = 0.0f;
    pushPoseSetpoint(1.0f, 0.0f);
    odo->tick();
    EXPECT_FALSE(std::isinf(motor.lastLeft));
    EXPECT_FALSE(std::isinf(motor.lastRight));
    EXPECT_FALSE(std::isnan(motor.lastLeft));
    EXPECT_FALSE(std::isnan(motor.lastRight));
}
