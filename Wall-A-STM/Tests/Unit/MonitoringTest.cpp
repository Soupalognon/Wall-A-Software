#include "Stubs/HalStub.h"
#include <gtest/gtest.h>
#include "Stubs/FreeRTOS.h"
#include "Mocks/MockBus.h"
#include "Mocks/MockKernelHAL.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Tasks/Monitoring.h"

class MonitoringTest : public ::testing::Test {
protected:
    MockBus bus;
    MockKernelHAL kernel;
    void SetUp() override {
        bus.clear();
        setMockTick(0);
        OdoControl::latestSnapshot    = {};
        SensorManager::latestSnapshot = {};
        ExternalComm::latestSnapshot  = {};
        // Healthy kernel by default: one task with plenty of stack, high heap.
        kernel.heapFree    = 16384;
        kernel.heapMinFree = 8192;
        kernel.taskList.clear();
        kernel.addTask("OdoCtrl", 256);
    }
};

TEST_F(MonitoringTest, OdoFreshNoAlert) {
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
}

TEST_F(MonitoringTest, OdoStalePublishesAltStaleOdo) {
    setMockTick(501);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "ODO"));
}

TEST_F(MonitoringTest, SensorStalePublishesAltStaleSensor) {
    setMockTick(501);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "SENSOR"));
}

TEST_F(MonitoringTest, CommStalePublishesAltStaleComm) {
    setMockTick(501);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "COMM"));
}

TEST_F(MonitoringTest, AllFreshNoAlert) {
    setMockTick(499);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_EQ(0u, bus.count(Topic::ALERT));
}

TEST_F(MonitoringTest, AllStaleThreeAlerts) {
    setMockTick(501);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_EQ(3u, bus.count(Topic::ALERT));
}

TEST_F(MonitoringTest, OnlyOdoStaleOneAlert) {
    setMockTick(501);
    SensorManager::latestSnapshot.timestamps[0] = 501;
    ExternalComm::latestSnapshot.timestamp      = 501;
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_EQ(1u, bus.count(Topic::ALERT));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "ODO"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "SENSOR"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "COMM"));
}

TEST_F(MonitoringTest, StaleThresholdBoundary) {
    setMockTick(500);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));

    bus.clear();
    setMockTick(501);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));
}

// ── FreeRTOS kernel health ────────────────────────────────────────────────

TEST_F(MonitoringTest, HealthyKernelNoRtosAlert) {
    kernel.taskList.clear();
    kernel.addTask("OdoCtrl",  256);
    kernel.addTask("SensorMgr", 128);
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "STACK_LOW"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "HEAP_LOW"));
}

TEST_F(MonitoringTest, StackLowPublishesAlert) {
    kernel.taskList.clear();
    kernel.addTask("MoPlan", 10);   // below RTOS_STACK_WARN_WORDS (48)
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_EQ(1u, bus.count(Topic::ALERT));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "STACK_LOW"));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "MoPlan"));
}

TEST_F(MonitoringTest, HeapLowPublishesAlert) {
    kernel.heapFree = 1000;         // below RTOS_HEAP_WARN_BYTES (2048)
    Monitoring mon(&bus, &kernel);
    mon.checkOnce();
    EXPECT_EQ(1u, bus.count(Topic::ALERT));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "HEAP_LOW"));
}

TEST_F(MonitoringTest, HeapLowAlertReArmsAfterRecovery) {
    Monitoring mon(&bus, &kernel);
    kernel.heapFree = 1000;         // dip below threshold
    mon.checkOnce();
    mon.checkOnce();                // still low → no repeat
    EXPECT_EQ(1u, bus.count(Topic::ALERT));

    kernel.heapFree = 16384;        // recover → re-arm
    mon.checkOnce();
    kernel.heapFree = 1000;         // dip again → new alert
    mon.checkOnce();
    EXPECT_EQ(2u, bus.count(Topic::ALERT));
}

TEST_F(MonitoringTest, RtosHealthThrottled) {
    Monitoring mon(&bus, &kernel);
    for (int i = 0; i < Config::RTOS_HEALTH_DIVIDER; i++)
        mon.checkOnce();

    size_t heapHealth = 0;
    for (const auto& p : bus.published)
        if (p.topic == Topic::HEALTH && p.payload.find("RTOS_HEAP") != std::string::npos)
            ++heapHealth;
    EXPECT_EQ(1u, heapHealth);
}
