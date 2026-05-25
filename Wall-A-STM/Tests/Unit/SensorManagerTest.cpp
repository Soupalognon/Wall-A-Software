#include "Stubs/HalStub.h"
#include <gtest/gtest.h>
#include "Mocks/MockBus.h"
#include "Mocks/MockSensor.h"
#include "Mocks/MockAdcGroup.h"
#include "Stubs/FreeRTOS.h"
#include "Tasks/SensorManager.h"

class SensorManagerTest : public ::testing::Test {
protected:
    MockBus bus;

    void SetUp() override {
        resetTestNotifications();
        bus.clear();
        setMockTick(1000);
        SensorManager::latestSnapshot = {};
    }
};

TEST_F(SensorManagerTest, EmptyGroupsNoCrash) {
    SensorManager sm{nullptr, 0, nullptr, &bus};
    EXPECT_NO_THROW(sm.pollDueGroups());
    EXPECT_EQ(0u, SensorManager::latestSnapshot.count);
}

TEST_F(SensorManagerTest, SingleSensorNoAlarmNoBit) {
    MockSensor s0{0, "temp", 25.0f, false};
    ISensor* sensors[] = {&s0};
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, sensors, 1, 1000, 0} };
    SensorManager sm{groups, 1, nullptr, &bus};
    sm.pollDueGroups();
    EXPECT_EQ(0u, SensorManager::latestSnapshot.alarmMask & 0x01u);
}

TEST_F(SensorManagerTest, SingleSensorAlarmSetsBit0) {
    MockSensor s0{0, "prox", 0.05f, true};
    ISensor* sensors[] = {&s0};
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, sensors, 1, 1000, 0} };
    SensorManager sm{groups, 1, nullptr, &bus};
    sm.pollDueGroups();
    EXPECT_NE(0u, SensorManager::latestSnapshot.alarmMask & 0x01u);
}

TEST_F(SensorManagerTest, SensorAtId3SetsBit3) {
    MockSensor s3{3, "current", 10.0f, true};
    ISensor* sensors[] = {&s3};
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, sensors, 1, 1000, 0} };
    SensorManager sm{groups, 1, nullptr, &bus};
    sm.pollDueGroups();
    EXPECT_NE(0u, SensorManager::latestSnapshot.alarmMask & 0x08u);
}

TEST_F(SensorManagerTest, SnapshotValuesMatchSensorReads) {
    MockSensor s0{0, "temp", 42.5f, false};
    MockSensor s1{1, "prox", 0.1f,  false};
    ISensor* sensors[] = {&s0, &s1};
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, sensors, 2, 1000, 0} };
    SensorManager sm{groups, 1, nullptr, &bus};
    sm.pollDueGroups();
    EXPECT_FLOAT_EQ(42.5f, SensorManager::latestSnapshot.values[0]);
    EXPECT_FLOAT_EQ(0.1f,  SensorManager::latestSnapshot.values[1]);
}

TEST_F(SensorManagerTest, AlarmBitClearsWhenAlarmStops) {
    MockSensor s0{0, "temp", 100.0f, true};
    ISensor* sensors[] = {&s0};
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, sensors, 1, 1000, 0} };
    SensorManager sm{groups, 1, nullptr, &bus};
    sm.pollDueGroups();
    EXPECT_NE(0u, SensorManager::latestSnapshot.alarmMask & 0x01u);

    s0.setAlarm(false);
    groups[0].nextDueTick = 0;          // make it due again
    sm.pollDueGroups();
    EXPECT_EQ(0u, SensorManager::latestSnapshot.alarmMask & 0x01u);
}

TEST_F(SensorManagerTest, TimestampPerSensorUpdated) {
    MockSensor s0{0, "temp",   25.0f, false};
    MockSensor s1{1, "cur",    1.0f,  false};
    ISensor* sensorsA[] = {&s0};
    ISensor* sensorsB[] = {&s1};
    MockAdcGroup gA, gB;
    SensorManager::SensorGroup groups[] = {
        {&gA, sensorsA, 1, 1000, 0},
        {&gB, sensorsB, 1, 200,  0},
    };
    SensorManager sm{groups, 2, nullptr, &bus};

    setMockTick(1000);
    sm.pollDueGroups();
    EXPECT_EQ(1000u, SensorManager::latestSnapshot.timestamps[0]);
    EXPECT_EQ(1000u, SensorManager::latestSnapshot.timestamps[1]);

    // Avance le temps : seul le groupe à 200ms doit être dû
    setMockTick(1300);
    sm.pollDueGroups();
    EXPECT_EQ(1000u, SensorManager::latestSnapshot.timestamps[0]); // groupe 1Hz pas encore dû
    EXPECT_EQ(1300u, SensorManager::latestSnapshot.timestamps[1]); // groupe 5Hz dû
}

TEST_F(SensorManagerTest, GroupNotDueSkipsTrigger) {
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, nullptr, 0, 1000, 5000} };
    SensorManager sm{groups, 1, nullptr, &bus};
    setMockTick(1000);
    sm.pollDueGroups();
    EXPECT_EQ(0, adc.triggerCallCount);
}

TEST_F(SensorManagerTest, DueGroupTriggers) {
    MockAdcGroup adc;
    SensorManager::SensorGroup groups[] = { {&adc, nullptr, 0, 1000, 0} };
    SensorManager sm{groups, 1, nullptr, &bus};
    setMockTick(1000);
    sm.pollDueGroups();
    EXPECT_EQ(1, adc.triggerCallCount);
}

TEST_F(SensorManagerTest, TwoDueGroupsBothTriggered) {
    MockAdcGroup g0, g1;
    SensorManager::SensorGroup groups[] = {
        {&g0, nullptr, 0, 1000, 0},
        {&g1, nullptr, 0, 200,  0},
    };
    SensorManager sm{groups, 2, nullptr, &bus};
    sm.pollDueGroups();
    EXPECT_EQ(1, g0.triggerCallCount);
    EXPECT_EQ(1, g1.triggerCallCount);
}

TEST_F(SensorManagerTest, CountReflectsAllSensorsAcrossGroups) {
    MockSensor s0{0, "a"}, s1{1, "b"}, s2{2, "c"};
    ISensor* groupA[] = {&s0, &s1};
    ISensor* groupB[] = {&s2};
    MockAdcGroup gA, gB;
    SensorManager::SensorGroup groups[] = {
        {&gA, groupA, 2, 1000, 0},
        {&gB, groupB, 1, 200,  0},
    };
    SensorManager sm{groups, 2, nullptr, &bus};
    EXPECT_EQ(3u, SensorManager::latestSnapshot.count);
}
