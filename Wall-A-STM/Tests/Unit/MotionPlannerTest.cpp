#include "Stubs/HalStub.h"
#include <gtest/gtest.h>
#include "Mocks/MockBus.h"
#include "Tasks/MotionPlanner.h"

class MotionPlannerTest : public ::testing::Test {
protected:
    MockBus       bus;
    QueueHandle_t cmdMailbox;
    QueueHandle_t setpointMailbox;
    MotionPlanner* mp;

    void SetUp() override {
        cmdMailbox      = xQueueCreate(1, sizeof(MoveCmd));
        setpointMailbox = xQueueCreate(1, sizeof(Setpoint));
        mp = new MotionPlanner{ &bus, cmdMailbox, setpointMailbox };
    }
    void TearDown() override { delete mp; }
};

// CMD MOVE → Setpoint POSE écrit dans setpointMailbox
TEST_F(MotionPlannerTest, ProcessCmdWritesPoseSetpoint) {
    MoveCmd cmd{ 1.0f, 0.5f, 0.0f };
    mp->processCmd(cmd);

    Setpoint sp{};
    ASSERT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdTRUE);
    EXPECT_EQ(sp.mode, SetpointMode::POSE);
    EXPECT_FLOAT_EQ(sp.pose.x,     1.0f);
    EXPECT_FLOAT_EQ(sp.pose.y,     0.5f);
    EXPECT_FLOAT_EQ(sp.pose.angle, 0.0f);
}

// Setpoint précédent écrasé par nouveau CMD MOVE (xQueueOverwrite)
TEST_F(MotionPlannerTest, ProcessCmdOverwritesPreviousSetpoint) {
    mp->processCmd({ 1.0f, 0.0f, 0.0f });
    mp->processCmd({ 2.0f, 3.0f, 1.57f });

    Setpoint sp{};
    ASSERT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdTRUE);
    EXPECT_FLOAT_EQ(sp.pose.x, 2.0f);
}

// Alarme → xQueueReset sur setpointMailbox + ALT ALARM publié
TEST_F(MotionPlannerTest, HandleAlarmResetsMailboxAndPublishesAlert) {
    mp->processCmd({ 1.0f, 0.0f, 0.0f });

    mp->handleAlarm(0x01);

    Setpoint sp{};
    EXPECT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdFALSE);

    ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_NE(bus.last(Topic::ALERT)->payload.find("ALARM"), std::string::npos);
}

// Alarme sans consigne préalable — pas de crash
TEST_F(MotionPlannerTest, HandleAlarmWithEmptyMailboxDoesNotCrash) {
    EXPECT_NO_THROW(mp->handleAlarm(0xFF));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));
}

// ExternalComm → cmdMailbox → processCmd via queue (intégration queue)
TEST_F(MotionPlannerTest, QueueIntegration_CmdMailboxDeliveredToSetpoint) {
    MoveCmd cmd{ 5.0f, 2.5f, 0.785f };
    xQueueOverwrite(cmdMailbox, &cmd);

    MoveCmd received{};
    ASSERT_EQ(xQueueReceive(cmdMailbox, &received, 0), pdTRUE);
    mp->processCmd(received);

    Setpoint sp{};
    ASSERT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdTRUE);
    EXPECT_FLOAT_EQ(sp.pose.x, 5.0f);
    EXPECT_FLOAT_EQ(sp.pose.y, 2.5f);
}
