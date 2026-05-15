#include "Stubs/HalStub.h"
#include <gtest/gtest.h>
#include "Stubs/FreeRTOS.h"
#include "Mocks/MockBus.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Tasks/Monitoring.h"

class MonitoringTest : public ::testing::Test {
protected:
    MockBus bus;
    void SetUp() override {
        bus.clear();
        setMockTick(0);
        OdoControl::latestSnapshot    = {};
        SensorManager::latestSnapshot = {};
        ExternalComm::latestSnapshot  = {};
    }
};

TEST_F(MonitoringTest, OdoFreshNoAlert) {
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
}

TEST_F(MonitoringTest, OdoStalePublishesAltStaleOdo) {
    setMockTick(501);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "ODO"));
}

TEST_F(MonitoringTest, SensorStalePublishesAltStaleSensor) {
    setMockTick(501);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "SENSOR"));
}

TEST_F(MonitoringTest, CommStalePublishesAltStaleComm) {
    setMockTick(501);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "COMM"));
}

TEST_F(MonitoringTest, AllFreshNoAlert) {
    setMockTick(499);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_EQ(0u, bus.count(Topic::ALERT));
}

TEST_F(MonitoringTest, AllStaleThreeAlerts) {
    setMockTick(501);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_EQ(3u, bus.count(Topic::ALERT));
}

TEST_F(MonitoringTest, OnlyOdoStaleOneAlert) {
    setMockTick(501);
    SensorManager::latestSnapshot.timestamp = 501;
    ExternalComm::latestSnapshot.timestamp  = 501;
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_EQ(1u, bus.count(Topic::ALERT));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "ODO"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "SENSOR"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "COMM"));
}

TEST_F(MonitoringTest, StaleThresholdBoundary) {
    setMockTick(500);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));

    bus.clear();
    setMockTick(501);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));
}
