# Story 3.3: Monitoring — supervision par pull sur shared memory horodatée

Status: review

## Story

As a developer,
I want `Monitoring` to periodically poll the static snapshots of `OdoControl`, `SensorManager`, and `ExternalComm` to detect stale data and publish alerts on IBus,
so that the system has centralized supervision without IBus subscribe and without any module knowing `Monitoring`.

## Acceptance Criteria

1. **Given** `Monitoring` reads `OdoControl::latestSnapshot.timestamp` at each polling cycle
   **When** `HAL_GetTick() - latestSnapshot.timestamp > Config::MONITORING_STALE_MS`
   **Then** `bus_->publish(Topic::ALERT, BusFormat::altStale("ODO"))` is called

2. **Given** `Monitoring` reads `SensorManager::latestSnapshot` and `ExternalComm::latestSnapshot`
   **When** the timestamp of one of these structs exceeds `Config::MONITORING_STALE_MS`
   **Then** a corresponding alert is published on `IBus(ALERT)` with the module name ("SENSOR" or "COMM")

3. **Given** `MonitoringTest.cpp` injects snapshots with artificial timestamps
   **When** `HAL_GetTick()` is mocked and the simulated delta exceeds the threshold
   **Then** `MockBus` captures the expected alert — test valid without STM32 hardware

4. **Given** `Monitoring` copies a snapshot struct to inspect it (multi-field read susceptible to preemption)
   **When** the copy is performed
   **Then** it is bracketed by `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` — verified in code review and in `MonitoringTest.cpp`

5. **And** `Monitoring` does not subscribe to IBus — IBus is publish-only; Monitoring reads only the static structs of known modules (`OdoControl`, `SensorManager`, `ExternalComm`)

## Tasks / Subtasks

- [x] Task 1: Add `altStale` to `App/Services/BusFormat.h` and `BusFormat.cpp` (AC: #1, #2)
  - [x] Add declaration: `static const char* altStale(const char* module);`
  - [x] Add implementation: `snprintf(buf, sizeof(buf), "ALT STALE %s\n", module);`
  - [x] Follow `altEncoderFault` pattern (same buf size, same snprintf style)

- [x] Task 2: Add `MONITORING_POLL_MS` to `App/Config.h` (AC: #1, #2)
  - [x] Add `static constexpr uint32_t MONITORING_POLL_MS = 100;` near `MONITORING_STALE_MS`
  - [x] 100ms polling ensures stale detection well before `MONITORING_STALE_MS = 500ms`

- [x] Task 3: Create `App/Tasks/Monitoring.h` (AC: #1, #2, #4, #5)
  - [x] Guard: `#ifndef APP_TASKS_MONITORING_H`
  - [x] Includes: `"Tasks/OdoControl.h"`, `"Tasks/SensorManager.h"`, `"Tasks/ExternalComm.h"`, `"Interfaces/IBus.h"`, `"Services/BusFormat.h"`, `"Config.h"`, `<FreeRTOS.h>`, `<task.h>`, `"stm32f4xx_hal.h"`
  - [x] Constructor: `Monitoring(IBus* bus);`
  - [x] Public: `static void task(void* param);`
  - [x] Public: `void checkOnce();` — testable entry point, called by `task` each cycle
  - [x] Private: `IBus* _bus`
  - [x] No snapshot struct — Monitoring reads others' snapshots, exposes none

- [x] Task 4: Create `App/Tasks/Monitoring.cpp` (AC: #1, #2, #4, #5)
  - [x] `task()`: `vTaskDelay(pdMS_TO_TICKS(Config::MONITORING_POLL_MS))` + `self->checkOnce()` in `for(;;)`
  - [x] `checkOnce()`: copy each of the 3 snapshots with critical section, then check staleness — see Dev Notes for exact implementation
  - [x] Never `snprintf` inline — use `BusFormat::altStale()` exclusively
  - [x] Never `new`/`delete`/`malloc`/`free`

- [x] Task 5: Update `App/SystemInit/SystemInit.cpp` — add Monitoring instance + task (AC: #1)
  - [x] Add: `#include "Tasks/Monitoring.h"`
  - [x] Add static instance: `static Monitoring monitoring{&extComm};`
  - [x] In `boot()`: add `OdoControl::latestSnapshot = {};` initialization
  - [x] In `boot()`: `xTaskCreate(Monitoring::task, "Monitor", Config::STACK_MONITORING, &monitoring, Config::PRIO_MONITORING, nullptr);`
  - [x] Declaration order: `monitoring` must be declared AFTER `extComm` (IBus* must be valid)

- [x] Task 6: Update `Tests/Stubs/StaticDefs.cpp` — add OdoControl and SensorManager snapshot definitions (AC: #3)
  - [x] Add: `#include "Tasks/OdoControl.h"` + `OdoControl::OdoSnapshot OdoControl::latestSnapshot{};`
  - [x] Add: `#include "Tasks/SensorManager.h"` + `SensorManager::SensorSnapshot SensorManager::latestSnapshot{};`
  - [x] These are needed because MonitoringTest does NOT link OdoControl.cpp or SensorManager.cpp
  - [x] Check there are no duplicate symbol errors: OdoControlTest links OdoControl.cpp (not StaticDefs.cpp); SensorManagerTest links SensorManager.cpp (not StaticDefs.cpp) — no conflict

- [x] Task 7: Create `Tests/Unit/MonitoringTest.cpp` (AC: #3, #4)
  - [x] Include order: `HalStub.h` FIRST, then gtest, FreeRTOS.h, MockBus.h, snapshots headers, Monitoring.h
  - [x] `SetUp()`: `bus.clear(); setMockTick(0); OdoControl::latestSnapshot = {}; SensorManager::latestSnapshot = {}; ExternalComm::latestSnapshot = {};`
  - [x] See Dev Notes for exact test list (minimum 8 tests)

- [x] Task 8: Update `Tests/CMakeLists.txt` — add MonitoringTest target (AC: #3)
  - [x] Sources: `Unit/MonitoringTest.cpp`, `${APP_DIR}/Tasks/Monitoring.cpp`, `${APP_DIR}/Services/BusFormat.cpp`, `Stubs/FreeRTOSStub.cpp`, `Stubs/StaticDefs.cpp`
  - [x] Includes: `${COMMON_INCLUDES}`
  - [x] Link: `GTest::gtest_main`
  - [x] Add `gtest_discover_tests(MonitoringTest)` at the bottom

- [x] Task 9: Update `Tests/run_tests.sh` — add MonitoringTest to TESTS array (AC: #3)
  - [x] Add `MonitoringTest` to the TESTS list after `ActuatorManagerTest`

- [x] Task 10: Verify NFR compliance
  - [x] No `snprintf` inline in Monitoring.cpp
  - [x] No `new`/`delete`/`malloc`/`free` in Monitoring.cpp
  - [x] `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` bracket every snapshot copy
  - [x] No accented characters in any Monitoring file (NFR-08)
  - [x] Guard follows `APP_TASKS_MONITORING_H` convention (NFR-06)
  - [x] Private members prefixed `_` (NFR-06)
  - [x] `MONITORING_POLL_MS` added to `Config.h` (NFR-10)

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Config.h                          ← UPDATE (add MONITORING_POLL_MS)
├── Services/
│   ├── BusFormat.h                   ← UPDATE (add altStale declaration)
│   └── BusFormat.cpp                 ← UPDATE (add altStale implementation)
├── Tasks/
│   ├── Monitoring.h                  ← NEW
│   └── Monitoring.cpp                ← NEW
└── SystemInit/
    └── SystemInit.cpp                ← UPDATE (add Monitoring, OdoControl snapshot init)

Tests/
├── CMakeLists.txt                    ← UPDATE (add MonitoringTest)
├── run_tests.sh                      ← UPDATE (add MonitoringTest to TESTS array)
├── Stubs/
│   └── StaticDefs.cpp                ← UPDATE (add OdoControl + SensorManager snapshot defs)
└── Unit/
    └── MonitoringTest.cpp            ← NEW
```

**Do NOT touch:** `App/Tasks/OdoControl.h/.cpp`, `App/Tasks/SensorManager.h/.cpp`, `App/Tasks/ExternalComm.h/.cpp`, `App/Interfaces/`, `Core/`, `Drivers/`, `Middlewares/`

---

### CRITICAL: Monitoring Has No Snapshot of Its Own

Unlike `OdoControl` or `SensorManager`, `Monitoring` does not expose a `latestSnapshot`. It is a pure reader/publisher: it reads from the three modules' snapshots and writes to IBus. No other module reads Monitoring's state. Do NOT add a snapshot struct to Monitoring.

---

### CRITICAL: Stale Threshold Is Strictly Greater-Than

The check is `>` not `>=`:
```cpp
if (HAL_GetTick() - snap.timestamp > Config::MONITORING_STALE_MS)
```
At exactly `MONITORING_STALE_MS` (e.g., 500), the condition is **false** (NOT stale). At `501`, it is **true** (stale). This matters for the boundary tests.

---

### Monitoring.h — Exact Structure

```cpp
#ifndef APP_TASKS_MONITORING_H
#define APP_TASKS_MONITORING_H

#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Interfaces/IBus.h"
#include "Services/BusFormat.h"
#include "Config.h"
#include <FreeRTOS.h>
#include <task.h>
#include "stm32f4xx_hal.h"

class Monitoring {
public:
    Monitoring(IBus* bus);
    static void task(void* param);
    void checkOnce();

private:
    IBus* _bus;
};

#endif // APP_TASKS_MONITORING_H
```

---

### Monitoring.cpp — Full Implementation

```cpp
#include "Tasks/Monitoring.h"

Monitoring::Monitoring(IBus* bus) : _bus(bus) {}

void Monitoring::task(void* param) {
    auto* self = static_cast<Monitoring*>(param);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(Config::MONITORING_POLL_MS));
        self->checkOnce();
    }
}

void Monitoring::checkOnce() {
    uint32_t now = HAL_GetTick();

    OdoControl::OdoSnapshot odoSnap;
    taskENTER_CRITICAL();
    odoSnap = OdoControl::latestSnapshot;
    taskEXIT_CRITICAL();
    if (now - odoSnap.timestamp > Config::MONITORING_STALE_MS)
        _bus->publish(Topic::ALERT, BusFormat::altStale("ODO"));

    SensorManager::SensorSnapshot sensorSnap;
    taskENTER_CRITICAL();
    sensorSnap = SensorManager::latestSnapshot;
    taskEXIT_CRITICAL();
    if (now - sensorSnap.timestamp > Config::MONITORING_STALE_MS)
        _bus->publish(Topic::ALERT, BusFormat::altStale("SENSOR"));

    ExternalComm::CommSnapshot commSnap;
    taskENTER_CRITICAL();
    commSnap = ExternalComm::latestSnapshot;
    taskEXIT_CRITICAL();
    if (now - commSnap.timestamp > Config::MONITORING_STALE_MS)
        _bus->publish(Topic::ALERT, BusFormat::altStale("COMM"));
}
```

**Why `HAL_GetTick()` once, stored in `now`?** A single `now` guarantees consistent comparison across all three checks within one `checkOnce()` call. Calling `HAL_GetTick()` three times could yield slightly different values if a tick fires between calls.

**Why no critical section around `HAL_GetTick()`?** `HAL_GetTick()` reads a single 32-bit value updated by the SysTick ISR — already atomic on ARM Cortex-M.

---

### BusFormat::altStale — New Method

In `BusFormat.h`, add after `altEncoderFault`:
```cpp
static const char* altStale(const char* module);
```

In `BusFormat.cpp`, add after `altEncoderFault`:
```cpp
const char* BusFormat::altStale(const char* module) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT STALE %s\n", module);
    return buf;
}
```

Pattern is identical to `altEncoderFault` but with `"ALT STALE"` prefix. Buffer size 32 is sufficient (`"ALT STALE SENSOR\n"` = 18 chars).

---

### Config.h — Add MONITORING_POLL_MS

Add immediately after `MONITORING_STALE_MS`:
```cpp
static constexpr uint32_t MONITORING_POLL_MS     = 100;
```

100ms polling gives 5 checks per `MONITORING_STALE_MS = 500ms` window — reliable stale detection without burning CPU.

---

### SystemInit.cpp — Exact Additions

**Add include** (after `#include "Tasks/ActuatorManager.h"`):
```cpp
#include "Tasks/Monitoring.h"
```

**Add static instance** (after `extComm` declaration, since Monitoring receives `IBus* = &extComm`):
```cpp
static Monitoring monitoring{&extComm};
```

**In `boot()`** (add after `SensorManager::latestSnapshot = { };`):
```cpp
OdoControl::latestSnapshot = { };
```

**In `boot()`** (add after the SensorManager xTaskCreate):
```cpp
xTaskCreate(Monitoring::task, "Monitor", Config::STACK_MONITORING, &monitoring,
    Config::PRIO_MONITORING, nullptr);
```

**Ordering rationale:** `monitoring` must be declared after `extComm` because its constructor stores `&extComm` as `IBus*`. All statics are zero-initialized before `boot()` runs, so the pointer is valid.

---

### StaticDefs.cpp — Exact Additions

Current content of `Tests/Stubs/StaticDefs.cpp`:
```cpp
#include "Stubs/HalStub.h"
#include "Tasks/ExternalComm.h"
ExternalComm::CommSnapshot ExternalComm::latestSnapshot{};
```

Add after the existing content:
```cpp
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
OdoControl::OdoSnapshot    OdoControl::latestSnapshot{};
SensorManager::SensorSnapshot SensorManager::latestSnapshot{};
```

**Why no duplicate symbol errors?**
- `OdoControlTest` links `OdoControl.cpp` (which defines `OdoControl::latestSnapshot`) but does NOT link `StaticDefs.cpp`
- `SensorManagerTest` links `SensorManager.cpp` (which defines `SensorManager::latestSnapshot`) but does NOT link `StaticDefs.cpp`
- `MonitoringTest` links `StaticDefs.cpp` but NOT `OdoControl.cpp` or `SensorManager.cpp`
- `ExternalCommTest` links `StaticDefs.cpp` but NOT `OdoControl.cpp` or `SensorManager.cpp`
  → No target ever defines the same symbol twice.

---

### MonitoringTest.cpp — Full Test Fixture and Test List

```cpp
#include "Stubs/HalStub.h"       // MUST be first — provides setMockTick, HAL_GetTick
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
        OdoControl::latestSnapshot     = {};
        SensorManager::latestSnapshot  = {};
        ExternalComm::latestSnapshot   = {};
    }
};
```

**Required tests (minimum 8):**

| Test name | What it validates |
|-----------|-------------------|
| `OdoFreshNoAlert` | tick=0, odoSnap.timestamp=0 → `0-0=0`, NOT > 500 → no ALERT published |
| `OdoStalePublishesAltStaleOdo` | tick=501, odoSnap.timestamp=0 → stale → ALERT with "ODO" |
| `SensorStalePublishesAltStaleSensor` | tick=501, sensorSnap.timestamp=0 → ALERT with "SENSOR" |
| `CommStalePublishesAltStaleComm` | tick=501, commSnap.timestamp=0 → ALERT with "COMM" |
| `AllFreshNoAlert` | tick=499, all timestamps=0 → 499 NOT > 500 → zero ALERTs |
| `AllStaleThreeAlerts` | tick=501, all timestamps=0 → 3 ALERTs published |
| `OnlyOdoStaleOneAlert` | tick=501, only odoSnap stale (sensor+comm have timestamp=501) → exactly 1 ALERT |
| `StaleThresholdBoundary` | tick=500, timestamp=0 → `500 > 500` = false → no alert; tick=501 → alert |

**Test implementation examples:**

```cpp
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

TEST_F(MonitoringTest, StaleThresholdBoundary) {
    setMockTick(500);
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT));  // 500 > 500 is false

    bus.clear();
    setMockTick(501);
    mon.checkOnce();
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));   // 501 > 500 is true
}

TEST_F(MonitoringTest, OnlyOdoStaleOneAlert) {
    setMockTick(501);
    SensorManager::latestSnapshot.timestamp  = 501;
    ExternalComm::latestSnapshot.timestamp   = 501;
    Monitoring mon(&bus);
    mon.checkOnce();
    EXPECT_EQ(1u, bus.count(Topic::ALERT));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT, "ODO"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "SENSOR"));
    EXPECT_FALSE(bus.hasPublished(Topic::ALERT, "COMM"));
}
```

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| No `snprintf` inline | FR-09 | Use `BusFormat::altStale()` exclusively |
| No `new`/`delete` in `App/` | NFR-02 | Monitoring instance is `static` in SystemInit |
| English identifiers | NFR-08 | All code in English |
| Guard convention | NFR-06 | `APP_TASKS_MONITORING_H` |
| Critical section for multi-field read | NFR-04 | Bracket each struct copy with `taskENTER_CRITICAL/EXIT_CRITICAL` |
| IBus is publish-only | FR-03 | Monitoring never reads from IBus — only writes to it |
| No task calls another task directly | Arch boundary #2 | Monitoring reads static structs, never calls methods of OdoControl/SensorManager/ExternalComm |
| Stacks and priorities in Config.h | NFR-10 | `STACK_MONITORING = 256`, `PRIO_MONITORING = 1` already defined |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes to `Core/`, `Drivers/`, `Middlewares/` |
| `SystemInit` sole wiring point | FR-11 | Only `SystemInit.cpp` instantiates Monitoring and calls xTaskCreate |

---

### What Already Exists — Do Not Recreate

- `App/Interfaces/IBus.h` — `publish(Topic, const char*)`, `Topic` enum (Story 1.2)
- `App/Services/BusFormat.h/.cpp` — all existing methods; only `altStale` is new
- `App/Config.h` — `MONITORING_STALE_MS = 500`, `STACK_MONITORING = 256`, `PRIO_MONITORING = 1` — already defined
- `App/Tasks/OdoControl.h` — `OdoSnapshot` with `x, y, angle, vLeft, vRight, rawV, rawW, timestamp`
- `App/Tasks/SensorManager.h` — `SensorSnapshot` with `values[15], alarms[15], count, timestamp`
- `App/Tasks/ExternalComm.h` — `CommSnapshot` with `rxCount, txCount, lastCmd[32], timestamp`
- `Tests/Mocks/MockBus.h` — `published` vector, `hasPublished()`, `count()`, `clear()` (Story 1.6)
- `Tests/Stubs/FreeRTOS.h` / `FreeRTOSStub.cpp` — `taskENTER_CRITICAL` is a no-op on host; `vTaskDelay` throws `TaskDelayEscape`
- `Tests/Stubs/HalStub.h` — `setMockTick(v)` / `HAL_GetTick()` controllable in tests

---

### Test Build Does NOT Exercise the `task()` Loop

`Monitoring::task()` contains a `for(;;)` with `vTaskDelay()`. On host, `vTaskDelay()` throws `TaskDelayEscape`. Do NOT call `Monitoring::task()` in tests — call `checkOnce()` directly. This is the same pattern as `SensorManager::pollOnce()` and `OdoControl::tick()`.

---

### Previous Story Learnings (from Story 3.2)

- Include `Stubs/HalStub.h` FIRST in test files that use `HAL_GetTick()` / `setMockTick()` — otherwise the HAL stub may not shadow the real header correctly
- `FreeRTOSStub.cpp` is a shared stub included in every test target — add it to the new CMake target
- `StaticDefs.cpp` is used by `ExternalCommTest`; adding OdoControl/SensorManager snapshot defs there is safe because neither `OdoControl.cpp` nor `SensorManager.cpp` are in `ExternalCommTest`'s sources
- `BusFormat.cpp` must be included in every test target that calls BusFormat methods
- `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` are no-ops on host (defined in `FreeRTOS.h` stub) — they compile away silently in tests; behavior is validated via the snapshot copy semantics
- `vTaskDelay` throws `TaskDelayEscape` in tests — never call `task()` directly in unit tests; always test via the public `checkOnce()` entry point

---

### References

- Story requirements: [epics.md — Story 3.3](..//planning-artifacts/epics.md)
- FR-08 (Monitoring pull on snapshots): [epics.md — Requirements Inventory](..//planning-artifacts/epics.md)
- Architecture pull model: [architecture.md — Monitoring Accès aux données](..//planning-artifacts/architecture.md)
- Architecture NFR-04 (critical sections): [architecture.md — NFR-04](_bmad-output/planning-artifacts/architecture.md)
- OdoControl snapshot struct: [OdoControl.h](Wall-A-STM/App/Tasks/OdoControl.h#L30-L40)
- SensorManager snapshot struct: [SensorManager.h](Wall-A-STM/App/Tasks/SensorManager.h#L13-L19)
- ExternalComm snapshot struct: [ExternalComm.h](Wall-A-STM/App/Tasks/ExternalComm.h#L15-L20)
- BusFormat existing methods (pattern for altStale): [BusFormat.cpp](Wall-A-STM/App/Services/BusFormat.cpp)
- SystemInit current wiring: [SystemInit.cpp](Wall-A-STM/App/SystemInit/SystemInit.cpp)
- StaticDefs.cpp current content: [StaticDefs.cpp](Wall-A-STM/Tests/Stubs/StaticDefs.cpp)
- HalStub.h (setMockTick / HAL_GetTick): [HalStub.h](Wall-A-STM/Tests/Stubs/HalStub.h)
- FreeRTOS stub (taskENTER_CRITICAL no-op): [FreeRTOS.h](Wall-A-STM/Tests/Stubs/FreeRTOS.h)
- SensorManagerTest pattern (snapshot reset in SetUp): [SensorManagerTest.cpp](Wall-A-STM/Tests/Unit/SensorManagerTest.cpp)
- Previous story (3.2) learnings: [3-2-actuatormanager.md](_bmad-output/implementation-artifacts/3-2-actuatormanager-iactuator-max-actuators-commandbyid.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

Aucun blocage rencontré. Implémentation conforme aux Dev Notes sans dérogation.

### Completion Notes List

- BusFormat::altStale(module) ajouté en suivant exactement le pattern altEncoderFault (buf[32], snprintf)
- MONITORING_POLL_MS = 100 ajouté dans Config.h immédiatement après MONITORING_STALE_MS
- Monitoring.h créé avec guard APP_TASKS_MONITORING_H, structure exacte conforme aux Dev Notes
- Monitoring.cpp : HAL_GetTick() appelé une seule fois (stocké dans `now`), chaque copie de snapshot encadrée par taskENTER_CRITICAL/taskEXIT_CRITICAL, BusFormat::altStale() exclusivement utilisé
- SystemInit.cpp : monitoring déclaré après extComm, OdoControl::latestSnapshot initialisé dans boot(), xTaskCreate ajouté après SensorManager
- StaticDefs.cpp : OdoControl et SensorManager snapshot definitions ajoutés — aucun conflit de symboles (OdoControlTest et SensorManagerTest n'incluent pas StaticDefs.cpp)
- MonitoringTest.cpp : 8 tests couvrant les cas frais, stales individuels, tous stales, limites de seuil (500 = pas stale, 501 = stale)
- CMakeLists.txt : cible MonitoringTest avec sources exactes, gtest_discover_tests ajouté
- run_tests.sh : MonitoringTest ajouté en fin de tableau TESTS

### File List

- Wall-A-STM/App/Services/BusFormat.h (modifié)
- Wall-A-STM/App/Services/BusFormat.cpp (modifié)
- Wall-A-STM/App/Config.h (modifié)
- Wall-A-STM/App/Tasks/Monitoring.h (nouveau)
- Wall-A-STM/App/Tasks/Monitoring.cpp (nouveau)
- Wall-A-STM/App/SystemInit/SystemInit.cpp (modifié)
- Wall-A-STM/Tests/Stubs/StaticDefs.cpp (modifié)
- Wall-A-STM/Tests/Unit/MonitoringTest.cpp (nouveau)
- Wall-A-STM/Tests/CMakeLists.txt (modifié)
- Wall-A-STM/Tests/run_tests.sh (modifié)

### Change Log

- Implémentation Story 3.3 — Monitoring supervision par pull sur shared memory horodatée (Date: 2026-05-14)
