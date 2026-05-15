# Story 3.2: ActuatorManager — IActuator[MAX_ACTUATORS], commandById

Status: review

## Story

As a developer,
I want `ActuatorManager` to implement `IActuatorManager`, orchestrate up to 10 `IActuator*` instances injected by `SystemInit`, and route `commandById(id, cmd)` to the matching actuator,
so that `ExternalComm` can dispatch actuator commands without knowing any concrete actuator type, and an unknown id logs a warn without crashing.

## Acceptance Criteria

1. **Given** `ActuatorManager` implements `IActuatorManager` with `commandById(uint8_t id, const char* cmd)`
   **When** `ExternalComm` receives `CMD ACTUATOR PUMP_1 ON\n`
   **Then** `ActuatorManager::commandById` routes the command to the `IActuator*` whose `id()` matches — that actuator's `command("ON")` is called

2. **Given** `ActuatorManager::commandById` is called with an `id` that matches no actuator
   **When** the full array is searched without a match
   **Then** `bus_->publish(Topic::LOG, BusFormat::logWarn("unknown actuator id"))` is called — no crash, no out-of-bounds access

3. **Given** `Tests/Unit/ActuatorManagerTest.cpp` exists with 10 mock `IActuator` instances
   **When** routing tests run on PC host
   **Then** each command reaches the correct mock and unknown-id calls produce a LOG warn — tests pass without STM32 hardware

4. **And** `ActuatorManager` does not know `ExternalComm` — it receives `IBus*` by constructor injection; it never includes or references `ExternalComm`

## Tasks / Subtasks

- [x] Task 1: Add `logWarn` to `App/Services/BusFormat.h` and `App/Services/BusFormat.cpp` (AC: #2)
  - [x] Add declaration to `BusFormat.h`: `static const char* logWarn(const char* msg);`
  - [x] Add implementation to `BusFormat.cpp`: `snprintf(buf, sizeof(buf), "LOG WARN %s\n", msg);`
  - [x] Verify `logInfo` already exists — `logWarn` follows the same pattern, different prefix

- [x] Task 2: Verify existing interfaces — do NOT recreate (AC: #1, #4)
  - [x] `App/Interfaces/IActuator.h` already exists: `id()`, `name()`, `command(cmd)` — do NOT touch
  - [x] `App/Interfaces/IActuatorManager.h` already exists: `commandById(uint8_t, const char*)` — do NOT touch

- [x] Task 3: Create `App/Tasks/ActuatorManager.h` (AC: #1, #2, #4)
  - [x] Guard: `#ifndef APP_TASKS_ACTUATORMANAGER_H`
  - [x] Includes: `"Interfaces/IActuatorManager.h"`, `"Interfaces/IActuator.h"`, `"Interfaces/IBus.h"`, `"Config.h"`, `<cstdint>`
  - [x] Constructor: `ActuatorManager(IActuator** actuators, uint8_t actuatorCount, IBus* bus);`
  - [x] Override: `void commandById(uint8_t id, const char* cmd) override;`
  - [x] Private members: `IActuator** _actuators`, `uint8_t _actuatorCount`, `IBus* _bus`
  - [x] **No** `static void task(void*)` — ActuatorManager has NO FreeRTOS task, no stack/priority constants needed
  - [x] **No** `static_cast`/`dynamic_cast` on `IActuator*` pointers — only interface calls

- [x] Task 4: Create `App/Tasks/ActuatorManager.cpp` (AC: #1, #2)
  - [x] Constructor: store pointers, nothing else
  - [x] `commandById`: full algorithm — see Dev Notes for exact implementation
  - [x] Never `snprintf` inline — use `BusFormat::logWarn()` for the unknown-id path
  - [x] Never `new`/`delete`/`malloc`/`free`

- [x] Task 5: Update `App/SystemInit/SystemInit.cpp` — replace stub with real ActuatorManager (AC: #1)
  - [x] Remove: `#include "Tasks/StubActuatorManager.h"` and `static StubActuatorManager stubActMgr{};`
  - [x] Add: `#include "Tasks/ActuatorManager.h"`
  - [x] Add stub actuator array: `static IActuator* actuators[Config::MAX_ACTUATORS] = {};` and `static uint8_t actuatorCount = 0;`
  - [x] Replace: `static ActuatorManager actuatorMgr{actuators, actuatorCount, nullptr};` + `actuatorMgr.setBus(&extComm)` in boot()
  - [x] Update ExternalComm construction: replace `&stubActMgr` with `&actuatorMgr`
  - [x] **Story 3.4 will only modify `actuators[]` and `actuatorCount`** — ActuatorManager itself stays unchanged (NFR-05)

- [x] Task 6: Create `Tests/Mocks/MockActuator.h` (AC: #3)
  - [x] Guard: `#ifndef TESTS_MOCKS_MOCKACTUATOR_H`
  - [x] Class `MockActuator : public IActuator` with all 3 methods + test state tracking
  - [x] Track `lastCmd` (last command received) and `commandCallCount`
  - [x] See Dev Notes for exact implementation

- [x] Task 7: Create `Tests/Unit/ActuatorManagerTest.cpp` (AC: #3)
  - [x] Include pattern: `FreeRTOS.h` (for Config.h) before `ActuatorManager.h`
  - [x] See Dev Notes for test list

- [x] Task 8: Update `Tests/CMakeLists.txt` — add ActuatorManagerTest target (AC: #3)
  - [x] Target: `ActuatorManagerTest` with sources: `Unit/ActuatorManagerTest.cpp`, `${APP_DIR}/Tasks/ActuatorManager.cpp`, `${APP_DIR}/Services/BusFormat.cpp`, `Stubs/FreeRTOSStub.cpp`
  - [x] Include directories: `${COMMON_INCLUDES}` (already defined)
  - [x] Link: `GTest::gtest_main`
  - [x] Add `gtest_discover_tests(ActuatorManagerTest)` at the bottom

- [x] Task 9: Verify NFR compliance
  - [x] No `static_cast`/`dynamic_cast` on `IActuator*` in ActuatorManager
  - [x] No `snprintf` inline in ActuatorManager.cpp
  - [x] No `new`/`delete`/`malloc`/`free` in ActuatorManager.cpp
  - [x] No accented characters in any ActuatorManager file (NFR-08)
  - [x] Guard follows `APP_TASKS_ACTUATORMANAGER_H` convention (NFR-06)
  - [x] Private members prefixed `_` (NFR-06)

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Services/
│   ├── BusFormat.h                   ← UPDATE (add logWarn declaration)
│   └── BusFormat.cpp                 ← UPDATE (add logWarn implementation)
├── Tasks/
│   ├── ActuatorManager.h             ← NEW
│   ├── ActuatorManager.cpp           ← NEW
│   └── StubActuatorManager.h         ← DELETE (replaced by real ActuatorManager)
└── SystemInit/
    └── SystemInit.cpp                ← UPDATE (replace stub, wire ActuatorManager)

Tests/
├── CMakeLists.txt                    ← UPDATE (add ActuatorManagerTest)
├── Mocks/
│   └── MockActuator.h                ← NEW
└── Unit/
    └── ActuatorManagerTest.cpp       ← NEW
```

**Do NOT touch:** `App/Interfaces/IActuator.h`, `App/Interfaces/IActuatorManager.h`, `App/Config.h`, `App/Tasks/SensorManager.h/.cpp`, `App/Tasks/ExternalComm.h/.cpp`, `Core/`, `Drivers/`, `Middlewares/`

---

### CRITICAL: ActuatorManager Has NO FreeRTOS Task

Unlike `SensorManager`, `ActuatorManager` has no polling task. `ExternalComm::_processRxLine` calls `_actuatorMgr->commandById(id, actCmd)` synchronously when parsing `CMD ACTUATOR`. ActuatorManager just routes the call — no `static void task(void*)`, no stack constant, no `xTaskCreate` in SystemInit.

---

### How ExternalComm Dispatches to ActuatorManager (Context for Wiring)

In `ExternalComm::_processRxLine` (already implemented, **do not modify**):
```cpp
} else if (strncmp(verb, "ACTUATOR", 8) == 0) {
    char actorId[16]{}, actCmd[32]{};
    sscanf(line, "%*s %*s %15s %31s", actorId, actCmd);
    const char* underscore = strrchr(actorId, '_');
    uint8_t id = underscore ? static_cast<uint8_t>(atoi(underscore + 1)) : 0;
    if (_actuatorMgr)
        _actuatorMgr->commandById(id, actCmd);
}
```
So `CMD ACTUATOR PUMP_1 ON\n` → id=1, cmd="ON".

---

### ActuatorManager.h — Exact Structure

```cpp
#ifndef APP_TASKS_ACTUATORMANAGER_H
#define APP_TASKS_ACTUATORMANAGER_H

#include "Interfaces/IActuatorManager.h"
#include "Interfaces/IActuator.h"
#include "Interfaces/IBus.h"
#include "Config.h"
#include <cstdint>

class ActuatorManager : public IActuatorManager {
public:
    ActuatorManager(IActuator** actuators, uint8_t actuatorCount, IBus* bus);
    void commandById(uint8_t id, const char* cmd) override;

private:
    IActuator** _actuators;
    uint8_t     _actuatorCount;
    IBus*       _bus;
};

#endif // APP_TASKS_ACTUATORMANAGER_H
```

---

### ActuatorManager::commandById — Full Algorithm

```cpp
void ActuatorManager::commandById(uint8_t id, const char* cmd) {
    for (uint8_t i = 0; i < _actuatorCount && i < Config::MAX_ACTUATORS; ++i) {
        if (_actuators[i] == nullptr) continue;
        if (_actuators[i]->id() == id) {
            _actuators[i]->command(cmd);
            return;
        }
    }
    _bus->publish(Topic::LOG, BusFormat::logWarn("unknown actuator id"));
}
```

**Why `i < _actuatorCount && i < Config::MAX_ACTUATORS`?** Defensive double-guard — `_actuatorCount` comes from SystemInit; the `MAX_ACTUATORS` cap prevents array overflow if ever misconfigured.

**Why `return` after first match?** IDs are unique by design. Stopping at first match avoids unnecessary iteration and prevents double-commanding.

**Why nullptr check?** Array is initialized sparse in SystemInit (populated in Story 3.4); nullptrs are valid until concrete actuators are wired.

---

### BusFormat::logWarn — New Method (mirrors logInfo)

In `BusFormat.h`, add after `logInfo`:
```cpp
static const char* logWarn(const char* msg);
```

In `BusFormat.cpp`, add after `logInfo` implementation:
```cpp
const char* BusFormat::logWarn(const char* msg) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "LOG WARN %s\n", msg);
    return buf;
}
```

Pattern is identical to `logInfo` but with `"LOG WARN"` prefix instead of `"LOG INFO"`.

---

### SystemInit.cpp — Exact Diff (Actuator Wiring)

**Remove:**
```cpp
#include "Tasks/StubActuatorManager.h"
// ...
static StubActuatorManager stubActMgr{};
// ...
static ExternalComm extComm{&uartCh, &usbCh, nullptr, &stubActMgr, cmdMailbox};
```

**Add:**
```cpp
#include "Tasks/ActuatorManager.h"
// ...
static IActuator* actuators[Config::MAX_ACTUATORS] = {};  // populated by Story 3.4
static uint8_t    actuatorCount = 0;                       // updated by Story 3.4
static ActuatorManager actuatorMgr{actuators, actuatorCount, &extComm};
// ...
static ExternalComm extComm{&uartCh, &usbCh, nullptr, &actuatorMgr, cmdMailbox};
```

**Note:** `extComm` is declared after `actuatorMgr` but `ActuatorManager` constructor receives `&extComm`. Since `extComm` is initialized as a `static` local, its address is fixed at link time — this is safe. Verify this ordering compiles. If not, use a forward pointer and set after init.

Actually **IMPORTANT ordering issue**: `actuatorMgr` is constructed with `&extComm`, but `extComm` must be declared first for `&extComm` to be valid. Check the current SystemInit.cpp order: `extComm` is declared AFTER `stubActMgr`. Since `ActuatorManager` just stores the `IBus*` pointer (doesn't call it in constructor), the pointer is set to valid storage at the time `boot()` runs (both statics are zero-initialized before `boot()` executes). This is safe for static locals with deferred usage. Confirm by matching the existing declaration order in SystemInit.cpp.

---

### MockActuator.h — Exact Implementation

```cpp
#ifndef TESTS_MOCKS_MOCKACTUATOR_H
#define TESTS_MOCKS_MOCKACTUATOR_H

#include "Interfaces/IActuator.h"
#include <cstring>
#include <cstdint>

class MockActuator : public IActuator {
public:
    MockActuator(uint8_t id, const char* name)
        : _id(id), _name(name) {}

    uint8_t     id()   const override { return _id; }
    const char* name() const override { return _name; }
    void command(const char* cmd) override {
        strncpy(_lastCmd, cmd ? cmd : "", sizeof(_lastCmd) - 1);
        _lastCmd[sizeof(_lastCmd) - 1] = '\0';
        ++_commandCallCount;
    }

    const char* lastCmd()         const { return _lastCmd; }
    uint32_t    commandCallCount()const { return _commandCallCount; }
    void        reset()                 { _lastCmd[0] = '\0'; _commandCallCount = 0; }

private:
    uint8_t     _id;
    const char* _name;
    char        _lastCmd[32]{};
    uint32_t    _commandCallCount{};
};

#endif // TESTS_MOCKS_MOCKACTUATOR_H
```

---

### ActuatorManagerTest.cpp — Test Fixture and Full Test List

```cpp
#include <gtest/gtest.h>
#include "Stubs/FreeRTOS.h"   // for Config.h (UBaseType_t)
#include "Mocks/MockBus.h"
#include "Mocks/MockActuator.h"
#include "Tasks/ActuatorManager.h"

class ActuatorManagerTest : public ::testing::Test {
protected:
    MockBus bus;
    void SetUp() override { bus.clear(); }
};
```

**Required tests (minimum 8):**

| Test name | What it validates |
|-----------|-------------------|
| `SingleActuatorRouted` | 1 actuator id=1, `commandById(1, "ON")` → actuator receives "ON", no LOG published |
| `UnknownIdPublishesLogWarn` | No actuator has id=99, `commandById(99, "X")` → LOG WARN published |
| `EmptyArrayAlwaysLogs` | 0 actuators, any `commandById` → LOG WARN |
| `NullActuatorSkipped` | Array with nullptr at index 0, actuator at index 1 id=2 → routes to index 1 |
| `TenActuatorsEachRouted` | 10 actuators id=0..9, each `commandById(i, "CMD")` → each mock gets 1 call |
| `WrongIdAmongTen` | 10 actuators id=0..9, `commandById(100, "X")` → LOG WARN, no actuator called |
| `FirstMatchWins` | 2 actuators with same id (edge case) — only first is called, no double dispatch |
| `CommandStringPassedThrough` | `commandById(3, "EXTEND 50")` → actuator at id=3 receives exactly "EXTEND 50" |

---

### IActuator Interface — Already Exists (Do NOT Recreate)

```cpp
// App/Interfaces/IActuator.h — ALREADY IMPLEMENTED, DO NOT MODIFY
class IActuator {
public:
    virtual uint8_t     id()   const = 0;
    virtual const char* name() const = 0;
    virtual void command(const char* cmd) = 0;
    virtual ~IActuator() = default;
};
```

**Important:** `id()` and `name()` are `const` in the existing interface. MockActuator must match this (`const` qualifier).

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| No `static_cast`/`dynamic_cast` on IActuator | FR-06 (NFR-05) | Interface calls only |
| No `snprintf` inline | FR-09 | Use `BusFormat::logWarn()` exclusively |
| No `new`/`delete` in `App/` | NFR-02 | ActuatorManager instance is `static` in SystemInit |
| English identifiers | NFR-08 | All code in English |
| Guard convention | NFR-06 | `APP_TASKS_ACTUATORMANAGER_H` |
| `SystemInit` sole wiring point | FR-11 | Only SystemInit.cpp touches the actuator array |
| Extending ActuatorManager = new IActuator class only | NFR-05 | Story 3.4 will add concrete actuators without modifying ActuatorManager |
| IBus via constructor injection | FR-06 | ActuatorManager receives `IBus*` — never instantiates IBus internally |
| No task creation inside ActuatorManager | FR-11 | Only SystemInit calls `xTaskCreate` — ActuatorManager has no task |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes to `Core/`, `Drivers/`, `Middlewares/` |

---

### What Already Exists — Do Not Recreate

- `App/Interfaces/IActuator.h` — `id() const`, `name() const`, `command(cmd)` (Story 1.2)
- `App/Interfaces/IActuatorManager.h` — `commandById(uint8_t id, const char* cmd)` (Story 1.2)
- `App/Interfaces/IBus.h` — `publish(Topic, const char*)`, `Topic` enum (Story 1.2)
- `App/Services/BusFormat.h/.cpp` — `logInfo`, `telOdo`, etc. already defined; only `logWarn` is new
- `App/Tasks/StubActuatorManager.h` — REPLACE this stub with real ActuatorManager in SystemInit
- `Tests/Mocks/MockBus.h` — `published` vector, `hasPublished()`, `clear()` (Story 1.6)
- `Tests/Stubs/FreeRTOS.h`, `queue.h`, `task.h`, `FreeRTOSStub.cpp` — all stubs ready
- `Config::MAX_ACTUATORS = 10` — already in Config.h, do NOT duplicate

---

### Deferred Work

**Story 3.4** will:
- Create concrete `Pump`, `Servo`, `LinearTransducer` implementing `IActuator`
- Populate `actuators[]` and `actuatorCount` in `SystemInit.cpp` — only file modified
- Each concrete actuator implements `command()` with its own hardware logic

**Story 3.3 (Monitoring)** — ActuatorManager exposes no snapshot. Monitoring only reads `OdoControl::latestSnapshot`, `SensorManager::latestSnapshot`, `ExternalComm::latestSnapshot`. No snapshot needed here.

---

### Previous Story Learnings (from Story 3.1)

- Include `Stubs/FreeRTOS.h` (not `Stubs/MockHAL.h`) first in test file — ActuatorManager doesn't use `HAL_GetTick()`, so MockHAL.h is NOT required
- `FreeRTOSStub.cpp` is already in the test build as a shared stub; add it to the new CMake target
- `vTaskDelay` throws `TaskDelayEscape` in tests — not relevant here (no task loop), but still needed for FreeRTOS stub compilation
- Pattern for static array initialization: `static IActuator* actuators[Config::MAX_ACTUATORS] = {};` — mirrors the SensorManager wiring
- `BusFormat.cpp` must be included in every test target that calls BusFormat methods

---

### References

- Story requirements: [epics.md — Story 3.2](_bmad-output/planning-artifacts/epics.md)
- FR-06 (ActuatorManager IActuator[MAX_ACTUATORS]): [epics.md — Requirements Inventory](_bmad-output/planning-artifacts/epics.md)
- Architecture Tasks/ folder criteria: [architecture.md — Folder Belonging Criteria](_bmad-output/planning-artifacts/architecture.md)
- Architecture boundaries (no task calls task directly): [architecture.md — Architectural Boundaries](_bmad-output/planning-artifacts/architecture.md)
- ExternalComm dispatch logic (CMD ACTUATOR): [ExternalComm.cpp — _processRxLine](Wall-A-STM/App/Tasks/ExternalComm.cpp#L151)
- SensorManager wiring pattern (mirror for ActuatorManager in SystemInit): [3-1-sensormanager.md — SystemInit wiring](_bmad-output/implementation-artifacts/3-1-sensormanager-polling-isensor-max-sensors-alarmes-xtasknotify.md)
- IActuator interface (existing): [IActuator.h](Wall-A-STM/App/Interfaces/IActuator.h)
- BusFormat.cpp (logInfo pattern to mirror for logWarn): [BusFormat.cpp](Wall-A-STM/App/Services/BusFormat.cpp)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

- Dépendance circulaire `actuatorMgr` ↔ `extComm` résolue via `setBus(IBus*)` appelé dans `boot()` avant `xTaskCreate`. Pattern "forward pointer" suggéré par les Dev Notes.

### Completion Notes List

- `BusFormat::logWarn()` ajoutée, suit exactement le patron de `logInfo`.
- `ActuatorManager` créé sans tâche FreeRTOS — routage synchrone via `commandById` appelé depuis `ExternalComm::_processRxLine`.
- `StubActuatorManager` retiré de `SystemInit.cpp` et remplacé par la vraie implémentation. `IBus*` branché via `setBus(&extComm)` dans `boot()` pour casser la dépendance circulaire.
- 8 tests unitaires écrits et passants (8/8). Aucune régression sur les 6 autres suites (53 tests au total).
- Tous les NFR respectés : pas de cast, pas de snprintf inline, pas de new/delete, identifiants ASCII, convention de garde et préfixe `_`.

### File List

- Wall-A-STM/App/Services/BusFormat.h (modifié — ajout logWarn)
- Wall-A-STM/App/Services/BusFormat.cpp (modifié — implémentation logWarn)
- Wall-A-STM/App/Tasks/ActuatorManager.h (nouveau)
- Wall-A-STM/App/Tasks/ActuatorManager.cpp (nouveau)
- Wall-A-STM/App/Tasks/StubActuatorManager.h (supprimé)
- Wall-A-STM/App/SystemInit/SystemInit.cpp (modifié — remplacement stub, câblage ActuatorManager)
- Wall-A-STM/Tests/Mocks/MockActuator.h (nouveau)
- Wall-A-STM/Tests/Unit/ActuatorManagerTest.cpp (nouveau)
- Wall-A-STM/Tests/CMakeLists.txt (modifié — cible ActuatorManagerTest)
- Wall-A-STM/Tests/run_tests.sh (modifié — ActuatorManagerTest ajouté à la liste)

### Change Log

- 2026-05-14: Implémentation complète Story 3.2 — ActuatorManager avec IActuator[MAX_ACTUATORS], commandById, logWarn, remplacement StubActuatorManager, 8 tests unitaires (claude-sonnet-4-6)
