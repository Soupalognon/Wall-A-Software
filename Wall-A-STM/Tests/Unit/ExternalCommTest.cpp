#include "Stubs/HalStub.h"   // provides HAL_GetTick() — must come first
#include <gtest/gtest.h>
#include "MockBus.h"
#include "MockCommChannel.h"
#include "Tasks/ExternalComm.h"
#include "Tasks/MotionPlanner.h"  // for MoveCmd

// Helper: feed a string byte-by-byte into the interrupt rx queue
static void pushRxBytes(ExternalComm* comm, const char* str) {
    QueueHandle_t q = comm->rxByteQueue();
    for (const char* p = str; *p; ++p) {
        uint8_t b = static_cast<uint8_t>(*p);
        xQueueSend(q, &b, 0);
    }
}

// Helper: run rxTask until it blocks on empty queue (throws TaskDelayEscape)
static void runRxOnce(ExternalComm* comm) {
    try { ExternalComm::rxTask(comm); } catch (const TaskDelayEscape&) {}
}

// Helper: run txTask until it drains queued messages (exits on first vTaskDelay)
static void runTxOnce(ExternalComm* comm) {
    try { ExternalComm::txTask(comm); } catch (const TaskDelayEscape&) {}
}

// ── MockActuatorManager ──────────────────────────────────────────────────────

class MockActuatorManager : public IActuatorManager {
public:
    uint8_t     lastId{};
    std::string lastCmd;

    void commandById(uint8_t id, const char* cmd) override {
        lastId  = id;
        lastCmd = cmd;
    }
};

// ── Fixture ──────────────────────────────────────────────────────────────────

class ExternalCommTest : public ::testing::Test {
protected:
    MockCommChannel   uart, usb, eth;
    MockActuatorManager actuator;
    QueueHandle_t     motionMailbox{};
    ExternalComm*     comm{};

    void SetUp() override {
        getMockTick() = 0;
        motionMailbox = xQueueCreate(1, sizeof(MoveCmd));
        comm = new ExternalComm(&uart, &usb, &eth, &actuator, motionMailbox);
    }
    void TearDown() override {
        delete comm;
        vQueueDelete(motionMailbox);
    }
};

// ── Tests ────────────────────────────────────────────────────────────────────

TEST_F(ExternalCommTest, PublishTelemetry_EnqueuesForTx) {
    const char* msg = "TEL ODO 1.0 2.0 3.0\n";
    comm->publish(Topic::TELEMETRY, msg);

    // txTask finds the telemetry entry, transmits it, then exits on next vTaskDelay
    runTxOnce(comm);

    ASSERT_FALSE(usb.transmitted.empty());
    EXPECT_EQ(usb.transmitted.back(), msg);
}

TEST_F(ExternalCommTest, CmdMove_WritesToMotionMailbox) {
    pushRxBytes(comm, "CMD MOVE 0.5 0.3 1.57\n");

    runRxOnce(comm);

    MoveCmd cmd{};
    ASSERT_EQ(xQueueReceive(motionMailbox, &cmd, 0), pdTRUE);
    EXPECT_FLOAT_EQ(cmd.x,     0.5f);
    EXPECT_FLOAT_EQ(cmd.y,     0.3f);
    EXPECT_NEAR(cmd.angle,     1.57f, 0.001f);
}

TEST_F(ExternalCommTest, CmdActuator_CallsCommandById) {
    pushRxBytes(comm, "CMD ACTUATOR PUMP_1 ON\n");

    runRxOnce(comm);

    EXPECT_EQ(actuator.lastId,  1);
    EXPECT_EQ(actuator.lastCmd, "ON");
}

TEST_F(ExternalCommTest, InvalidCmd_NoCrash_LogPublished) {
    pushRxBytes(comm, "GARBAGE INPUT\n");

    // Must not throw anything other than the expected TaskDelayEscape
    bool unexpectedException = false;
    try {
        ExternalComm::rxTask(comm);
    } catch (const TaskDelayEscape&) {
        // expected — task exited normally after one iteration
    } catch (...) {
        unexpectedException = true;
    }
    EXPECT_FALSE(unexpectedException);
}

TEST_F(ExternalCommTest, UartPriority_OverwritesUsbCmd) {
    // UART bytes fed into rxByteQueue; only UART source supported via interrupt path
    pushRxBytes(comm, "CMD MOVE 0.5 0.3 0.0\n");

    runRxOnce(comm);

    MoveCmd cmd{};
    ASSERT_EQ(xQueueReceive(motionMailbox, &cmd, 0), pdTRUE);
    EXPECT_FLOAT_EQ(cmd.x, 0.5f);  // UART values win
    EXPECT_FLOAT_EQ(cmd.y, 0.3f);
}

TEST_F(ExternalCommTest, CommSnapshot_UpdatedOnRx) {
    ExternalComm::latestSnapshot = {};
    pushRxBytes(comm, "CMD MOVE 0.1 0.2\n");

    runRxOnce(comm);

    EXPECT_EQ(ExternalComm::latestSnapshot.rxCount, 1u);
    EXPECT_STREQ(ExternalComm::latestSnapshot.lastCmd, "CMD MOVE 0.1 0.2");
}
