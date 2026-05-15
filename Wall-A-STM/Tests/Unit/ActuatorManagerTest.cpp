#include <gtest/gtest.h>
#include "Stubs/FreeRTOS.h"
#include "Mocks/MockBus.h"
#include "Mocks/MockActuator.h"
#include "Tasks/ActuatorManager.h"

class ActuatorManagerTest : public ::testing::Test {
protected:
    MockBus bus;
    void SetUp() override { bus.clear(); }
};

TEST_F(ActuatorManagerTest, SingleActuatorRouted) {
    MockActuator a(1, "pump");
    IActuator* arr[] = { &a };
    ActuatorManager mgr(arr, 1, &bus);

    mgr.commandById(1, "ON");

    EXPECT_STREQ("ON", a.lastCmd());
    EXPECT_EQ(1u, a.commandCallCount());
    EXPECT_FALSE(bus.hasPublished(Topic::LOG));
}

TEST_F(ActuatorManagerTest, UnknownIdPublishesLogWarn) {
    MockActuator a(1, "pump");
    IActuator* arr[] = { &a };
    ActuatorManager mgr(arr, 1, &bus);

    mgr.commandById(99, "X");

    EXPECT_EQ(0u, a.commandCallCount());
    EXPECT_TRUE(bus.hasPublished(Topic::LOG, "WARN"));
}

TEST_F(ActuatorManagerTest, EmptyArrayAlwaysLogs) {
    IActuator* arr[] = {};
    ActuatorManager mgr(arr, 0, &bus);

    mgr.commandById(0, "CMD");

    EXPECT_TRUE(bus.hasPublished(Topic::LOG, "WARN"));
}

TEST_F(ActuatorManagerTest, NullActuatorSkipped) {
    MockActuator a(2, "servo");
    IActuator* arr[] = { nullptr, &a };
    ActuatorManager mgr(arr, 2, &bus);

    mgr.commandById(2, "EXTEND");

    EXPECT_STREQ("EXTEND", a.lastCmd());
    EXPECT_EQ(1u, a.commandCallCount());
    EXPECT_FALSE(bus.hasPublished(Topic::LOG));
}

TEST_F(ActuatorManagerTest, TenActuatorsEachRouted) {
    MockActuator a0(0,"a0"), a1(1,"a1"), a2(2,"a2"), a3(3,"a3"), a4(4,"a4"),
                 a5(5,"a5"), a6(6,"a6"), a7(7,"a7"), a8(8,"a8"), a9(9,"a9");
    IActuator* arr[] = { &a0,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8,&a9 };
    ActuatorManager mgr(arr, 10, &bus);

    MockActuator* mocks[] = { &a0,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8,&a9 };
    for (uint8_t i = 0; i < 10; ++i)
        mgr.commandById(i, "CMD");

    for (auto* m : mocks)
        EXPECT_EQ(1u, m->commandCallCount());
    EXPECT_FALSE(bus.hasPublished(Topic::LOG));
}

TEST_F(ActuatorManagerTest, WrongIdAmongTen) {
    MockActuator a0(0,"a0"), a1(1,"a1"), a2(2,"a2"), a3(3,"a3"), a4(4,"a4"),
                 a5(5,"a5"), a6(6,"a6"), a7(7,"a7"), a8(8,"a8"), a9(9,"a9");
    IActuator* arr[] = { &a0,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8,&a9 };
    ActuatorManager mgr(arr, 10, &bus);

    mgr.commandById(100, "X");

    MockActuator* mocks[] = { &a0,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8,&a9 };
    for (auto* m : mocks)
        EXPECT_EQ(0u, m->commandCallCount());
    EXPECT_TRUE(bus.hasPublished(Topic::LOG, "WARN"));
}

TEST_F(ActuatorManagerTest, FirstMatchWins) {
    MockActuator a1(5, "first");
    MockActuator a2(5, "second");
    IActuator* arr[] = { &a1, &a2 };
    ActuatorManager mgr(arr, 2, &bus);

    mgr.commandById(5, "GO");

    EXPECT_EQ(1u, a1.commandCallCount());
    EXPECT_EQ(0u, a2.commandCallCount());
}

TEST_F(ActuatorManagerTest, CommandStringPassedThrough) {
    MockActuator a(3, "linear");
    IActuator* arr[] = { &a };
    ActuatorManager mgr(arr, 1, &bus);

    mgr.commandById(3, "EXTEND 50");

    EXPECT_STREQ("EXTEND 50", a.lastCmd());
}
