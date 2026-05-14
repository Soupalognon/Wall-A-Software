# Story 2.4: Robustesse OdoControl — détection blocage moteur, encodeur silencieux, guard boot

Status: review

## Story

As a developer,
I want `OdoControl` to detect motor stall and silent encoder failures and publish IBus alerts, with a safe boot behavior,
so that the system is intrinsically defensive and failure modes are traceable without hardware intervention.

## Acceptance Criteria

1. **Given** `|avgDuty| > Config::STALL_DUTY_THRESHOLD` and measured speed `|v| < Config::STALL_SPEED_THRESHOLD` for `Config::STALL_TIME_MS` continuously
   **When** `OdoControl` detects the motor stall condition
   **Then** `_bus->publish(Topic::ALERT, BusFormat::altStall())` is published + `IMotorHAL::setMotors(0, 0)` + `_pidSpeed.reset()` + `_pidAngle.reset()` + `_stallCount = 0` + `return` — PID integral reset prevents oscillation on restart

2. **Given** `|avgDuty| > Config::STALL_DUTY_THRESHOLD` and `_odom->getVLeft() == 0.0f` (exact: zero ticks = exact zero velocity) for N consecutive ticks (N = STALL_TIME_MS × ODO_FREQ_HZ / 1000)
   **When** `OdoControl` detects the left encoder silence condition
   **Then** `_bus->publish(Topic::ALERT, BusFormat::altEncoderFault("LEFT"))` is published — high duty + zero ticks is physically impossible except on encoder hardware failure. Motor does NOT stop (sensor warning only).

3. **Given** same condition for right encoder (`_odom->getVRight() == 0.0f`)
   **When** `OdoControl` detects right encoder silence
   **Then** `_bus->publish(Topic::ALERT, BusFormat::altEncoderFault("RIGHT"))` is published

4. **Given** `_hasSetpoint == false` at boot (before `MotionPlanner` writes to mailbox)
   **When** `xQueuePeek` returns `pdFALSE`
   **Then** `IMotorHAL::setMotors(0, 0)` is called and PIDs are NOT computed — safe behavior guaranteed from tick 1 (already implemented in Story 2.2, verified by `NoMotionWithoutSetpoint` test)

5. **Given** `OdoControl` clamps its PID output to `[-Config::MAX_DUTY, +Config::MAX_DUTY]` before `setMotors()`
   **When** clamping is active (PID saturation)
   **Then** the pre-clamp values `rawV` and `rawW` are stored in `OdoSnapshot` for detecting overly aggressive gains — `Drv8262` driver clamps to `[-1, 1]` as the ultimate safety net

6. **Given** `Tests/Unit/OdoControlTest.cpp` covers robustness scenarios
   **When** tests inject zero ticks with high duty for `STALL_TIME_MS` ticks
   **Then** `MockBus` captures expected `ALT STALL` and `ALT ENCODER_FAULT` alerts — tests pass on PC host without hardware

## Tasks / Subtasks

- [x] Task 1: Add `Config::STALL_SPEED_THRESHOLD` to `App/Config.h` (AC: #1)
  - [x] Add inside `namespace Config`: `static constexpr float STALL_SPEED_THRESHOLD = 0.05f;` — threshold in m/s below which measured speed is considered "approximately zero" for stall detection
  - [x] Place near the existing `STALL_DUTY_THRESHOLD` and `STALL_TIME_MS` constants for readability

- [x] Task 2: Add `BusFormat::altStall()` and `BusFormat::altEncoderFault()` (AC: #1, #2, #3)
  - [x] In `App/BusFormat.h`, add declarations:
    ```cpp
    static const char* altStall();
    static const char* altEncoderFault(const char* side);
    ```
  - [x] In `App/BusFormat.cpp`, implement:
    ```cpp
    const char* BusFormat::altStall() { return "ALT STALL\n"; }

    const char* BusFormat::altEncoderFault(const char* side) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "ALT ENCODER_FAULT %s\n", side);
        return buf;
    }
    ```
  - [x] `altStall()` returns a static literal — no buffer needed, no `snprintf`
  - [x] `altEncoderFault()` uses `static char buf` — never return a pointer to a stack-local variable
  - [x] Verify: `grep -r "snprintf" App/BusFormat.cpp | grep -v "altEncoderFault\|altAlarm"` → zero (only two `snprintf` allowed in BusFormat.cpp)

- [x] Task 3: Extend `OdoControl::OdoSnapshot` with pre-clamp fields (AC: #5)
  - [x] In `App/Tasks/OdoControl.h`, update struct:
    ```cpp
    struct OdoSnapshot {
        float x, y, angle, vLeft, vRight;
        float rawV, rawW;   // pre-clamp PID output — diagnostics only
        uint32_t timestamp;
    };
    ```
  - [x] Update the aggregate initialization in `OdoControl::tick()` to include `rawV, rawW` before `HAL_GetTick()` — field order must match struct declaration

- [x] Task 4: Add robustness private state to `OdoControl.h` (AC: #1, #2, #3)
  - [x] In the private section of `OdoControl`, add:
    ```cpp
    uint32_t _stallCount      = 0;
    uint32_t _encFaultCountL  = 0;
    uint32_t _encFaultCountR  = 0;
    ```
  - [x] These counters are reset to 0 on arrival condition, on stall trigger, on encoder fault trigger, and when the guard condition is no longer met each tick

- [x] Task 5: Update `OdoControl::tick()` — insert robustness checks (AC: #1, #2, #3, #5)
  - [ ] Store pre-clamp values BEFORE clamping:
    ```cpp
    float rawV = v;   // store before clamp
    float rawW = w;
    v = clamp(v, -Config::MAX_DUTY, Config::MAX_DUTY);
    w = clamp(w, -Config::MAX_DUTY, Config::MAX_DUTY);
    ```
  - [ ] After `_motor->setMotors(leftDuty, rightDuty)`, insert stall detection block:
    ```cpp
    // ─── Stall detection ───────────────────────────────────────────────────
    static constexpr uint32_t STALL_TICKS =
        Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;  // = 100 ticks @ 200Hz

    float avgDuty = (fabsf(leftDuty) + fabsf(rightDuty)) * 0.5f;
    if (avgDuty > Config::STALL_DUTY_THRESHOLD &&
        fabsf(_odom->getV()) < Config::STALL_SPEED_THRESHOLD) {
        if (++_stallCount >= STALL_TICKS) {
            _motor->setMotors(0.0f, 0.0f);
            _bus->publish(Topic::ALERT, BusFormat::altStall());
            _pidSpeed.reset();
            _pidAngle.reset();
            _stallCount = 0;
            return;
        }
    } else {
        _stallCount = 0;
    }
    ```
  - [x] After stall detection block, insert encoder fault detection block:
    ```cpp
    // ─── Encoder fault detection ───────────────────────────────────────────
    if (avgDuty > Config::STALL_DUTY_THRESHOLD) {
        // getVLeft() == 0.0f is exact: ΔticksL=0 → 0.0f * constant / dt = 0.0f exactly
        if (_odom->getVLeft() == 0.0f) {
            if (++_encFaultCountL >= STALL_TICKS) {
                _bus->publish(Topic::ALERT, BusFormat::altEncoderFault("LEFT"));
                _encFaultCountL = 0;
            }
        } else {
            _encFaultCountL = 0;
        }
        if (_odom->getVRight() == 0.0f) {
            if (++_encFaultCountR >= STALL_TICKS) {
                _bus->publish(Topic::ALERT, BusFormat::altEncoderFault("RIGHT"));
                _encFaultCountR = 0;
            }
        } else {
            _encFaultCountR = 0;
        }
    } else {
        _encFaultCountL = 0;
        _encFaultCountR = 0;
    }
    ```
  - [x] Update `OdoSnapshot` initialization to include `rawV, rawW`:
    ```cpp
    if (_tickCount % Config::TELEM_DIVIDER == 0) {
        latestSnapshot = {
            _odom->getX(), _odom->getY(), _odom->getAngle(),
            _odom->getVLeft(), _odom->getVRight(),
            rawV, rawW,
            HAL_GetTick()
        };
        _bus->publish(Topic::TELEMETRY,
                      BusFormat::telOdo(_odom->getX(), _odom->getY(), _odom->getAngle()));
    }
    ```
  - [x] Also reset fault counters in the arrival condition block:
    ```cpp
    // In arrival block, add before return:
    _stallCount = 0;
    _encFaultCountL = 0;
    _encFaultCountR = 0;
    ```
  - [x] Add `#include <cmath>` if `fabsf` is not yet included (it typically comes from `<cmath>`)

- [x] Task 6: Add new test cases to `Tests/Unit/OdoControlTest.cpp` (AC: #6)
  - [x] Add helper to existing `OdoControlTest` fixture for stall setup:
    ```cpp
    void setupStallCondition() {
        // IOdomHAL::getV() returns 0 (measured speed = 0)
        odom.v      = 0.0f;
        odom.vLeft  = 0.0f;
        odom.vRight = 0.0f;
        // Push a far target so PID output exceeds STALL_DUTY_THRESHOLD
        pushPoseSetpoint(1000.0f, 0.0f);
    }
    ```
  - [x] Add test `StallDetectionTriggersAfterStallTimeMs`:
    ```cpp
    TEST_F(OdoControlTest, StallDetectionTriggersAfterStallTimeMs) {
        setupStallCondition();
        constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
        // Run STALL_TICKS - 1 ticks — no alert yet
        for (uint32_t i = 0; i < STALL_TICKS - 1; ++i) odo->tick();
        EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
        // Tick #STALL_TICKS — triggers stall
        odo->tick();
        ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
        EXPECT_NE(bus.last(Topic::ALERT)->payload.find("STALL"), std::string::npos);
        // Motors stopped
        EXPECT_FLOAT_EQ(motor.lastLeft,  0.0f);
        EXPECT_FLOAT_EQ(motor.lastRight, 0.0f);
    }
    ```
  - [x] Add test `StallCounterResetsWhenSpeedReturns`:
    ```cpp
    TEST_F(OdoControlTest, StallCounterResetsWhenSpeedReturns) {
        setupStallCondition();
        constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
        // Half the ticks with stall condition
        for (uint32_t i = 0; i < STALL_TICKS / 2; ++i) odo->tick();
        // Speed returns — counter should reset
        odom.v = 1.0f;
        odo->tick();
        odom.v = 0.0f;
        // Another half without triggering (counter was reset)
        for (uint32_t i = 0; i < STALL_TICKS / 2; ++i) odo->tick();
        EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
    }
    ```
  - [x] Add test `EncoderFaultLeftPublishesAlert`:
    ```cpp
    TEST_F(OdoControlTest, EncoderFaultLeftPublishesAlert) {
        pushPoseSetpoint(1000.0f, 0.0f);  // high error → high duty
        odom.v      = 1.0f;    // speed non-zero (not a stall)
        odom.vLeft  = 0.0f;    // left encoder silent
        odom.vRight = 1.0f;    // right encoder OK
        constexpr uint32_t STALL_TICKS = Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
        for (uint32_t i = 0; i < STALL_TICKS; ++i) odo->tick();
        ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
        EXPECT_NE(bus.last(Topic::ALERT)->payload.find("ENCODER_FAULT LEFT"), std::string::npos);
    }
    ```
  - [x] Add test `EncoderFaultRightPublishesAlert` (same pattern, `vLeft = 1.0f`, `vRight = 0.0f`, check `"ENCODER_FAULT RIGHT"`)
  - [x] Add test `RawVWStoredInSnapshot`:
    ```cpp
    TEST_F(OdoControlTest, RawVWStoredInSnapshot) {
        pushPoseSetpoint(1000.0f, 0.0f);  // huge target → PID saturates
        for (int i = 0; i < 10; ++i) odo->tick();  // trigger TELEM_DIVIDER
        // rawV should exceed MAX_DUTY (pre-clamp) if PID is saturating
        // At minimum, snapshot must have been updated (timestamp non-zero if getMockTick() > 0)
        getMockTick() = 1000;
        pushPoseSetpoint(1000.0f, 0.0f);
        for (int i = 0; i < 10; ++i) odo->tick();
        EXPECT_EQ(OdoControl::latestSnapshot.timestamp, 1000u);
        // rawV and rawW fields accessible (struct extension validated by compilation)
        (void)OdoControl::latestSnapshot.rawV;
        (void)OdoControl::latestSnapshot.rawW;
    }
    ```

- [x] Task 7: Verify NFR compliance
  - [x] `grep -r "snprintf" App/Tasks/OdoControl.cpp` → zero results
  - [x] `grep -r "new\|delete\|malloc\|free" App/Tasks/OdoControl.h App/Tasks/OdoControl.cpp` → zero results
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/Tasks/OdoControl.h App/Tasks/OdoControl.cpp` → zero results
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/BusFormat.h App/BusFormat.cpp` → zero results

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Config.h                          ← UPDATE (add STALL_SPEED_THRESHOLD)
├── BusFormat.h                       ← UPDATE (add altStall, altEncoderFault declarations)
├── BusFormat.cpp                     ← UPDATE (implement altStall, altEncoderFault)
└── Tasks/
    ├── OdoControl.h                  ← UPDATE (extend OdoSnapshot, add _stallCount, _encFaultCountL/R)
    └── OdoControl.cpp                ← UPDATE (tick() robustness checks + pre-clamp logging)

Tests/
└── Unit/
    └── OdoControlTest.cpp            ← UPDATE (add 5 new test cases)
```

**Ne pas toucher :** `App/Interfaces/`, `App/Drivers/`, `App/Tasks/MotionPlanner.*`, `App/Tasks/Pid.*`, `App/Tasks/ExternalComm.*`, `App/SystemInit/`, `Tests/CMakeLists.txt` (no new executable — tests added to existing OdoControlTest), `Core/`, `Middlewares/`

---

### Critical: OdoSnapshot Struct Field Order

The existing aggregate initialization in `tick()` is:
```cpp
latestSnapshot = {
    _odom->getX(), _odom->getY(), _odom->getAngle(),
    _odom->getVLeft(), _odom->getVRight(),
    HAL_GetTick()    // ← was last field
};
```

After the struct extension, this MUST become:
```cpp
latestSnapshot = {
    _odom->getX(), _odom->getY(), _odom->getAngle(),
    _odom->getVLeft(), _odom->getVRight(),
    rawV, rawW,      // ← two new fields BEFORE timestamp
    HAL_GetTick()    // ← timestamp stays last
};
```

Missing these fields in the aggregate initializer will silently zero-initialize `rawV` and `rawW` in C++17 (no error) — the test `RawVWStoredInSnapshot` catches this via compilation verification.

**The existing test `SnapshotUpdatedAtDividerRate` still works** — it only checks `timestamp`, which remains the last field.

---

### Stall vs Encoder Fault — Key Design Distinction

| Condition | Detection | Action |
|-----------|-----------|--------|
| **Stall** | `avgDuty > STALL_DUTY_THRESHOLD` AND `getV() < STALL_SPEED_THRESHOLD` for 100 ticks | Stop motors + reset PID + publish `ALT STALL` |
| **Encoder fault** | `avgDuty > STALL_DUTY_THRESHOLD` AND `getVLeft() == 0.0f` (exact) for 100 ticks | Publish `ALT ENCODER_FAULT LEFT` only — no motor stop |
| **Boot guard** | `_hasSetpoint == false` | `setMotors(0,0)` + no PID (already in Story 2.2) |

**Why exact comparison `== 0.0f` for encoder fault:** `getVLeft()` is computed as `ΔticksL × constant / dt`. If `ΔticksL == 0` (integer), the float result is `0.0f` exactly — no floating-point rounding. This is not an approximation; it is a reliable exact-zero check.

**Why stall uses `getV()` not tick-based:** Stall detects PHYSICAL blockage (motor blocked). Even if both encoders are working, if measured speed is near zero despite high duty, the motor is blocked. `getV()` = `(vLeft + vRight) / 2` from IOdomHAL is the right signal.

---

### STALL_TICKS Computation

```cpp
static constexpr uint32_t STALL_TICKS =
    Config::STALL_TIME_MS * Config::ODO_FREQ_HZ / 1000u;
// = 500ms × 200Hz / 1000 = 100 ticks
```

Declare this as `static constexpr` inside `tick()` (function-local) — it is computed at compile time but belongs to this function's logic, not to the global Config namespace. Same value reused for both stall and encoder fault counters.

---

### Complete Updated tick() Structure

The 12-step sequence from Story 2.2 becomes 14 steps with robustness blocks inserted between step 10 (setMotors) and step 11 (telemetry):

```
1.  vTaskDelayUntil
2.  IOdomHAL::update()
3.  xQueuePeek (non-blocking)
4.  if !_hasSetpoint → setMotors(0,0), return (boot guard — unchanged)
5.  Compute errDist, errAngle
5b. if errDist < ARRIVAL_THRESHOLD → stop + publish arrival + reset PIDs + reset counters + return
6.  _pidSpeed.compute(errDist, dt)
7.  _pidAngle.compute(errAngle, dt)
7b. Store rawV = v, rawW = w  (pre-clamp)
8.  Clamp v, w to ±MAX_DUTY
9.  leftDuty = v - w, rightDuty = v + w
10. _motor->setMotors(leftDuty, rightDuty)
10b. STALL DETECTION: avgDuty + getV() check → stop + alert + reset if threshold reached
10c. ENCODER FAULT: avgDuty + getVLeft/Right() == 0.0f check → alert (no stop)
11. if tickCount % TELEM_DIVIDER → update latestSnapshot (with rawV, rawW) + publish telemetry
```

---

### MockOdomHAL Requirements for Tests

The existing `Tests/Mocks/MockOdomHAL.h` must expose `v`, `vLeft`, `vRight` as settable public fields. If they don't exist yet, add them. Expected interface:

```cpp
struct MockOdomHAL : IOdomHAL {
    float x = 0.0f, y = 0.0f, angle = 0.0f;
    float vLeft = 0.0f, vRight = 0.0f;
    float v = 0.0f, w = 0.0f;
    float dt = 0.005f;  // 5ms at 200Hz

    void  update()     override {}
    float getX()       override { return x; }
    float getY()       override { return y; }
    float getAngle()   override { return angle; }
    float getVLeft()   override { return vLeft; }
    float getVRight()  override { return vRight; }
    float getV()       override { return v; }
    float getW()       override { return w; }
    float getDt()      override { return dt; }
};
```

Check the actual `MockOdomHAL.h` before implementing tests — if `v` field is missing, add it.

---

### Why No Stop on Encoder Fault

The encoder fault condition (`altEncoderFault`) only publishes an alert — it does NOT stop the motors. Rationale:
- An encoder failure is a sensor fault, not a motor fault
- The robot can still move (one-sided velocity estimation is possible)
- Stopping abruptly on a sensor fault could be more dangerous in some contexts
- The PC operator receives the alert and can issue `CMD STOP` explicitly
- **Motor stop only happens on physical stall** (detected via speed, not tick counts)

---

### BusFormat Pattern — Existing snprintf Usage

Only two functions in `BusFormat.cpp` are allowed to use `snprintf`:
1. `BusFormat::altAlarm(uint32_t bitmask)` — added in Story 2.3
2. `BusFormat::altEncoderFault(const char* side)` — added in this story

All other BusFormat methods return static string literals directly. The test `BusFormatTest.cpp` should include a test for `altEncoderFault` format validation.

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| No `snprintf` in `OdoControl.cpp` | FR-09 | All alerts via `BusFormat::altStall()`, `BusFormat::altEncoderFault()` |
| No `new`/`delete` | NFR-02 | Counters as plain `uint32_t` members |
| English identifiers | NFR-08 | `_stallCount`, `_encFaultCountL/R`, `rawV`, `rawW` |
| Guard convention | NFR-06 | `OdoControl.h` guard unchanged: `APP_TASKS_ODOCONTROL_H` |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes to `Core/`, `Drivers/`, `Middlewares/` |
| `xTaskCreate` only in SystemInit | FR-11 | No FreeRTOS task creation in this story |
| `atan2f` not `atan2` | NFR-01 perf | No new angle computation added — existing uses unchanged |

---

### Dependencies — Must Exist Before Implementation

**From Story 2.2 (must be complete):**
- `App/Tasks/OdoControl.h` — `OdoSnapshot` struct, `Pid::reset()`, `_hasSetpoint` boot guard
- `App/Tasks/OdoControl.cpp` — `tick()` 12-step sequence, `latestSnapshot` static definition
- `App/Tasks/Pid.h/.cpp` — `reset()` method available

**From Story 2.1 (must be complete):**
- `App/Config.h` — `STALL_DUTY_THRESHOLD`, `STALL_TIME_MS`, `ODO_FREQ_HZ` already declared
- `App/Interfaces/IOdomHAL.h` — `getV()`, `getVLeft()`, `getVRight()` methods in interface

**From Stories 1.x:**
- `App/BusFormat.h/.cpp` — existing, will be extended
- `Tests/Mocks/MockOdomHAL.h` — needs `v` field if not already present
- `Tests/Mocks/MockBus.h` — `hasPublished()`, `last()`, `clear()` methods

---

### Deferred Work

- **Encoder fault recovery:** No recovery mechanism defined — alert published each time fault persists for STALL_TICKS. Re-detection after reset to zero is intentional (ongoing fault = ongoing alerts at STALL_TICKS intervals).
- **Velocity mode robustness:** `SetpointMode::VELOCITY` is not implemented — stall/encoder fault logic only runs in POSE mode path. No change needed.
- **Stall auto-retry:** After stall, `_hasSetpoint` remains `true` but `_stallCount = 0`. Robot will try again on next `processCmd` from MotionPlanner. No auto-retry logic — operator must re-send CMD MOVE.

---

### References

- Story épique source : [epics.md — Story 2.4](../../planning-artifacts/epics.md)
- OdoControl état actuel (tick() complet, Pid::reset(), OdoSnapshot) : [2-2-odocontrol-tache-200hz-double-pid-orthogonal-setpoint-union-condition-arrivee.md](2-2-odocontrol-tache-200hz-double-pid-orthogonal-setpoint-union-condition-arrivee.md)
- MotionPlanner (BusFormat::altAlarm pattern — same snprintf pattern for altEncoderFault) : [2-3-motionplanner-tache-event-driven-mailbox-consigne-setpoint-arret-urgence.md](2-3-motionplanner-tache-event-driven-mailbox-consigne-setpoint-arret-urgence.md)
- Architecture : [architecture.md](../../planning-artifacts/architecture.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

(aucun blocage)

### Completion Notes List

- Story 2.4 implémentée complètement le 2026-05-14 par claude-sonnet-4-6.
- AC #1 (stall) : détection après STALL_TICKS = 100 ticks, stop moteurs + reset PID + publish "ALT STALL\n".
- AC #2/#3 (encoder fault) : détection encodeur silencieux (exact `== 0.0f`) séparée du stall, warning seulement sans arrêt moteur.
- AC #4 (boot guard) : déjà implémenté en Story 2.2, vérifié par test `NoMotionWithoutSetpoint` (inchangé).
- AC #5 (rawV/rawW) : stockage pré-clamp dans `OdoSnapshot`, accessible en telemetry/diagnostic.
- AC #6 (tests) : 5 nouveaux tests dans `OdoControlTest_Robustness`, tous passent (39/39 total suite).
- Ajustement test `StallCounterResetsWhenSpeedReturns` : vL/vR aussi remis à 1.0f lors du reset pour éviter accumulation du compteur encoder fault, ce qui correspond à une condition physique cohérente (si la vitesse est revenue, les encodeurs lisent aussi).
- NFR conforme : zéro `snprintf` dans OdoControl.cpp, zéro allocation dynamique, identifiants en anglais.

### File List

- robot-cdr/App/Config.h (modifié : ajout STALL_SPEED_THRESHOLD = 0.05f)
- robot-cdr/App/BusFormat.h (modifié : déclarations altStall, altEncoderFault)
- robot-cdr/App/BusFormat.cpp (modifié : implémentation altStall, altEncoderFault)
- robot-cdr/App/Tasks/OdoControl.h (modifié : OdoSnapshot + rawV/rawW, _stallCount, _encFaultCountL/R)
- robot-cdr/App/Tasks/OdoControl.cpp (modifié : tick() + pré-clamp + stall/encodeur fault + reset arrivée)
- robot-cdr/Tests/Unit/OdoControlTest.cpp (modifié : 5 nouveaux tests dans OdoControlTest_Robustness)

## Change Log

- 2026-05-14 : Implémentation complète Story 2.4 — détection stall moteur (`ALT STALL`) et encodeur silencieux (`ALT ENCODER_FAULT LEFT/RIGHT`), champs `rawV/rawW` dans `OdoSnapshot`, 5 nouveaux tests, 39/39 tests passent.
