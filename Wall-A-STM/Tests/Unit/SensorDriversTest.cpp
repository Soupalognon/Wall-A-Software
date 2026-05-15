#include <gtest/gtest.h>
#include "Mocks/MockSensorHAL.h"
#include "Drivers/ProximitySensor.h"
#include "Drivers/TemperatureSensor.h"
#include "Drivers/CurrentSensor.h"

// ── ProximitySensor ──────────────────────────────────────────────────────────

TEST(ProximitySensorTest, ProxRead_DelegatesToHAL) {
    MockSensorHAL hal;
    hal.returnValue = 0.35f;
    ProximitySensor sensor{1, &hal};
    EXPECT_FLOAT_EQ(0.35f, sensor.read());
}

TEST(ProximitySensorTest, ProxAlarm_WhenTooClose) {
    MockSensorHAL hal;
    hal.returnValue = 0.10f;
    ProximitySensor sensor{1, &hal};
    sensor.read();
    EXPECT_TRUE(sensor.isAlarm());
}

TEST(ProximitySensorTest, ProxNoAlarm_WhenFar) {
    MockSensorHAL hal;
    hal.returnValue = 0.30f;
    ProximitySensor sensor{1, &hal};
    sensor.read();
    EXPECT_FALSE(sensor.isAlarm());
}

TEST(ProximitySensorTest, ProxId_IsCorrect) {
    MockSensorHAL hal;
    ProximitySensor sensor{3, &hal};
    EXPECT_EQ(3u, sensor.id());
    EXPECT_STREQ("PROXIMITY", sensor.name());
}

// ── TemperatureSensor ────────────────────────────────────────────────────────

TEST(TemperatureSensorTest, TempRead_DelegatesToHAL) {
    MockSensorHAL hal;
    hal.returnValue = 25.0f;
    TemperatureSensor sensor{2, &hal};
    EXPECT_FLOAT_EQ(25.0f, sensor.read());
}

TEST(TemperatureSensorTest, TempAlarm_WhenOverThreshold) {
    MockSensorHAL hal;
    hal.returnValue = 75.0f;
    TemperatureSensor sensor{2, &hal};
    sensor.read();
    EXPECT_TRUE(sensor.isAlarm());
}

TEST(TemperatureSensorTest, TempNoAlarm_WhenCool) {
    MockSensorHAL hal;
    hal.returnValue = 40.0f;
    TemperatureSensor sensor{2, &hal};
    sensor.read();
    EXPECT_FALSE(sensor.isAlarm());
}

TEST(TemperatureSensorTest, TempId_IsCorrect) {
    MockSensorHAL hal;
    TemperatureSensor sensor{2, &hal};
    EXPECT_EQ(2u, sensor.id());
    EXPECT_STREQ("TEMPERATURE", sensor.name());
}

// ── CurrentSensor ────────────────────────────────────────────────────────────

TEST(CurrentSensorTest, CurrRead_DelegatesToHAL) {
    MockSensorHAL hal;
    hal.returnValue = 1.5f;
    CurrentSensor sensor{3, &hal};
    EXPECT_FLOAT_EQ(1.5f, sensor.read());
}

TEST(CurrentSensorTest, CurrAlarm_WhenOverThreshold) {
    MockSensorHAL hal;
    hal.returnValue = 3.0f;
    CurrentSensor sensor{3, &hal};
    sensor.read();
    EXPECT_TRUE(sensor.isAlarm());
}

TEST(CurrentSensorTest, CurrNoAlarm_WhenNormal) {
    MockSensorHAL hal;
    hal.returnValue = 1.0f;
    CurrentSensor sensor{3, &hal};
    sensor.read();
    EXPECT_FALSE(sensor.isAlarm());
}

TEST(CurrentSensorTest, CurrId_IsCorrect) {
    MockSensorHAL hal;
    CurrentSensor sensor{5, &hal};
    EXPECT_EQ(5u, sensor.id());
    EXPECT_STREQ("CURRENT", sensor.name());
}
