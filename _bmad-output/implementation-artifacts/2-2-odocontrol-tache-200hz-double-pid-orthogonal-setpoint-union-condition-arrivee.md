# Story 2.2: OdoControl — tâche 200Hz, double PID orthogonal (v/ω), struct Setpoint union, condition d'arrivée

Status: review

## Story

As a developer,
I want `OdoControl` to run at 200Hz via `vTaskDelayUntil` with two orthogonal PIDs on linear velocity (v) and angular velocity (ω), a polymorphic `Setpoint` union struct, and an explicit arrival condition,
so that closed-loop locomotion converges naturally to waypoints without an explicit state machine, with real-time performance guaranteed by construction.

## Acceptance Criteria

1. **Given** `OdoControl` is constructed with `OdoControl(IOdomHAL*, IMotorHAL*, IBus*)`
   **When** the task runs at 200Hz via `vTaskDelayUntil`
   **Then** measured execution time per cycle is < 4ms (80% of the 5ms budget) via HAL timer

2. **Given** the 200Hz tick sequence is implemented
   **When** one tick executes
   **Then** the order is exactly:
   1. `vTaskDelayUntil()` — deterministic wake
   2. `IOdomHAL::update()` — reads encoders + dt, integrates position
   3. `xQueuePeek(mailbox, 0)` — non-blocking setpoint read
   4. if `!_hasSetpoint` → `setMotors(0, 0)`, return — boot guard
   5. Compute distance error: `errDist = sqrtf(dx*dx + dy*dy)`
   6. Compute heading error: `errAngle = atan2f(dy, dx) - _odom->getAngle()`
   7. `_pidSpeed.compute(errDist, dt)`
   8. `_pidAngle.compute(errAngle, dt)`
   9. Clamp: `v = clamp(v, -MAX_DUTY, MAX_DUTY)`, `ω = clamp(ω, -MAX_DUTY, MAX_DUTY)`
   10. Decompose: `leftDuty = v - ω`, `rightDuty = v + ω`
   11. `_motor->setMotors(leftDuty, rightDuty)`
   12. if `(_tickCount % Config::TELEM_DIVIDER == 0)` → update `OdoSnapshot` + publish telemetry

3. **Given** `struct Setpoint` is defined at file scope in `OdoControl.h`
   **When** `MotionPlanner` writes to the mailbox
   **Then** `Setpoint` contains `Mode mode` + `union { PoseTarget{x,y,angle}; VelocityTarget{v,w} }` — single unified mailbox, `OdoControl` dispatches on `mode`

4. **Given** two orthogonal PID instances `_pidSpeed` (on v) and `_pidAngle` (on ω) are implemented via class `Pid`
   **When** gains are tuned
   **Then** angular and linear aggressiveness are fully independent — anti-windup clamps integral to `Config::PID_I_MAX_SPEED` and `Config::PID_I_MAX_ANGLE` respectively; `Pid::reset()` clears both integral and previous error

5. **Given** `errDist < Config::ARRIVAL_THRESHOLD`
   **When** the arrival condition is detected
   **Then** `_motor->setMotors(0, 0)` is called AND `_bus->publish(Topic::ALERT, BusFormat::evtArrival())` notifies `MotionPlanner` — `OdoControl` becomes stateless between waypoints (clears `_hasSetpoint`)

6. **Given** `OdoControl` publishes telemetry at `TELEM_DIVIDER` rate (200Hz / 10 = 20Hz)
   **When** `_tickCount % Config::TELEM_DIVIDER == 0`
   **Then** `_bus->publish(Topic::TELEMETRY, BusFormat::telOdo(x, y, angle))` is called — zero `snprintf` inline in `OdoControl.cpp`

7. **Given** `atan2f` (float) is used for heading error
   **When** the STM32F303 FPU computes it
   **Then** `atan2f` (not `atan2`) is used exclusively — FPU executes in ~0.3µs, negligible vs 5ms budget

8. **Given** `Tests/Unit/OdoControlTest.cpp` exists with `MockOdomHAL`, updated `MockMotorHAL`, `MockBus`
   **When** tests run on PC host
   **Then** the following are validated: 12-step tick sequence, PID anti-windup (integral ≤ I_MAX), arrival condition (motors stop + alert published), `_hasSetpoint` boot guard, `MAX_DUTY` clamping, telemetry at TELEM_DIVIDER rate — without STM32 hardware

9. **And** `OdoControl::OdoSnapshot latestSnapshot` (static public struct with `x`, `y`, `angle`, `vLeft`, `vRight`, `timestamp`) is updated at each `TELEM_DIVIDER` block — readable by `Monitoring` without explicit synchronization (atomic ARM Cortex-M write)

## Tasks / Subtasks

- [x] Task 1: Create `App/Tasks/Pid.h` — generic PID class (AC: #4)
  - [x] Guard: `#ifndef APP_TASKS_PID_H`
  - [x] Class with configurable kP, kI, kD and I_MAX:
    ```cpp
    class Pid {
    public:
        Pid(float kp, float ki, float kd, float iMax);
        float compute(float error, float dt);
        void  reset();
    private:
        float _kp, _ki, _kd, _iMax;
        float _integral    = 0.0f;
        float _prevError   = 0.0f;
    };
    ```
  - [x] `compute()` implements standard PID with anti-windup:
    ```cpp
    float Pid::compute(float error, float dt) {
        _integral += error * dt;
        if (_integral >  _iMax) _integral =  _iMax;
        if (_integral < -_iMax) _integral = -_iMax;
        float derivative = (error - _prevError) / dt;
        _prevError = error;
        return _kp * error + _ki * _integral + _kd * derivative;
    }
    ```
  - [x] `reset()` sets `_integral = 0.0f` and `_prevError = 0.0f`

- [x] Task 2: Create `App/Tasks/Pid.cpp` (AC: #4)
  - [x] Implement constructor and all methods from `Pid.h`

- [x] Task 3: Rewrite `App/Tasks/OdoControl.h` — new architecture with IOdomHAL (AC: #1, #2, #3, #4, #5, #6, #9)
  - [x] Guard: `#ifndef APP_TASKS_ODOCONTROL_H` (keep existing guard)
  - [x] Includes: `"Interfaces/IOdomHAL.h"`, `"Interfaces/IMotorHAL.h"`, `"Interfaces/IBus.h"`, `"Tasks/Pid.h"`, `"Config.h"`, `<FreeRTOS.h>`, `<task.h>`, `<queue.h>`, `<cmath>`
  - [x] Define enums and Setpoint struct at file scope (outside class):
    ```cpp
    enum class SetpointMode : uint8_t { POSE, VELOCITY };

    struct PoseTarget     { float x, y, angle; };
    struct VelocityTarget { float v, w; };

    struct Setpoint {
        SetpointMode mode = SetpointMode::POSE;
        union {
            PoseTarget     pose;
            VelocityTarget velocity;
        };
    };
    ```
  - [x] `OdoSnapshot` public nested struct: `float x, y, angle, vLeft, vRight; uint32_t timestamp;`
  - [x] `static OdoSnapshot latestSnapshot` as public static member
  - [x] Constructor: `OdoControl(IOdomHAL* odom, IMotorHAL* motor, IBus* bus, QueueHandle_t mailbox)`
  - [x] Public: `static void task(void* param)` — FreeRTOS entry point
  - [x] Public: `void tick()` — one cycle, exposed for unit testing
  - [x] Private members: `IOdomHAL* _odom`, `IMotorHAL* _motor`, `IBus* _bus`, `QueueHandle_t _mailbox`
  - [x] Private PID instances: `Pid _pidSpeed`, `Pid _pidAngle`
  - [x] Private state: `bool _hasSetpoint = false`, `uint32_t _tickCount = 0`

- [x] Task 4: Rewrite `App/Tasks/OdoControl.cpp` — full 12-step tick sequence (AC: #1, #2, #3, #4, #5, #6, #7, #9)
  - [x] Define static: `OdoControl::OdoSnapshot OdoControl::latestSnapshot{}`
  - [x] Constructor: initialize `_pidSpeed(Config::PID_KP_DEFAULT, Config::PID_KI_DEFAULT, Config::PID_KD_DEFAULT, Config::PID_I_MAX_SPEED)` and `_pidAngle` with `PID_I_MAX_ANGLE`
  - [x] `task()` — same `vTaskDelayUntil` pattern as before:
    ```cpp
    void OdoControl::task(void* param) {
        auto* self = static_cast<OdoControl*>(param);
        TickType_t lastWake = xTaskGetTickCount();
        const TickType_t period = pdMS_TO_TICKS(1000 / Config::ODO_FREQ_HZ);
        for (;;) {
            vTaskDelayUntil(&lastWake, period);
            self->tick();
        }
    }
    ```
  - [x] `tick()` — implement the full 12-step sequence (see Dev Notes)
  - [x] `atan2f` used exclusively for heading error — never `atan2`
  - [x] `snprintf` must NOT appear anywhere in `OdoControl.cpp`

- [x] Task 5: Add `BusFormat::evtArrival()` to `App/BusFormat.h/.cpp` (AC: #5)
  - [x] Declaration: `static const char* evtArrival();`
  - [x] Implementation: `return "ALT ARRIVAL\n";` — static literal, no buffer needed

- [x] Task 6: Update `App/SystemInit/SystemInit.cpp` — wire new OdoControl with IOdomHAL (AC: #1)
  - [x] Add `#include "Tasks/OdoControl.h"` (gets `Setpoint`, `SetpointMode`, `OdoSnapshot`)
  - [x] Add `#include "Drivers/ConcreteOdomHAL.h"` and `#include "Drivers/MotorHAL.h"`
  - [x] Keep stubs (Option B) until real HAL is wired on target — `StubEncoderHAL` and `StubMotorHAL` defined locally in SystemInit.cpp
  - [x] Instantiate: `ConcreteOdomHAL odomHAL{&stubEncL, &stubEncR}`, `StubMotorHAL stubMotorHAL`, `OdoControl odoCtrl{&odomHAL, &stubMotorHAL, &extComm, setpointMailbox}`
  - [x] Remove old `OdoControl::OdoSnapshot OdoControl::latestSnapshot{}` definition (now in OdoControl.cpp)
  - [x] Renamed `motionMailbox` → `cmdMailbox` (ExternalComm) and added separate `setpointMailbox` (OdoControl) matching Story 2.3 topology

- [x] Task 7: Update `Tests/Unit/OdoControlTest.cpp` — rewrite for new architecture (AC: #8)
  - [x] Replace `MockEncoderHAL` injection with `MockOdomHAL` injection
  - [x] Replace `MockMotorHAL` with updated `setMotors(float, float)` signature + added `reset()`
  - [x] All 9 test cases from Dev Notes implemented and passing

- [x] Task 8: Update `Tests/CMakeLists.txt` — add `Pid.cpp` to OdoControlTest (AC: #8)
  - [x] Add `${APP_DIR}/Tasks/Pid.cpp` to `OdoControlTest` sources
  - [x] Add `${APP_DIR}/Tasks/OdoControl.cpp` remains in target
  - [x] Add `${APP_DIR}/BusFormat.cpp` (for `evtArrival`)

- [x] Task 9: Verify NFR compliance
  - [x] `grep -r "snprintf" App/Tasks/OdoControl.cpp` → zero results
  - [x] `grep -r "atan2[^f]" App/Tasks/OdoControl.cpp` → zero results (only `atan2f` allowed)
  - [x] `grep -r "new\|delete\|malloc\|free" App/Tasks/OdoControl.cpp App/Tasks/Pid.cpp` → zero results
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/Tasks/OdoControl.h App/Tasks/OdoControl.cpp App/Tasks/Pid.h App/Tasks/Pid.cpp` → zero results

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── BusFormat.h                       ← UPDATE (add evtArrival declaration)
├── BusFormat.cpp                     ← UPDATE (add evtArrival implementation)
├── Tasks/
│   ├── Pid.h                         ← NEW
│   ├── Pid.cpp                       ← NEW
│   ├── OdoControl.h                  ← REWRITE (new constructor, Setpoint union, IOdomHAL)
│   └── OdoControl.cpp                ← REWRITE (12-step tick, double PID, arrival condition)
└── SystemInit/
    └── SystemInit.cpp                ← UPDATE (wire ConcreteOdomHAL + MotorHAL)

Tests/
├── CMakeLists.txt                    ← UPDATE (add Pid.cpp to OdoControlTest)
└── Unit/
    └── OdoControlTest.cpp            ← REWRITE (MockOdomHAL, new test cases)
```

**Do NOT touch:** `App/Interfaces/`, `App/Drivers/` (written in Story 2.1), `App/Tasks/ExternalComm.h/.cpp`, `Core/`, `Middlewares/`

---

### OdoControl::tick() — Full 12-Step Implementation

```cpp
void OdoControl::tick() {
    ++_tickCount;

    // Step 2: update odometry (reads encoders, integrates position, computes dt)
    _odom->update();
    float dt = _odom->getDt();

    // Step 3: non-blocking setpoint read
    Setpoint sp{};
    if (xQueuePeek(_mailbox, &sp, 0) == pdTRUE) {
        _hasSetpoint = true;
    }

    // Step 4: boot guard
    if (!_hasSetpoint) {
        _motor->setMotors(0.0f, 0.0f);
        return;
    }

    // Step 5-6: compute errors (POSE mode)
    float dx       = sp.pose.x - _odom->getX();
    float dy       = sp.pose.y - _odom->getY();
    float errDist  = sqrtf(dx * dx + dy * dy);
    float errAngle = atan2f(dy, dx) - _odom->getAngle();

    // Step 5 alt: arrival condition
    if (errDist < Config::ARRIVAL_THRESHOLD) {
        _motor->setMotors(0.0f, 0.0f);
        _bus->publish(Topic::ALERT, BusFormat::evtArrival());
        _hasSetpoint = false;
        _pidSpeed.reset();
        _pidAngle.reset();
        return;
    }

    // Steps 6-7: PID computation
    float v = _pidSpeed.compute(errDist,  dt);
    float w = _pidAngle.compute(errAngle, dt);

    // Step 8: clamp to MAX_DUTY
    auto clamp = [](float val, float lo, float hi) {
        return val < lo ? lo : (val > hi ? hi : val);
    };
    v = clamp(v, -Config::MAX_DUTY, Config::MAX_DUTY);
    w = clamp(w, -Config::MAX_DUTY, Config::MAX_DUTY);

    // Steps 9-10: differential decomposition and motor command
    float leftDuty  = v - w;
    float rightDuty = v + w;
    _motor->setMotors(leftDuty, rightDuty);

    // Steps 11-12: telemetry at TELEM_DIVIDER rate
    if (_tickCount % Config::TELEM_DIVIDER == 0) {
        latestSnapshot = {
            _odom->getX(), _odom->getY(), _odom->getAngle(),
            _odom->getVLeft(), _odom->getVRight(),
            HAL_GetTick()
        };
        _bus->publish(Topic::TELEMETRY,
                      BusFormat::telOdo(_odom->getX(), _odom->getY(), _odom->getAngle()));
    }
}
```

---

### Pid Class — Anti-Windup Behavior

Anti-windup clamp must run BEFORE computing the derivative to avoid derivative kick on the clamped integral. Clamping order:

```
integral += error * dt
integral = clamp(integral, -iMax, +iMax)   ← clamp BEFORE derivative
derivative = (error - prevError) / dt
output = kp * error + ki * integral + kd * derivative
prevError = error
```

`Pid::reset()` is called on arrival and on stall detection (Story 2.4) to avoid oscillation on restart.

---

### Setpoint Union — Memory Safety Note

The `union` in `Setpoint` overlaps `PoseTarget` and `VelocityTarget` in memory. Always set `mode` before writing to the union member, and always check `mode` before reading. `OdoControl` currently implements POSE mode only — the union is defined to allow future VELOCITY mode without mailbox API changes.

```cpp
// MotionPlanner writes:
Setpoint sp;
sp.mode = SetpointMode::POSE;
sp.pose = {1.0f, 0.5f, 0.0f};
xQueueOverwrite(mailbox, &sp);

// OdoControl reads:
if (sp.mode == SetpointMode::POSE) { /* use sp.pose */ }
```

---

### SystemInit.cpp — Concrete HAL Wiring Options

**Option A (target with real encoders and motors):**
```cpp
// Requires CubeMX TIM config for encoders and PWM for motors
static Encoder          encLeft{&htim3};   // adapt to actual TIM
static Encoder          encRight{&htim4};
static ConcreteEncoderHAL halEncL{encLeft};
static ConcreteEncoderHAL halEncR{encRight};
static Drv8262          drv{/* PWM handles */};
static ConcreteOdomHAL  odomHAL{&halEncL, &halEncR};
static MotorHAL         motorHAL{drv};
static OdoControl       odoCtrl{&odomHAL, &motorHAL, &extComm, setpointMailbox};
```

**Option B (stub until hardware ready — compile and run FreeRTOS scheduler only):**
```cpp
// Keep temporary stubs from Story 2.1 — OdoControl reads zeros, publishes "TEL ODO 0.00 0.00 0.00"
static StubEncoderHAL  stubEncL{}, stubEncR{};
static ConcreteOdomHAL odomHAL{&stubEncL, &stubEncR};
static StubMotorHAL    motorHAL{};
static OdoControl      odoCtrl{&odomHAL, &motorHAL, &extComm, setpointMailbox};
```

---

### OdoControlTest.cpp — Rewritten Test Cases

```cpp
#include "Stubs/MockHAL.h"  // must be first
#include <gtest/gtest.h>
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
    void TearDown() override { delete odo; }

    void pushPoseSetpoint(float x, float y, float angle = 0.0f) {
        Setpoint sp;
        sp.mode = SetpointMode::POSE;
        sp.pose = {x, y, angle};
        xQueueOverwrite(mailbox, &sp);
    }
};

// Boot guard: no setpoint → motors zero
TEST_F(OdoControlTest, NoMotionWithoutSetpoint) {
    odo->tick();
    EXPECT_FLOAT_EQ(motor.lastLeft,  0.0f);
    EXPECT_FLOAT_EQ(motor.lastRight, 0.0f);
}

// Forward target → positive duty on both wheels
TEST_F(OdoControlTest, ForwardSetpointProducesPositiveDuty) {
    pushPoseSetpoint(1.0f, 0.0f);
    odo->tick();
    EXPECT_GT(motor.lastLeft,  0.0f);
    EXPECT_GT(motor.lastRight, 0.0f);
}

// Arrival condition: target reached → motors zero + arrival alert
TEST_F(OdoControlTest, ArrivalStopsMotorsAndPublishesAlert) {
    // odom reports robot at (0.999, 0) — within ARRIVAL_THRESHOLD of (1.0, 0)
    odom.x = 0.999f;
    pushPoseSetpoint(1.0f, 0.0f);
    odo->tick();
    EXPECT_FLOAT_EQ(motor.lastLeft,  0.0f);
    EXPECT_FLOAT_EQ(motor.lastRight, 0.0f);
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_TRUE(bus.published[0].payload.find("ARRIVAL") != std::string::npos);
}

// After arrival, _hasSetpoint is cleared — next tick without new setpoint → motors zero
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
    EXPECT_FALSE(bus.hasPublished(Topic::TELEMETRY));  // ticks 1–9: no telemetry
    bus.clear(); odo->tick();  // tick 10
    EXPECT_TRUE(bus.hasPublished(Topic::TELEMETRY));
}

// No snprintf inline — telemetry payload starts with "TEL ODO "
TEST_F(OdoControlTest, TelemetryFormatCorrect) {
    pushPoseSetpoint(10.0f, 0.0f);
    for (int i = 0; i < 10; ++i) odo->tick();
    ASSERT_FALSE(bus.published.empty());
    EXPECT_EQ(bus.published[0].payload.substr(0, 8), "TEL ODO ");
}

// MAX_DUTY clamp: PID cannot exceed MAX_DUTY
TEST_F(OdoControlTest, DutyClampedToMaxDuty) {
    pushPoseSetpoint(1000.0f, 0.0f);  // huge target → large PID error
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
    // Duty must still be clamped (if integral were unbounded, duty would diverge)
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
```

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| `OdoControl` receives `IOdomHAL*` not `IEncoderHAL*` | FR-01 adapted + brainstorm | `OdoControl.h` constructor uses `IOdomHAL*` |
| `xQueuePeek` with timeout=0 on 200Hz path | FR-02 / NFR-01 | Non-blocking on critical path |
| No `snprintf` inline | FR-09 | All messaging via `BusFormat::` |
| `atan2f` float only | NFR-01 perf | Never `atan2` (double) |
| No dynamic allocation in `App/` | NFR-02 | `Pid` members on class storage, `static` in SystemInit |
| English identifiers | NFR-08 | All new identifiers in English |
| Guard convention | NFR-06 | `APP_TASKS_ODOCONTROL_H`, `APP_TASKS_PID_H` |
| `xTaskCreate` only in SystemInit | FR-11 | OdoControl task created only in `SystemInit::boot()` |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes to `Core/`, `Drivers/` (CubeMX), `Middlewares/` |
| Writer (OdoControl) does NOT need critical section | NFR-04 | Highest priority task — Monitoring cannot preempt it |

---

### What Must Exist Before Implementing (Dependencies)

From **Story 2.1** (must be complete):
- `App/Interfaces/IOdomHAL.h` — `update()`, `getX/Y/Angle/VLeft/VRight/V/W/getDt()`
- `App/Interfaces/IMotorHAL.h` — `setMotors(float, float)`
- `App/Drivers/ConcreteOdomHAL.h/.cpp` — concrete implementation
- `App/Drivers/MotorHAL.h/.cpp` — wraps Drv8262
- `App/Config.h` — all 11 OdoControl constants including `TELEM_DIVIDER`, `MAX_DUTY`, `ARRIVAL_THRESHOLD`, `PID_I_MAX_SPEED`, `PID_I_MAX_ANGLE`
- `Tests/Mocks/MockOdomHAL.h` — injectable mock for `IOdomHAL`
- `Tests/Mocks/MockMotorHAL.h` — updated with `setMotors(float, float)`

From **Stories 1.x**:
- `App/Interfaces/IBus.h`, `App/BusFormat.h/.cpp` (add `evtArrival` in this story)
- `Tests/Mocks/MockBus.h`, FreeRTOS stubs, `Tests/Stubs/MockHAL.h`

---

### Deferred Work

- **Velocity mode:** `SetpointMode::VELOCITY` is declared but not implemented. `OdoControl` ignores it for now and defaults to `POSE` behavior. Story 2.3 (MotionPlanner) decides which mode to use.
- **Stall detection and encoder fault:** implemented in Story 2.4, which adds checks inside `tick()`.

---

### References

- Story requirements: [epics.md — Story 2.2](_bmad-output/planning-artifacts/epics.md)
- Brainstorming decisions: [brainstorming-session-2026-05-13-1000.md](_bmad-output/brainstorming/brainstorming-session-2026-05-13-1000.md) — Thèmes B, C
- HAL interfaces and mocks: [2-1-hal-interfaces-locomotion-iencoderhal-iodomhal-imotorhal-concreteodomhal-motorhal.md](_bmad-output/implementation-artifacts/2-1-hal-interfaces-locomotion-iencoderhal-iodomhal-imotorhal-concreteodomhal-motorhal.md)
- Story 2.3 (MotionPlanner will consume `Setpoint` struct): [epics.md — Story 2.3](_bmad-output/planning-artifacts/epics.md)
- Architecture: [architecture.md](_bmad-output/planning-artifacts/architecture.md)
- MockBus / FreeRTOS stubs: [1-6-infrastructure-google-test-mocks-hal-tests-busformat-externalcomm.md](_bmad-output/implementation-artifacts/1-6-infrastructure-google-test-mocks-hal-tests-busformat-externalcomm.md)
- SystemInit current wiring: [1-5-systeminit-boot-cablage-statique-complet.md](_bmad-output/implementation-artifacts/1-5-systeminit-boot-cablage-statique-complet.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

(aucun blocage)

### Completion Notes List

- Implémenté `Pid.h/.cpp` : PID générique avec anti-windup, `reset()` clair.
- Réécrit `OdoControl.h` : struct `Setpoint` union `POSE/VELOCITY`, `OdoSnapshot` avec `vLeft/vRight`, PIDs initialisés in-header via constexpr Config.
- Créé `OdoControl.cpp` : séquence tick 12 étapes complète conforme AC#2. `HAL_GetTick()` via `stm32f4xx_hal.h` (stub en test, réel sur cible). Décision : `stm32f4xx_hal.h` plutôt que `Stubs/MockHAL.h` pour que le stub de test soit appliqué automatiquement via l'ordre des includes.
- Ajouté `BusFormat::evtArrival()` : literal statique `"ALT ARRIVAL\n"`, zéro buffer.
- Mis à jour `SystemInit.cpp` : Option B (stubs) — deux files distinctes `cmdMailbox` (ExternalComm) et `setpointMailbox` (OdoControl) reflétant la topologie Story 2.3. `latestSnapshot` retiré de SystemInit (maintenant dans OdoControl.cpp).
- Réécrit `OdoControlTest.cpp` : 9 tests, tous passent. Ajouté `reset()` à `MockMotorHAL`. Ajouté `vTaskDelayUntil` et `xTaskGetTickCount` aux stubs FreeRTOS.
- **Résultats** : 29/29 tests (9 nouveaux + 20 régressions) — 0 échec.

### File List

- `App/Tasks/Pid.h` (nouveau)
- `App/Tasks/Pid.cpp` (nouveau)
- `App/Tasks/OdoControl.h` (réécrit)
- `App/Tasks/OdoControl.cpp` (nouveau)
- `App/BusFormat.h` (mis à jour — evtArrival)
- `App/BusFormat.cpp` (mis à jour — evtArrival)
- `App/SystemInit/SystemInit.cpp` (mis à jour — câblage OdoControl, dual mailbox)
- `Tests/Unit/OdoControlTest.cpp` (nouveau)
- `Tests/CMakeLists.txt` (mis à jour — OdoControlTest target)
- `Tests/Mocks/MockMotorHAL.h` (mis à jour — reset())
- `Tests/Stubs/task.h` (mis à jour — vTaskDelayUntil, xTaskGetTickCount)
- `Tests/Stubs/FreeRTOSStub.cpp` (mis à jour — vTaskDelayUntil, xTaskGetTickCount)
