#include <gtest/gtest.h>
#include "Mocks/MockActuatorHAL.h"
#include "Drivers/Pump.h"
#include "Drivers/Servo.h"
#include "Drivers/LinearTransducer.h"

// ── Pump ─────────────────────────────────────────────────────────────────────

TEST(PumpTest, PumpOn_SetsHALTo1) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    pump.command("ON");
    EXPECT_FLOAT_EQ(1.0f, hal.lastValue);
    EXPECT_EQ(1u, hal.callCount);
}

TEST(PumpTest, PumpOff_SetsHALTo0) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    pump.command("OFF");
    EXPECT_FLOAT_EQ(0.0f, hal.lastValue);
    EXPECT_EQ(1u, hal.callCount);
}

TEST(PumpTest, PumpUnknown_NoHALCall) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    pump.command("SPIN");
    EXPECT_EQ(0u, hal.callCount);
}

TEST(PumpTest, PumpId_IsCorrect) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    EXPECT_EQ(1u, pump.id());
    EXPECT_STREQ("PUMP", pump.name());
}

// ── Servo ─────────────────────────────────────────────────────────────────────

TEST(ServoTest, ServoAngle_SetsHALValue) {
    MockActuatorHAL hal;
    Servo servo{2, &hal};
    servo.command("90.0");
    EXPECT_FLOAT_EQ(90.0f, hal.lastValue);
}

TEST(ServoTest, ServoZero_SetsHALToZero) {
    MockActuatorHAL hal;
    Servo servo{2, &hal};
    servo.command("0");
    EXPECT_FLOAT_EQ(0.0f, hal.lastValue);
}

TEST(ServoTest, ServoId_IsCorrect) {
    MockActuatorHAL hal;
    Servo servo{2, &hal};
    EXPECT_EQ(2u, servo.id());
    EXPECT_STREQ("SERVO", servo.name());
}

// ── LinearTransducer ──────────────────────────────────────────────────────────

TEST(LinearTransducerTest, TransducerPos_SetsHALValue) {
    MockActuatorHAL hal;
    LinearTransducer lt{3, &hal};
    lt.command("0.5");
    EXPECT_FLOAT_EQ(0.5f, hal.lastValue);
}

TEST(LinearTransducerTest, TransducerFull_SetsHALTo1) {
    MockActuatorHAL hal;
    LinearTransducer lt{3, &hal};
    lt.command("1.0");
    EXPECT_FLOAT_EQ(1.0f, hal.lastValue);
}

TEST(LinearTransducerTest, TransducerId_IsCorrect) {
    MockActuatorHAL hal;
    LinearTransducer lt{3, &hal};
    EXPECT_EQ(3u, lt.id());
    EXPECT_STREQ("LINEAR_TRANSDUCER", lt.name());
}

TEST(ActuatorIdsTest, AllActuatorIds_AreCorrect) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    Servo servo{2, &hal};
    LinearTransducer lt{3, &hal};

    EXPECT_EQ(1u, pump.id());
    EXPECT_EQ(2u, servo.id());
    EXPECT_EQ(3u, lt.id());
    EXPECT_STREQ("PUMP", pump.name());
    EXPECT_STREQ("SERVO", servo.name());
    EXPECT_STREQ("LINEAR_TRANSDUCER", lt.name());
}
