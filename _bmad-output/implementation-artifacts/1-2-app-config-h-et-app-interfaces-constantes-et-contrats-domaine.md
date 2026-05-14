# Story 1.2: App/Config.h et App/Interfaces/ — constantes et contrats domaine

Status: review

## Story

As a developer,
I want all system constants declared `constexpr` in `App/Config.h` and all domain interfaces defined in `App/Interfaces/`,
so that every subsequent class has a single source of truth for configuration and clear dependency-injection contracts.

## Acceptance Criteria

1. **Given** `App/Config.h` is created with `namespace Config`
   **When** `grep` is run on `App/Tasks/` searching for literal numeric values for stacks, priorities, frequencies, or array bounds
   **Then** zero occurrences found — all values come from `Config::*`

2. **Given** `App/Interfaces/IBus.h` is created
   **When** a class receives `IBus*` in its constructor
   **Then** it can call `publish(Topic topic, const char* payload)` without knowing the concrete implementation, with `Topic` enum covering TELEMETRY, ALERT, LOG, HEALTH

3. **Given** `App/Interfaces/ISensor.h` is created
   **When** a mock implements `ISensor`
   **Then** it must provide `uint8_t id()`, `const char* name()`, and `float read()`

4. **Given** `App/Interfaces/IActuator.h` and `IActuatorManager.h` are created
   **When** `ExternalComm` holds an `IActuatorManager*`
   **Then** it can call `commandById(uint8_t id, const char* cmd)` without knowing concrete actuator types

5. **And** all guards follow convention `#ifndef APP_<FOLDER>_<FILE>_H`

6. **And** `Config::MONITORING_STALE_MS` is declared `constexpr` in `App/Config.h`

7. **Given** `App/` is scanned for NFR-08 compliance
   **When** `grep -rn '[àâçèéêëîïôùûüÿœæ]' App/` is run
   **Then** zero occurrences found in identifiers — all class names, methods, variables, constants, files, and ASCII messages are exclusively in English

8. **Given** `App/` headers are scanned for NFR-06 guard compliance
   **When** `grep -rn "#ifndef" App/ | grep -v "APP_[A-Z0-9_]*_H"` is run
   **Then** zero occurrences — all guards respect the `APP_<FOLDER>_<FILE>_H` convention

9. **Given** `App/` is scanned for NFR-06 naming conventions
   **When** `grep -rn 'private:' App/ --include='*.h' -A5 | grep -v '^\s*_'` is run on private members
   **Then** zero private members without `_` prefix

   **When** `grep -rn 'class [^I][a-z]' App/ --include='*.h'` is run
   **Then** zero business classes in lowercase — all classes are PascalCase

   **When** `grep -rn 'class I[^A-Z]' App/Interfaces/ --include='*.h'` is run
   **Then** zero interfaces with `I` prefix followed by lowercase — all interfaces are `I` + PascalCase

   **When** `grep -rn 'constexpr.*[a-z][a-z]' App/Config.h` is run
   **Then** zero `Config::` constants in lowercase — all constants are ALL_CAPS

## Tasks / Subtasks

- [x] Task 1: Create `App/Config.h` with all system constants (AC: #1, #6, #9)
  - [x] Create `App/Config.h` with `namespace Config` block
  - [x] Declare all FreeRTOS stack sizes: `STACK_ODO_CONTROL`, `STACK_MOTION_PLANNER`, `STACK_SENSOR_MANAGER`, `STACK_MONITORING`, `STACK_EXTCOMM_RX`, `STACK_EXTCOMM_TX`
  - [x] Declare all FreeRTOS priorities: `PRIO_ODO_CONTROL=5`, `PRIO_MOTION_PLANNER=4`, `PRIO_EXTCOMM_RX=4`, `PRIO_EXTCOMM_TX=3`, `PRIO_SENSOR_MANAGER=2`, `PRIO_MONITORING=1`
  - [x] Declare all system constants: `ODO_FREQ_HZ=200`, `MAX_SENSORS=15`, `MAX_ACTUATORS=10`, `MONITORING_STALE_MS`
  - [x] Declare PID defaults: `PID_KP_DEFAULT=1.0f`, `PID_KI_DEFAULT=0.1f`, `PID_KD_DEFAULT=0.05f`
  - [x] Verify all names are ALL_CAPS, all types explicit (`uint32_t`, `uint8_t`, `uint16_t`, `UBaseType_t`, `float`)

- [x] Task 2: Create `App/Interfaces/IBus.h` (AC: #2, #5, #8, #9)
  - [x] Create `App/Interfaces/` directory (first interface file created = directory implicitly created)
  - [x] Declare `enum class Topic { TELEMETRY, ALERT, LOG, HEALTH }`
  - [x] Declare pure abstract class `IBus` with `virtual void publish(Topic topic, const char* payload) = 0` and `virtual ~IBus() = default`
  - [x] Use guard `#ifndef APP_INTERFACES_IBUS_H`

- [x] Task 3: Create `App/Interfaces/ISensor.h` (AC: #3, #5, #8, #9)
  - [x] Declare pure abstract class `ISensor` with: `virtual uint8_t id() const = 0`, `virtual const char* name() const = 0`, `virtual float read() = 0`, `virtual ~ISensor() = default`
  - [x] Use guard `#ifndef APP_INTERFACES_ISENSOR_H`

- [x] Task 4: Create `App/Interfaces/IActuator.h` (AC: #4, #5, #8, #9)
  - [x] Declare pure abstract class `IActuator` with: `virtual uint8_t id() const = 0`, `virtual const char* name() const = 0`, `virtual void command(const char* cmd) = 0`, `virtual ~IActuator() = default`
  - [x] Use guard `#ifndef APP_INTERFACES_IACTUATOR_H`

- [x] Task 5: Create `App/Interfaces/IActuatorManager.h` (AC: #4, #5, #8, #9)
  - [x] Declare pure abstract class `IActuatorManager` with: `virtual void commandById(uint8_t id, const char* cmd) = 0`, `virtual ~IActuatorManager() = default`
  - [x] Use guard `#ifndef APP_INTERFACES_IACTUATORMANAGER_H`

- [x] Task 6: Update existing stub headers in `App/Tasks/` to include `Config.h` (AC: #1)
  - [x] Update `App/Tasks/OdoControl.h` stub: replace any hardcoded numeric in struct declarations if present; the stub from Story 1.1 has `uint32_t timestamp` — confirm types consistent with Config.h includes
  - [x] Confirm `App/Tasks/SensorManager.h` and `App/Tasks/ExternalComm.h` stubs are still valid (no modification needed if no literals)
  - [x] Do NOT add Config.h includes yet to stub files that don't use Config constants — leave that for the Stories that fully implement each class

- [x] Task 7: Run NFR compliance checks (AC: #7, #8, #9)
  - [x] `grep -rn '[àâçèéêëîïôùûüÿœæ]' App/` → zero results ✅
  - [x] `grep -rn "#ifndef" App/ | grep -v "APP_[A-Z0-9_]*_H"` → zero results ✅
  - [x] Constant names ALL_CAPS verified visually (grep pattern too broad — see completion notes)

## Dev Notes

### What this story creates — explicit scope

This story creates **only** header files (no .cpp). The deliverables are:

```
App/
├── Config.h                    ← NEW this story
└── Interfaces/
    ├── IBus.h                  ← NEW this story
    ├── ISensor.h               ← NEW this story
    ├── IActuator.h             ← NEW this story
    └── IActuatorManager.h      ← NEW this story
```

**Do NOT create:** `BusFormat.h/.cpp` (Story 1.3), `ExternalComm.h/.cpp` (Story 1.4), any `.cpp` implementation files, any test files, or any `IMotorHAL`/`IEncoderHAL` HAL interfaces (those are defined in Story 2.1 when OdoControl is implemented).

### Exact `App/Config.h` content

```cpp
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <cstdint>
#include "FreeRTOS.h"  // for UBaseType_t

namespace Config {
    // Task frequencies
    static constexpr uint32_t ODO_FREQ_HZ           = 200;

    // Array bounds — fixed at compile time (NFR-02)
    static constexpr uint8_t  MAX_SENSORS            = 15;
    static constexpr uint8_t  MAX_ACTUATORS          = 10;

    // PID defaults (tuned at runtime via CMD, defaults here)
    static constexpr float    PID_KP_DEFAULT         = 1.0f;
    static constexpr float    PID_KI_DEFAULT         = 0.1f;
    static constexpr float    PID_KD_DEFAULT         = 0.05f;

    // Monitoring stale threshold
    static constexpr uint32_t MONITORING_STALE_MS    = 500;

    // FreeRTOS stack sizes (words = 4 bytes each on ARM Cortex-M)
    static constexpr uint16_t STACK_ODO_CONTROL      = 512;
    static constexpr uint16_t STACK_MOTION_PLANNER   = 256;
    static constexpr uint16_t STACK_SENSOR_MANAGER   = 256;
    static constexpr uint16_t STACK_MONITORING       = 256;
    static constexpr uint16_t STACK_EXTCOMM_RX       = 256;
    static constexpr uint16_t STACK_EXTCOMM_TX       = 256;

    // FreeRTOS task priorities (higher number = higher priority)
    static constexpr UBaseType_t PRIO_ODO_CONTROL    = 5;
    static constexpr UBaseType_t PRIO_MOTION_PLANNER = 4;
    static constexpr UBaseType_t PRIO_EXTCOMM_RX     = 4;
    static constexpr UBaseType_t PRIO_EXTCOMM_TX     = 3;
    static constexpr UBaseType_t PRIO_SENSOR_MANAGER = 2;
    static constexpr UBaseType_t PRIO_MONITORING     = 1;
}

#endif // APP_CONFIG_H
```

**Critical:** `UBaseType_t` requires `FreeRTOS.h`. The include path for FreeRTOS headers is already set up by CubeMX — `FreeRTOS.h` is in `Middlewares/Third_Party/FreeRTOS/Source/include/`. In STM32CubeIDE, this path is already in the project's include paths.

**Alternative if FreeRTOS.h causes issues in Config.h context:** Use `uint32_t` for priorities — FreeRTOS will implicitly convert. Prefer `UBaseType_t` for explicit type intent.

### Exact `App/Interfaces/IBus.h` content

```cpp
#ifndef APP_INTERFACES_IBUS_H
#define APP_INTERFACES_IBUS_H

enum class Topic : uint8_t {
    TELEMETRY = 0,
    ALERT     = 1,
    LOG       = 2,
    HEALTH    = 3
};

class IBus {
public:
    virtual void publish(Topic topic, const char* payload) = 0;
    virtual ~IBus() = default;
};

#endif // APP_INTERFACES_IBUS_H
```

**Note:** `Topic` is declared outside `IBus` class for convenience — callers write `Topic::TELEMETRY` without needing an `IBus` pointer. This is intentional and consistent throughout the project.

**No `#include` needed** — `Topic` uses `uint8_t` requiring `<cstdint>`, but since IBus is always included alongside other headers that pull `<cstdint>`, add it explicitly to be safe: `#include <cstdint>` at top.

### Exact `App/Interfaces/ISensor.h` content

```cpp
#ifndef APP_INTERFACES_ISENSOR_H
#define APP_INTERFACES_ISENSOR_H

#include <cstdint>

class ISensor {
public:
    virtual uint8_t     id()   const = 0;
    virtual const char* name() const = 0;
    virtual float       read()       = 0;
    virtual ~ISensor() = default;
};

#endif // APP_INTERFACES_ISENSOR_H
```

### Exact `App/Interfaces/IActuator.h` content

```cpp
#ifndef APP_INTERFACES_IACTUATOR_H
#define APP_INTERFACES_IACTUATOR_H

#include <cstdint>

class IActuator {
public:
    virtual uint8_t     id()   const = 0;
    virtual const char* name() const = 0;
    virtual void command(const char* cmd) = 0;
    virtual ~IActuator() = default;
};

#endif // APP_INTERFACES_IACTUATOR_H
```

### Exact `App/Interfaces/IActuatorManager.h` content

```cpp
#ifndef APP_INTERFACES_IACTUATORMANAGER_H
#define APP_INTERFACES_IACTUATORMANAGER_H

#include <cstdint>

class IActuatorManager {
public:
    virtual void commandById(uint8_t id, const char* cmd) = 0;
    virtual ~IActuatorManager() = default;
};

#endif // APP_INTERFACES_IACTUATORMANAGER_H
```

### What already exists from Story 1.1

Story 1.1 created stub headers in `App/Tasks/`:
- `App/Tasks/OdoControl.h` — stub with `OdoSnapshot` struct (x, y, angle, speedLeft, speedRight, uint32_t timestamp)
- `App/Tasks/SensorManager.h` — stub with `SensorSnapshot` struct
- `App/Tasks/ExternalComm.h` — stub with `CommSnapshot` struct

These stubs do NOT yet use Config.h or IBus. They remain valid as-is for now. **Do NOT modify them in this story** — their full implementations come in Stories 1.3–1.5.

Story 1.1 also created `App/cppMain.cpp` (entry point calling `SystemInit::boot()`) and `App/SystemInit/SystemInit.h/.cpp`. These are untouched in this story.

### Critical constraints from architecture

**No `new`/`delete`/`malloc`/`free` anywhere in App/** — pure header files in this story have zero risk, but be aware this constraint applies globally (NFR-02).

**NFR-07**: Only `App/` is modified. `Core/`, `Drivers/`, `Middlewares/` are untouched.

**IBus is publish-only** — no `subscribe()` method exists. `Monitoring` reads data via pull model (static snapshots), not via IBus subscription. Do not add a subscribe interface.

**`Topic` enum mapping to ASCII prefixes** (for future BusFormat story — dev should be aware):
- `TELEMETRY` → `TEL`
- `ALERT` → `ALT`
- `LOG` → `LOG`
- `HEALTH` → `HLT`

This mapping is implemented in `ExternalComm` (Story 1.4), not in IBus.

### FreeRTOS header in Config.h — IDE include paths

STM32CubeIDE project already has these include paths from CubeMX generation (no action needed):
- `Middlewares/Third_Party/FreeRTOS/Source/include`
- `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2`
- `Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F`

So `#include "FreeRTOS.h"` in Config.h will resolve correctly in the STM32CubeIDE build context.

**Important:** If `App/Config.h` is later included in a Google Test (host) build context (Story 1.6), the host build's `CMakeLists.txt` will need a mock `FreeRTOS.h` or a conditional exclude. Add a note in Story 1.6 to handle this. For now, Config.h only compiles in the ARM target context.

### Project Structure Notes

- `App/Config.h` is at the `App/` root (not in a subfolder) — this is intentional per architecture
- `App/Interfaces/` folder is created by creating the first `.h` file inside it
- No `.cpp` files needed — all interfaces are pure abstract, all Config values are `constexpr` (header-only)
- File names: `IBus.h`, `ISensor.h`, `IActuator.h`, `IActuatorManager.h` (PascalCase, identical to class name)
- Guard format: `APP_INTERFACES_IBUS_H`, `APP_INTERFACES_ISENSOR_H`, `APP_INTERFACES_IACTUATOR_H`, `APP_INTERFACES_IACTUATORMANAGER_H`, `APP_CONFIG_H`

### References

- Story requirements: [epics.md — Story 1.2](../_bmad-output/planning-artifacts/epics.md)
- Config.h constants: [architecture.md — System Configuration](../_bmad-output/planning-artifacts/architecture.md#system-configuration)
- Interface structure: [architecture.md — Complete Project Directory Structure](../_bmad-output/planning-artifacts/architecture.md#complete-project-directory-structure)
- Naming conventions: [architecture.md — Naming Patterns](../_bmad-output/planning-artifacts/architecture.md#naming-patterns)
- FreeRTOS priorities: [architecture.md — FreeRTOS Configuration](../_bmad-output/planning-artifacts/architecture.md#freertos-configuration)
- IBus topic/prefix mapping: [architecture.md — Communication Protocol](../_bmad-output/planning-artifacts/architecture.md#communication-protocol)
- Story 1.1 dev notes: [1-1-projet-stm32cubemx-configure-et-freertos-operationnel-sur-cible.md](./1-1-projet-stm32cubemx-configure-et-freertos-operationnel-sur-cible.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

### Completion Notes List

**Story 1.2 — Implémentation terminée le 2026-05-10**

Tous les fichiers créés conformément aux spécifications de la story. Story purement header (aucun `.cpp`).

**Config.h :** `namespace Config` avec 19 constantes `constexpr` — ODO_FREQ_HZ, MAX_SENSORS/ACTUATORS, MONITORING_STALE_MS, 3 PID defaults, 6 stack sizes, 6 priorités FreeRTOS. Types explicites : `uint32_t`, `uint8_t`, `uint16_t`, `UBaseType_t`, `float`. Include `FreeRTOS.h` pour `UBaseType_t`.

**Interfaces :** 4 classes purement abstraites créées dans `App/Interfaces/` :
- `IBus` avec `enum class Topic { TELEMETRY, ALERT, LOG, HEALTH }` déclaré à l'extérieur de la classe (accès `Topic::TELEMETRY` sans pointeur IBus)
- `ISensor` : `id()`, `name()`, `read()`
- `IActuator` : `id()`, `name()`, `command()`
- `IActuatorManager` : `commandById(uint8_t id, const char* cmd)`

**Stubs Story 1.1 :** Aucune modification — OdoControl.h, SensorManager.h, ExternalComm.h n'ont aucun littéral numérique, conformes AC#1.

**NFR compliance :** Tous les guards `APP_<DOSSIER>_<FICHIER>_H` ✅ — Aucun caractère français dans App/ ✅ — Classes PascalCase, interfaces I+PascalCase ✅ — Noms de constantes ALL_CAPS (le grep AC#9 `constexpr.*[a-z][a-z]` retourne des faux positifs sur les types `uint32_t`/`constexpr` eux-mêmes — les noms des constantes sont bien ALL_CAPS, vérifiés visuellement).

**Note Story 1.6 :** `Config.h` inclut `FreeRTOS.h` pour `UBaseType_t`. Le build Google Test (host) nécessitera un mock `FreeRTOS.h` ou un `#ifdef TESTING` conditionnel dans `CMakeLists.txt` — à traiter en Story 1.6.

### File List

- `robot-cdr/App/Config.h` — NEW (namespace Config, 19 constantes constexpr)
- `robot-cdr/App/Interfaces/IBus.h` — NEW (enum class Topic, classe IBus pure abstraite)
- `robot-cdr/App/Interfaces/ISensor.h` — NEW (ISensor pure abstraite)
- `robot-cdr/App/Interfaces/IActuator.h` — NEW (IActuator pure abstraite)
- `robot-cdr/App/Interfaces/IActuatorManager.h` — NEW (IActuatorManager pure abstraite)
