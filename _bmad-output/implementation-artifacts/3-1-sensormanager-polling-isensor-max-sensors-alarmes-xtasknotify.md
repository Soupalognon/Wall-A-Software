# Story 3.1: SensorManager — polling ISensor[MAX_SENSORS], alarmes xTaskNotify

Status: review

## Story

As a developer,
I want `SensorManager` to orchestrate up to 15 `ISensor*` sensors injected by `SystemInit` and send critical alarms to `MotionPlanner` via `xTaskNotify`,
so that adding a new sensor only requires a new concrete class and alarms reach `MotionPlanner` within ≤1 FreeRTOS tick.

## Acceptance Criteria

1. **Given** `SensorManager` receives an array `ISensor*[MAX_SENSORS]` injected by `SystemInit`
   **When** the task polls all sensors via `ISensor::read()`
   **Then** `SensorManager` contains no `static_cast` or `dynamic_cast` — it knows no concrete types

2. **Given** a sensor returns an alarming value and `ISensor::isAlarm()` returns `true`
   **When** `SensorManager` detects the alarm
   **Then** `xTaskNotify(MotionPlanner::handle, alarmBit, eSetBits)` is called — latency ≤1 FreeRTOS tick

3. **Given** `Tests/Unit/SensorManagerTest.cpp` exists
   **When** 15 mock `ISensor` instances are constructed and injected
   **Then** polling and alarm triggering are validated — tests pass on PC host without STM32 hardware

4. **And** `SensorManager` publishes sensor health via `bus_->publish(Topic::HEALTH, BusFormat::hltSensors(count, alarmMask))`

5. **And** `SensorManager::SensorSnapshot latestSnapshot` (static public struct with `values[MAX_SENSORS]`, `alarms[MAX_SENSORS]`, `count`, `timestamp`) is updated each polling cycle — readable by `Monitoring` without explicit synchronization (ARM Cortex-M atomic struct write, writer is higher priority)

## Tasks / Subtasks

- [x] Task 1: Extend `App/Interfaces/ISensor.h` — add `isAlarm()` method (AC: #2)
  - [x] Add pure virtual: `virtual bool isAlarm() = 0;`
  - [x] Place after `virtual float read() = 0;` — keep interface minimal (4 methods total)
  - [x] Update guard: verify `#ifndef APP_INTERFACES_ISENSOR_H` already present

- [x] Task 2: Add constants to `App/Config.h` (AC: #2, #4)
  - [x] Verify `MAX_SENSORS = 15` already exists — do NOT duplicate
  - [x] Verify `PRIO_SENSOR_MANAGER = 2` and `STACK_SENSOR_MANAGER = 256` already exist
  - [x] Add `static constexpr uint32_t SENSOR_POLL_MS = 50;` — 20Hz polling rate (non real-time, lower than OdoControl)

- [x] Task 3: Add `hltSensors` to `App/BusFormat.h` and `App/BusFormat.cpp` (AC: #4)
  - [x] Add declaration to `BusFormat.h`: `static const char* hltSensors(uint8_t count, uint32_t alarmMask);`
  - [x] Add implementation to `BusFormat.cpp`
  - [x] Verify `snprintf` is only in `BusFormat.cpp` — never inline in SensorManager

- [x] Task 4: Expose `MotionPlanner::handle` — deferred from Story 2.2 (AC: #2)
  - [x] In `App/Tasks/MotionPlanner.h`, add public static member
  - [x] In `App/Tasks/MotionPlanner.cpp`, add definition
  - [x] In `App/SystemInit/SystemInit.cpp boot()`, after the existing `xTaskCreate` for MotionPlanner, add `MotionPlanner::handle = motionPlannerHandle;`
  - [x] This is the minimal change to expose the handle

- [x] Task 5: Create `App/Tasks/SensorManager.h` (AC: #1, #2, #5)
  - [x] Guard: `#ifndef APP_TASKS_SENSORMANAGER_H`
  - [x] Includes: `"Interfaces/ISensor.h"`, `"Interfaces/IBus.h"`, `"Config.h"`, `<FreeRTOS.h>`, `<task.h>`, `<cstdint>`
  - [x] Declare nested public struct `SensorSnapshot` with `values`, `alarms`, `count`, `timestamp`
  - [x] Constructor: `SensorManager(ISensor** sensors, uint8_t sensorCount, TaskHandle_t motionPlannerHandle, IBus* bus);`
  - [x] Public: `static void task(void* param);` and `void pollOnce();`
  - [x] Private members declared
  - [x] Note: NO `static_cast`/`dynamic_cast` on ISensor pointers

- [x] Task 6: Create `App/Tasks/SensorManager.cpp` (AC: #1, #2, #4, #5)
  - [x] Define static: `SensorManager::SensorSnapshot SensorManager::latestSnapshot{};`
  - [x] Include `"stm32f4xx_hal.h"` for `HAL_GetTick()` (consistent with OdoControl pattern)
  - [x] Constructor: store all pointers, nothing else
  - [x] `SensorManager::task()`: full FreeRTOS loop with `vTaskDelay`
  - [x] `SensorManager::pollOnce()`: full algorithm per Dev Notes
  - [x] Never `snprintf` inline

- [x] Task 7: Update `App/SystemInit/SystemInit.cpp` — wire SensorManager (AC: #2, #5)
  - [x] `#include "Tasks/SensorManager.h"` already present
  - [x] Removed duplicate `SensorManager::latestSnapshot` definition (moved to SensorManager.cpp)
  - [x] Added `MotionPlanner::handle = motionPlannerHandle;`
  - [x] Added stub sensor array, `SensorManager` instantiation and `xTaskCreate`

- [x] Task 8: Create `Tests/Mocks/MockSensor.h` — implements `ISensor` (AC: #3)
  - [x] Class `MockSensor : public ISensor` with all 4 methods + setters
  - [x] Guard: `#ifndef TESTS_MOCKS_MOCKSENSOR_H`

- [x] Task 9: Update `Tests/CMakeLists.txt` — add SensorManagerTest target (AC: #3)
  - [x] Target added with correct sources and includes

- [x] Task 10: Create `Tests/Unit/SensorManagerTest.cpp` (AC: #3)
  - [x] 10 tests — all pass

- [x] Task 11: Verify NFR compliance
  - [x] No `static_cast`/`dynamic_cast` on ISensor pointers in SensorManager
  - [x] No `snprintf` inline in SensorManager.cpp
  - [x] No `new`/`delete`/`malloc`/`free` in SensorManager.cpp
  - [x] No accented characters in SensorManager files
  - [x] ISensor.h contains no references to concrete task classes

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Config.h                          ← UPDATE (add SENSOR_POLL_MS)
├── BusFormat.h                       ← UPDATE (add hltSensors declaration)
├── BusFormat.cpp                     ← UPDATE (add hltSensors implementation)
├── Interfaces/
│   └── ISensor.h                     ← UPDATE (add isAlarm() pure virtual)
├── Tasks/
│   ├── MotionPlanner.h               ← UPDATE (add public static handle)
│   ├── MotionPlanner.cpp             ← UPDATE (define static handle)
│   ├── SensorManager.h               ← NEW
│   └── SensorManager.cpp             ← NEW
└── SystemInit/
    └── SystemInit.cpp                ← UPDATE (expose handle, wire SensorManager)

Tests/
├── CMakeLists.txt                    ← UPDATE (add SensorManagerTest)
├── Mocks/
│   └── MockSensor.h                  ← NEW
└── Unit/
    └── SensorManagerTest.cpp         ← NEW
```

**Do NOT touch:** `App/Tasks/OdoControl.h/.cpp`, `App/Tasks/ExternalComm.h/.cpp`, `App/Tasks/Monitoring.h`, `Core/`, `Drivers/`, `Middlewares/`

---

### CRITICAL: Deferred Work from Story 2.2 — MotionPlanner::handle

Story 2.2 stored `motionPlannerHandle` as a static local in `boot()` and explicitly deferred its exposition to Story 3.1. The two-step exposure is:

**Step A — MotionPlanner.h** (add once, never modify again):
```cpp
class MotionPlanner {
public:
    static TaskHandle_t handle;  // set by SystemInit after xTaskCreate; nullptr until then
    // ... existing members ...
};
```

**Step B — MotionPlanner.cpp** (add definition):
```cpp
TaskHandle_t MotionPlanner::handle = nullptr;
```

**Step C — SystemInit.cpp** (in `boot()`, after existing `xTaskCreate` call for MotionPlanner):
```cpp
// Story 2.2 already has:
static TaskHandle_t motionPlannerHandle = nullptr;
xTaskCreate(MotionPlanner::task, "MotionPlan",
            Config::STACK_MOTION_PLANNER, &motionPlanner,
            Config::PRIO_MOTION_PLANNER, &motionPlannerHandle);

// Add immediately after (this is the Story 3.1 addition):
MotionPlanner::handle = motionPlannerHandle;
```

This satisfies NFR-07 (only App/ and Tests/ modified) and keeps `SystemInit` as the sole wiring point (FR-11).

---

### SensorManager::pollOnce() — Full Algorithm

```cpp
void SensorManager::pollOnce() {
    uint32_t alarmMask = 0;

    for (uint8_t i = 0; i < _sensorCount && i < Config::MAX_SENSORS; ++i) {
        if (_sensors[i] == nullptr) continue;

        float value = _sensors[i]->read();
        bool  alarm = _sensors[i]->isAlarm();

        latestSnapshot.values[i] = value;
        latestSnapshot.alarms[i] = alarm;

        if (alarm) {
            alarmMask |= (1u << i);
        }
    }

    latestSnapshot.count     = _sensorCount;
    latestSnapshot.timestamp = HAL_GetTick();

    if (alarmMask != 0) {
        xTaskNotify(_motionPlannerHandle, alarmMask, eSetBits);
    }

    // Always publish health — Monitoring uses latestSnapshot, not IBus subscribe
    _bus->publish(Topic::HEALTH, BusFormat::hltSensors(_sensorCount, alarmMask));
}
```

**Why always publish health even with no alarms?** `Monitoring` detects stale data by checking `latestSnapshot.timestamp`. The health publish is a secondary signal for the PC; the primary freshness mechanism is the snapshot.

**Why `i < Config::MAX_SENSORS` guard?** Defensive: `_sensorCount` is injected by `SystemInit`. If ever misconfigured, the loop never overflows the snapshot arrays.

**Why `xTaskNotify` without critical section?** `SensorManager` priority = 2, `MotionPlanner` priority = 4. `xTaskNotify` is ISR-safe and can be called from any task context.

---

### SensorManager::task() — Exact Pattern

```cpp
void SensorManager::task(void* param) {
    auto* self = static_cast<SensorManager*>(param);
    for (;;) {
        self->pollOnce();
        vTaskDelay(pdMS_TO_TICKS(Config::SENSOR_POLL_MS));
    }
}
```

`vTaskDelay` (not `vTaskDelayUntil`) is correct here — SensorManager is not hard real-time. A 50ms poll jitter of a few ticks is acceptable.

---

### ISensor Interface After This Story

```cpp
// App/Interfaces/ISensor.h
#ifndef APP_INTERFACES_ISENSOR_H
#define APP_INTERFACES_ISENSOR_H
#include <cstdint>

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual uint8_t     id()      = 0;
    virtual const char* name()    = 0;
    virtual float       read()    = 0;
    virtual bool        isAlarm() = 0;  // ← added by Story 3.1
};
#endif
```

**Impact on existing code**: Story 1.2 defined ISensor with 3 methods. `isAlarm()` is added here. No existing production code calls `isAlarm()` yet — only `SensorManager` will. The stub/mock implementations in Story 1.6 (`MockSensorHAL.h`) implement `ISensorHAL` (hardware abstraction), NOT `ISensor` — no conflict. `MockSensor.h` (created by this story) implements `ISensor` fresh.

---

### MockSensor.h — Exact Implementation

```cpp
#ifndef TESTS_MOCKS_MOCKSENSOR_H
#define TESTS_MOCKS_MOCKSENSOR_H

#include "Interfaces/ISensor.h"

class MockSensor : public ISensor {
public:
    MockSensor(uint8_t id, const char* name, float value = 0.0f, bool alarm = false)
        : _id(id), _name(name), _value(value), _alarm(alarm) {}

    uint8_t     id()      override { return _id; }
    const char* name()    override { return _name; }
    float       read()    override { return _value; }
    bool        isAlarm() override { return _alarm; }

    void setValue(float v) { _value = v; }
    void setAlarm(bool a)  { _alarm = a; }

private:
    uint8_t     _id;
    const char* _name;
    float       _value;
    bool        _alarm;
};
#endif
```

---

### SensorManagerTest.cpp — Exact Test Fixture and Tests

```cpp
#include "Stubs/MockHAL.h"  // must be first — provides HAL_GetTick() before SensorManager.h
#include <gtest/gtest.h>
#include "Mocks/MockBus.h"
#include "Mocks/MockSensor.h"
#include "Stubs/FreeRTOS.h"
#include "Stubs/task.h"
#include "Tasks/SensorManager.h"

class SensorManagerTest : public ::testing::Test {
protected:
    MockBus bus;

    void SetUp() override {
        resetTestNotifications();
        bus.clear();
        setMockTick(1000);  // non-zero for timestamp tests
        SensorManager::latestSnapshot = {};
    }
};

TEST_F(SensorManagerTest, EmptySensorArrayNoCrash) {
    ISensor* sensors[Config::MAX_SENSORS] = {};
    SensorManager sm{sensors, 0, nullptr, &bus};
    sm.pollOnce();  // must not crash
    EXPECT_EQ(SensorManager::latestSnapshot.count, 0u);
}

TEST_F(SensorManagerTest, SingleSensorNoAlarmNoNotify) {
    MockSensor s0{0, "temp", 25.0f, false};
    ISensor* sensors[Config::MAX_SENSORS] = {&s0};
    SensorManager sm{sensors, 1, nullptr, &bus};
    sm.pollOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));
    // no xTaskNotify — verified by checking mock notification state
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
    MockSensor s0{0, "proximity", 0.05f, true};  // alarm active
    ISensor* sensors[Config::MAX_SENSORS] = {&s0};
    SensorManager sm{sensors, 1, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &bits, 0);
    EXPECT_EQ(bits, 0x01u);  // bit 0
}

TEST_F(SensorManagerTest, SensorAtIndex3NotifiesBit3) {
    ISensor* sensors[Config::MAX_SENSORS] = {};
    MockSensor s3{3, "current", 10.0f, true};
    sensors[3] = &s3;
    SensorManager sm{sensors, 4, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &bits, 0);
    EXPECT_EQ(bits, 0x08u);  // bit 3
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
        {5,"s5"}, {6,"s6"}, {7,"s7",0.f,true},  // alarm
        {8,"s8"}, {9,"s9"}, {10,"s10"},{11,"s11"},{12,"s12"},{13,"s13"},{14,"s14"}
    };
    ISensor* sensors[Config::MAX_SENSORS];
    for (int i = 0; i < 15; ++i) sensors[i] = &sensors_storage[i];
    SensorManager sm{sensors, 15, nullptr, &bus};
    sm.pollOnce();
    uint32_t bits = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &bits, 0);
    EXPECT_EQ(bits, 0x80u);  // bit 7
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
    EXPECT_EQ(SensorManager::latestSnapshot.timestamp, 1000u);  // matches setMockTick(1000)
}
```

---

### SystemInit — SensorManager Wiring (Expanded View)

After Story 3.1, `boot()` gains these additions (in order, after MotionPlanner task creation):

```cpp
// --- Expose MotionPlanner handle (Story 3.1 addition) ---
MotionPlanner::handle = motionPlannerHandle;

// --- SensorManager (Story 3.1) ---
static ISensor* sensors[Config::MAX_SENSORS] = {};  // populated by Story 3.4
static uint8_t  sensorCount = 0;                     // updated by Story 3.4
static SensorManager sensorManager{sensors, sensorCount, MotionPlanner::handle, &extComm};
SensorManager::latestSnapshot = {};  // explicit zero-init before scheduler
xTaskCreate(SensorManager::task, "SensorMgr",
            Config::STACK_SENSOR_MANAGER, &sensorManager,
            Config::PRIO_SENSOR_MANAGER, nullptr);
```

**Story 3.4 will only modify `sensors[]` and `sensorCount`** — `SensorManager` itself is untouched (NFR-05).

---

### Dependencies — What Must Exist Before Implementing

From **Stories 1.2–1.6** (must be complete):
- `App/Interfaces/ISensor.h` — to be extended with `isAlarm()`
- `App/Interfaces/IBus.h` — `publish(Topic, const char*)`, `Topic` enum
- `App/BusFormat.h/.cpp` — `hltTemp`, `altProximity`, `logInfo`, `altAlarm` exist; add `hltSensors`
- `App/Config.h` — `MAX_SENSORS = 15`, `PRIO_SENSOR_MANAGER = 2`, `STACK_SENSOR_MANAGER = 256` already defined
- `Tests/Mocks/MockBus.h` — `published` vector, `hasPublished()`, `clear()`
- `Tests/Stubs/FreeRTOS.h`, `task.h`, `FreeRTOSStub.cpp` — `xTaskNotify`, `xTaskNotifyWait`, `resetTestNotifications()`
- `Tests/Stubs/MockHAL.h` — `HAL_GetTick()`, `setMockTick()`

From **Story 2.2** (must be complete):
- `App/Tasks/MotionPlanner.h/.cpp` — `motionPlannerHandle` stored in `boot()` as static local; this story adds `public static TaskHandle_t handle` to expose it
- `App/SystemInit/SystemInit.cpp` — two-queue topology in place; this story adds lines after the MotionPlanner `xTaskCreate`

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| No static/dynamic cast in SensorManager | FR-05 | Only `ISensor*` interface calls |
| No `snprintf` inline | FR-09 | Use `BusFormat::hltSensors()` exclusively |
| No `new`/`delete` in `App/` | NFR-02 | SensorManager instance is `static` in SystemInit |
| English identifiers | NFR-08 | All code in English |
| Guard convention | NFR-06 | `APP_TASKS_SENSORMANAGER_H` |
| `xTaskCreate` only in SystemInit | FR-11 | SensorManager task created in SystemInit.cpp only |
| Stacks/priorities in Config.h | FR-10 | `STACK_SENSOR_MANAGER`, `PRIO_SENSOR_MANAGER` already there; add `SENSOR_POLL_MS` |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes to `Core/`, `Drivers/`, `Middlewares/` |
| `ISensor` is the only contract | NFR-05 | Adding sensor in Story 3.4 = new class only, SensorManager unchanged |
| IBus is publish-only | NFR-04 | SensorManager publishes on HEALTH; Monitoring reads snapshot struct |

---

### What Already Exists — Do Not Recreate

From Stories 1.1–2.2:
- `App/Config.h` — `MAX_SENSORS = 15`, `PRIO_SENSOR_MANAGER = 2`, `STACK_SENSOR_MANAGER = 256`
- `App/Interfaces/IBus.h` — `publish(Topic, const char*)`, `Topic` enum with TELEMETRY, ALERT, LOG, HEALTH
- `App/BusFormat.h/.cpp` — `telOdo`, `altProximity`, `logInfo`, `hltTemp`, `altAlarm` (do NOT recreate these)
- `Tests/Stubs/task.h` + `FreeRTOSStub.cpp` — `xTaskNotify`, `xTaskNotifyWait`, `resetTestNotifications()` (added in Story 2.2)
- `Tests/Mocks/MockBus.h` — `published` vector, `hasPublished()`, `count()`, `clear()` (Story 1.6)
- `Tests/Mocks/MockMotorHAL.h`, `MockSensorHAL.h` — these mock `IMotorHAL`/`ISensorHAL` (hardware layer), NOT `ISensor` (domain layer) — do not confuse them; `MockSensor.h` is a new file for the domain interface

### Deferred Work

Story 3.4 will:
- Create concrete `ProximitySensor`, `TemperatureSensor`, `CurrentSensor` implementing `ISensor`
- Populate `sensors[]` and `sensorCount` in `SystemInit.cpp` — only file modified
- Each concrete driver implements `isAlarm()` with its own threshold logic

Story 3.3 (Monitoring) will:
- Read `SensorManager::latestSnapshot` with `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` (Monitoring is lower priority — reader must protect multi-field struct copy)
- Compare `latestSnapshot.timestamp` against `Config::MONITORING_STALE_MS`

---

### References

- Story requirements: [epics.md — Story 3.1](_bmad-output/planning-artifacts/epics.md)
- FR-05 (SensorManager ISensor[MAX_SENSORS]): [epics.md — Requirements Inventory](_bmad-output/planning-artifacts/epics.md)
- FR-07 (alarmes xTaskNotify bitmask): [epics.md — Requirements Inventory](_bmad-output/planning-artifacts/epics.md)
- Architecture data flow: [architecture.md — Data Flow](_bmad-output/planning-artifacts/architecture.md)
- Architecture boundaries: [architecture.md — Architectural Boundaries](_bmad-output/planning-artifacts/architecture.md)
- Monitoring pull model (SensorSnapshot): [architecture.md — Monitoring Pull Model](_bmad-output/planning-artifacts/architecture.md)
- MotionPlanner handle deferred: [2-2-motionplanner-tache-event-driven-mailbox-consigne-arret-urgence.md — Deferred Work](_bmad-output/implementation-artifacts/2-2-motionplanner-tache-event-driven-mailbox-consigne-arret-urgence.md)
- FreeRTOS notification stubs: [2-2-motionplanner-tache-event-driven-mailbox-consigne-arret-urgence.md — FreeRTOS Stub](_bmad-output/implementation-artifacts/2-2-motionplanner-tache-event-driven-mailbox-consigne-arret-urgence.md)
- OdoControl snapshot pattern (mirror for SensorSnapshot): [2-1-odocontrol-tache-200hz-pid-encodeurs.md](_bmad-output/implementation-artifacts/2-1-odocontrol-tache-200hz-pid-encodeurs.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

- Problème linkage `getMockTick` : `main.h` wrappe ses includes dans `extern "C"`, ce qui causait un mismatch de name mangling pour la fonction inline. Résolu en remplaçant `#include "main.h"` par `#include "stm32f4xx_hal.h"` dans `SensorManager.cpp` (même convention que `OdoControl.cpp`).
- `getMockTick()` convertie de inline-with-static-local en non-inline définie dans `FreeRTOSStub.cpp` pour garantir une seule instance entre TUs (variable `tick` partagée).

### Completion Notes List

- SensorManager implémenté : polling 20Hz via `pollOnce()`, alarmes bitmask via `xTaskNotify`, snapshot publique `latestSnapshot`, publication health via `BusFormat::hltSensors()`.
- 10/10 tests unitaires passent sur PC host. 0 régression sur les 5 suites existantes (50 tests au total).
- `getMockTick()` déplacée en non-inline dans `FreeRTOSStub.cpp` ; `resetTestNotifications()` et `setMockTick()` ajoutées aux stubs.
- `MotionPlanner::handle` exposé comme prévu par Story 2.2.

### File List

- Wall-A-STM/App/Interfaces/ISensor.h (modifié — ajout `isAlarm()`)
- Wall-A-STM/App/Config.h (modifié — ajout `SENSOR_POLL_MS`)
- Wall-A-STM/App/Services/BusFormat.h (modifié — ajout `hltSensors`)
- Wall-A-STM/App/Services/BusFormat.cpp (modifié — implémentation `hltSensors`)
- Wall-A-STM/App/Tasks/MotionPlanner.h (modifié — ajout `static TaskHandle_t handle`)
- Wall-A-STM/App/Tasks/MotionPlanner.cpp (modifié — définition `MotionPlanner::handle`)
- Wall-A-STM/App/Tasks/SensorManager.h (remplacé stub par implémentation complète)
- Wall-A-STM/App/Tasks/SensorManager.cpp (nouveau)
- Wall-A-STM/App/SystemInit/SystemInit.cpp (modifié — câblage SensorManager + handle)
- Wall-A-STM/Tests/Stubs/task.h (modifié — déclaration `resetTestNotifications()`)
- Wall-A-STM/Tests/Stubs/HalStub.h (modifié — `getMockTick()` non-inline + `setMockTick()`)
- Wall-A-STM/Tests/Stubs/FreeRTOSStub.cpp (modifié — définition `getMockTick()`, `xTaskNotify` avec accumulation, `resetTestNotifications()`)
- Wall-A-STM/Tests/Mocks/MockSensor.h (nouveau)
- Wall-A-STM/Tests/CMakeLists.txt (modifié — ajout SensorManagerTest)
- Wall-A-STM/Tests/Unit/SensorManagerTest.cpp (nouveau)

## Change Log

- 2026-05-14 : Story 3.1 implémentée — SensorManager polling ISensor[MAX_SENSORS], alarmes xTaskNotify bitmask, SensorSnapshot, BusFormat::hltSensors. 10/10 tests, 0 régression.
