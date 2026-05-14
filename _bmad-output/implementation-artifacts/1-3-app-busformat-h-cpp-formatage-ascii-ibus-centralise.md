# Story 1.3: App/BusFormat.h/.cpp — Formatage ASCII IBus centralisé

Status: review

## Story

As a developer,
I want all IBus message formatting centralized in `App/BusFormat`,
so that every class can publish ASCII messages without writing inline `snprintf` and the protocol can be changed in a single place.

## Acceptance Criteria

1. **Given** `BusFormat.h/.cpp` are created with static factory methods
   **When** a class publishes odometric telemetry
   **Then** it calls `BusFormat::telOdo(float x, float y, float angle)` which returns a `const char*` in the format `TEL ODO %.2f %.2f %.2f\n`

2. **Given** BusFormat is available
   **When** `BusFormat::altProximity(float dist)`, `BusFormat::logInfo(const char* msg)`, `BusFormat::hltTemp(float t)` are called
   **Then** each returns a correctly prefixed ASCII string (`ALT`, `LOG`, `HLT`) terminated by `\n`

3. **Given** the `App/` directory is scanned
   **When** `grep -r "snprintf" App/` is run excluding `BusFormat.cpp`
   **Then** zero occurrences found

4. **And** all BusFormat methods use internal fixed buffers (static or stack-local) — zero dynamic allocation

## Tasks / Subtasks

- [x] Task 1: Create `App/BusFormat.h` (AC: #1, #2, #4)
  - [x] Declare class `BusFormat` with all static factory methods
  - [x] Declare: `static const char* telOdo(float x, float y, float angle)`
  - [x] Declare: `static const char* altProximity(float dist)`
  - [x] Declare: `static const char* logInfo(const char* msg)`
  - [x] Declare: `static const char* hltTemp(float t)`
  - [x] Use guard `#ifndef APP_BUSFORMAT_H`
  - [x] Include only `<cstdio>` in .cpp (header needs no includes — all types are primitives)

- [x] Task 2: Create `App/BusFormat.cpp` (AC: #1, #2, #3, #4)
  - [x] Implement each method using a `static char buf[64]` per method (thread-unsafe but acceptable — these are called from single tasks; see Dev Notes)
  - [x] `telOdo`: `snprintf(buf, sizeof(buf), "TEL ODO %.2f %.2f %.2f\n", x, y, angle)`
  - [x] `altProximity`: `snprintf(buf, sizeof(buf), "ALT PROXIMITY %.2f\n", dist)`
  - [x] `logInfo`: `snprintf(buf, sizeof(buf), "LOG INFO %s\n", msg)`
  - [x] `hltTemp`: `snprintf(buf, sizeof(buf), "HLT TEMP %.2f\n", t)`
  - [x] All buffers sized at 64 chars — sufficient for all formats

- [x] Task 3: Verify NFR compliance (AC: #3, #4)
  - [x] Run `grep -r "snprintf" App/ --include="*.cpp" --include="*.h" | grep -v BusFormat` → zero results ✅
  - [x] Run `grep -rn "new\|malloc\|delete\|free" App/BusFormat.cpp` → zero results ✅
  - [x] Run `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/BusFormat.h App/BusFormat.cpp` → zero results ✅
  - [x] Confirm guard `#ifndef APP_BUSFORMAT_H` present (NFR-06) ✅

## Dev Notes

### What this story creates — explicit scope

This story creates exactly **two files**:

```
App/
├── BusFormat.h    ← NEW this story
└── BusFormat.cpp  ← NEW this story
```

**Do NOT create:** any test files (Story 1.6), any modifications to `ExternalComm.h`, `OdoControl.h`, `SensorManager.h`, `Config.h`, or any `Interfaces/` header. Those are untouched.

### Exact `App/BusFormat.h` content

```cpp
#ifndef APP_BUSFORMAT_H
#define APP_BUSFORMAT_H

class BusFormat {
public:
    static const char* telOdo(float x, float y, float angle);
    static const char* altProximity(float dist);
    static const char* logInfo(const char* msg);
    static const char* hltTemp(float t);
};

#endif // APP_BUSFORMAT_H
```

**No includes needed in the header** — all types used are primitives (`float`, `const char*`). `<cstdio>` belongs in the `.cpp` only.

### Exact `App/BusFormat.cpp` content

```cpp
#include "BusFormat.h"
#include <cstdio>

const char* BusFormat::telOdo(float x, float y, float angle) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "TEL ODO %.2f %.2f %.2f\n", x, y, angle);
    return buf;
}

const char* BusFormat::altProximity(float dist) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "ALT PROXIMITY %.2f\n", dist);
    return buf;
}

const char* BusFormat::logInfo(const char* msg) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "LOG INFO %s\n", msg);
    return buf;
}

const char* BusFormat::hltTemp(float t) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "HLT TEMP %.2f\n", t);
    return buf;
}
```

### Static buffer design — intentional thread-unsafe tradeoff

Each method uses its own `static char buf[64]`. This means:
- **Caller must consume the returned pointer before any other BusFormat call** — the pointer is invalidated on the next call to the same method.
- This is acceptable because `IBus::publish(topic, payload)` passes the string to `ExternalComm` which transmits it immediately (no deferred use).
- Only one task at a time calls each BusFormat method (OdoControl for telOdo, SensorManager for altProximity/hltTemp, etc.) — no concurrent access risk per method.
- **Alternative considered and rejected:** stack-local buffers would require passing a caller-owned buffer — adds complexity without benefit at this stage.
- **Alternative considered and rejected:** dynamic allocation — banned by NFR-02.

**Important:** If future stories require calling `BusFormat::logInfo` from multiple tasks concurrently, the per-method static buffer approach is safe as long as each method has its own dedicated buffer (which it does). Two different methods can be called concurrently safely — only concurrent calls to the **same** method would race.

### ASCII Protocol — Format Reference

| Method | Format | Example output |
|--------|--------|----------------|
| `telOdo(1.23f, 0.45f, 90.0f)` | `TEL ODO %.2f %.2f %.2f\n` | `TEL ODO 1.23 0.45 90.00\n` |
| `altProximity(0.12f)` | `ALT PROXIMITY %.2f\n` | `ALT PROXIMITY 0.12\n` |
| `logInfo("SystemInit OK")` | `LOG INFO %s\n` | `LOG INFO SystemInit OK\n` |
| `hltTemp(36.5f)` | `HLT TEMP %.2f\n` | `HLT TEMP 36.50\n` |

All messages terminate with `\n` — mandatory for ExternalComm's line-based parsing.

### What already exists from Stories 1.1 and 1.2

Stories 1.1 and 1.2 created:
- `App/Tasks/OdoControl.h` — stub with `OdoSnapshot` (x, y, angle, speedLeft, speedRight, timestamp)
- `App/Tasks/SensorManager.h` — stub with `SensorSnapshot`
- `App/Tasks/ExternalComm.h` — stub with `CommSnapshot` (rxCount, txCount, lastCmd[32], timestamp)
- `App/SystemInit/SystemInit.h/.cpp` — boot(), blink task, snapshot zero-init
- `App/cppMain.cpp` — cppMain extern "C", calls SystemInit::boot()
- `App/Config.h` — 19 constexpr constants in `namespace Config`
- `App/Interfaces/IBus.h` — `enum class Topic { TELEMETRY, ALERT, LOG, HEALTH }` + `IBus` pure abstract
- `App/Interfaces/ISensor.h`, `IActuator.h`, `IActuatorManager.h` — pure abstract interfaces

**Do NOT modify any of these files.** BusFormat has zero dependencies on them — it only uses `<cstdio>`.

### Constraints from architecture

- **NFR-02**: No dynamic allocation — `static char buf[]` per method, not heap.
- **NFR-06**: Guard `#ifndef APP_BUSFORMAT_H`, file named `BusFormat.h/.cpp` (PascalCase = class name).
- **NFR-07**: Only `App/` is modified. `Core/`, `Drivers/`, `Middlewares/` are untouched.
- **NFR-08**: All identifiers in English. No French words.
- **FR-09**: `BusFormat` is the **exclusive** formatting gateway — `snprintf` is banned everywhere except `BusFormat.cpp`.

### Usage pattern (for future stories)

```cpp
// ✅ Correct pattern — always used via BusFormat::
_bus->publish(Topic::TELEMETRY, BusFormat::telOdo(_x, _y, _angle));
_bus->publish(Topic::ALERT,     BusFormat::altProximity(dist));
_bus->publish(Topic::LOG,       BusFormat::logInfo("boot complete"));
_bus->publish(Topic::HEALTH,    BusFormat::hltTemp(temp));

// ❌ Banned — snprintf inline
char buf[64];
snprintf(buf, sizeof(buf), "TEL ODO %.2f\n", x);
_bus->publish(Topic::TELEMETRY, buf);
```

### Story 1.6 note — Google Test build

`BusFormat.cpp` uses `<cstdio>` (standard C). No FreeRTOS or HAL dependency. BusFormat compiles on host x86 without any mock — it will be the easiest class to test. `Tests/Unit/BusFormatTest.cpp` (Story 1.6) can include `BusFormat.h` directly and call methods as-is.

### References

- Story requirements: [epics.md — Story 1.3](_bmad-output/planning-artifacts/epics.md)
- IBus message format: [architecture.md — Communication Protocol](_bmad-output/planning-artifacts/architecture.md#communication-protocol)
- IBus formatting rule (FR-09): [epics.md — FR-09](_bmad-output/planning-artifacts/epics.md)
- Naming conventions: [architecture.md — Naming Patterns](_bmad-output/planning-artifacts/architecture.md#naming-patterns)
- No dynamic allocation: [architecture.md — NFR-02](_bmad-output/planning-artifacts/architecture.md)
- Previous story 1.2 context: [1-2-app-config-h-et-app-interfaces-constantes-et-contrats-domaine.md](_bmad-output/implementation-artifacts/1-2-app-config-h-et-app-interfaces-constantes-et-contrats-domaine.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

### Completion Notes List

**Story 1.3 — Implémentation terminée le 2026-05-10**

Deux fichiers créés conformément aux spécifications exactes de la story :

**`App/BusFormat.h` :** Classe `BusFormat` avec 4 méthodes factory statiques — `telOdo`, `altProximity`, `logInfo`, `hltTemp`. Aucun include dans le header (tous les types sont des primitifs C++). Guard `#ifndef APP_BUSFORMAT_H` conforme NFR-06.

**`App/BusFormat.cpp` :** Implémentation via `snprintf` + buffers `static char buf[64]` dédiés par méthode. Include `<cstdio>` uniquement. Zéro allocation dynamique (NFR-02).

**Vérifications NFR passées :**
- Zéro `snprintf` hors `BusFormat.cpp` dans `App/` ✅
- Zéro allocation dynamique dans `BusFormat.cpp` ✅
- Zéro caractère français dans les fichiers ✅
- Guard `#ifndef APP_BUSFORMAT_H` conforme ✅

**Note Story 1.6 :** `BusFormat` n'a aucune dépendance FreeRTOS/HAL — `<cstdio>` uniquement. Les tests `BusFormatTest.cpp` pourront inclure `BusFormat.h` directement sur host x86 sans aucun mock.

### File List

- `robot-cdr/App/BusFormat.h` — NEW (classe BusFormat, 4 méthodes factory statiques)
- `robot-cdr/App/BusFormat.cpp` — NEW (implémentation snprintf, buffers static par méthode)
