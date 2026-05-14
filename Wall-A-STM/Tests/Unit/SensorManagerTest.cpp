#include "Stubs/HalStub.h"
#include <gtest/gtest.h>
#include "Mocks/MockBus.h"
#include "Mocks/MockSensor.h"
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

TEST_F(SensorManagerTest, EmptySensorArrayNoCrash) {
    ISensor* sensors[Config::MAX_SENSORS] = {};
    SensorManager sm{sensors, 0, nullptr, &bus};
    sm.pollOnce();
    EXPECT_EQ(SensorManager::latestSnapshot.count, 0u);
}

TEST_F(SensorManagerTest, SingleSensorNoAlarmNoNotify) {
    MockSensor s0{0, "temp", 25.0f, false};
    ISensor* sensors[Config::MAX_SENSORS] = {&s0};
    SensorManager sm{sensors, 1, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0, &bits, 0);
    EXPECT_EQ(bits, 0u);
}

TEST_F(SensorManagerTest, HealthAlwaysPublished) {
    MockSensor s0{0, "temp", 25.0f, false};
    ISensor* sensors[Config::MAX_SENSORS] = {&s0};
    SensorManager sm{sensors, 1, nullptr, &bus};
    sm.pollOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::HEALTH));
    EXPECT_TRUE(bus.published[0].payload.find("HLT SENSORS") != std::string::npos);
}

TEST_F(SensorManagerTest, SingleSensorAlarmNotifiesBit0) {
    MockSensor s0{0, "proximity", 0.05f, true};
    ISensor* sensors[Config::MAX_SENSORS] = {&s0};
    SensorManager sm{sensors, 1, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &bits, 0);
    EXPECT_EQ(bits, 0x01u);
}

TEST_F(SensorManagerTest, SensorAtIndex3NotifiesBit3) {
    ISensor* sensors[Config::MAX_SENSORS] = {};
    MockSensor s3{3, "current", 10.0f, true};
    sensors[3] = &s3;
    SensorManager sm{sensors, 4, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &bits, 0);
    EXPECT_EQ(bits, 0x08u);
}

TEST_F(SensorManagerTest, FifteenSensorsNoneAlarming) {
    MockSensor sensors_storage[15] = {
        {0,"s0"}, {1,"s1"}, {2,"s2"}, {3,"s3"}, {4,"s4"},
        {5,"s5"}, {6,"s6"}, {7,"s7"}, {8,"s8"}, {9,"s9"},
        {10,"s10"},{11,"s11"},{12,"s12"},{13,"s13"},{14,"s14"}
    };
    ISensor* sensors[Config::MAX_SENSORS];
    for (int i = 0; i < 15; ++i) sensors[i] = &sensors_storage[i];
    SensorManager sm{sensors, 15, nullptr, &bus};
    sm.pollOnce();
    EXPECT_EQ(SensorManager::latestSnapshot.count, 15u);
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0, &bits, 0);
    EXPECT_EQ(bits, 0u);
}

TEST_F(SensorManagerTest, FifteenSensorsSensor7Alarming) {
    MockSensor sensors_storage[15] = {
        {0,"s0"}, {1,"s1"}, {2,"s2"}, {3,"s3"}, {4,"s4"},
        {5,"s5"}, {6,"s6"}, {7,"s7",0.f,true},
        {8,"s8"}, {9,"s9"}, {10,"s10"},{11,"s11"},{12,"s12"},{13,"s13"},{14,"s14"}
    };
    ISensor* sensors[Config::MAX_SENSORS];
    for (int i = 0; i < 15; ++i) sensors[i] = &sensors_storage[i];
    SensorManager sm{sensors, 15, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &bits, 0);
    EXPECT_EQ(bits, 0x80u);
}

TEST_F(SensorManagerTest, SnapshotValuesMatchSensorReads) {
    MockSensor s0{0, "temp", 42.5f, false};
    MockSensor s1{1, "prox", 0.1f,  false};
    ISensor* sensors[Config::MAX_SENSORS] = {&s0, &s1};
    SensorManager sm{sensors, 2, nullptr, &bus};
    sm.pollOnce();
    EXPECT_FLOAT_EQ(SensorManager::latestSnapshot.values[0], 42.5f);
    EXPECT_FLOAT_EQ(SensorManager::latestSnapshot.values[1], 0.1f);
}

TEST_F(SensorManagerTest, SnapshotAlarmsMatchSensorStates) {
    MockSensor s0{0, "a", 0.f, false};
    MockSensor s1{1, "b", 0.f, true};
    ISensor* sensors[Config::MAX_SENSORS] = {&s0, &s1};
    SensorManager sm{sensors, 2, nullptr, &bus};
    sm.pollOnce();
    EXPECT_FALSE(SensorManager::latestSnapshot.alarms[0]);
    EXPECT_TRUE(SensorManager::latestSnapshot.alarms[1]);
}

TEST_F(SensorManagerTest, SnapshotTimestampUpdated) {
    ISensor* sensors[Config::MAX_SENSORS] = {};
    SensorManager sm{sensors, 0, nullptr, &bus};
    sm.pollOnce();
    EXPECT_EQ(SensorManager::latestSnapshot.timestamp, 1000u);
}
