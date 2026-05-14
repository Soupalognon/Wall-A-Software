# Story 1.4: ExternalComm — Implémentation IBus et communication tri-canal ASCII

Status: review

## Story

As a developer,
I want `ExternalComm` to implement `IBus` and manage UART/USB/Ethernet communication with the ASCII protocol,
so that all modules can publish without knowing the transport layer and the robot can receive commands from the PC.

## Acceptance Criteria

1. **Given** `ExternalComm` implements `IBus::publish(Topic, const char*)`
   **When** a module calls `bus_->publish(Topic::TELEMETRY, BusFormat::telOdo(...))`
   **Then** `ExternalComm::txTask` transmits the message to all active output channels (USB, ETH)

2. **Given** `ExternalComm::rxTask` is running and UART receives `CMD MOVE 0.5 0.3\n`
   **When** the message is parsed with `sscanf` on the first token
   **Then** the motion setpoint is written to the motion mailbox via `xQueueOverwrite`

3. **Given** UART and USB both receive a `CMD MOVE` simultaneously
   **When** `rxTask` processes them
   **Then** the UART command takes priority and overwrites the USB command

4. **Given** `Tests/Unit/ExternalCommTest.cpp` exists with `MockBus` and mock UART/USB
   **When** tests run on PC host
   **Then** all CMD variants (MOVE, ACTUATOR) are correctly dispatched and invalid inputs are handled — tests pass without STM32 hardware

5. **And** `ExternalComm` is constructed with injected dependencies — it instantiates no dependency internally

6. **And** `ExternalComm::CommSnapshot` (static public struct with `rxCount`, `txCount`, `lastCmd[32]`, `timestamp`) is updated by `rxTask` on each received command — readable by `Monitoring` without explicit synchronization (ARM Cortex-M atomic write)

## Tasks / Subtasks

- [x] Task 1: Create `App/Interfaces/ICommChannel.h` (AC: #4, #5)
  - [x] Declare pure abstract `ICommChannel` with `transmit(const char*, uint16_t)` and `receive(char*, uint16_t, uint32_t timeoutMs) → uint16_t`
  - [x] Guard: `#ifndef APP_INTERFACES_ICOMMCHANNEL_H`
  - [x] Include `<cstdint>`

- [x] Task 2: Replace stub `App/Tasks/ExternalComm.h` with full class definition (AC: #1, #2, #3, #5, #6)
  - [x] Inherit from `IBus` — `class ExternalComm : public IBus`
  - [x] Constructor: `ExternalComm(ICommChannel* uart, ICommChannel* usb, ICommChannel* eth, IActuatorManager* actuatorMgr, QueueHandle_t motionMailbox)`
  - [x] Declare `void publish(Topic topic, const char* payload) override`
  - [x] Declare `static void rxTask(void* arg)` and `static void txTask(void* arg)`
  - [x] Declare `static CommSnapshot latestSnapshot`
  - [x] Declare `BUS_CONFIG[]` static constexpr array (see Dev Notes for exact content)
  - [x] Declare per-topic TX queues and private members (see Dev Notes)
  - [x] Guard: `#ifndef APP_TASKS_EXTERNALCOMM_H`
  - [x] `#include "Interfaces/IBus.h"`, `"Interfaces/IActuatorManager.h"`, `"Interfaces/ICommChannel.h"`, `<FreeRTOS.h>`, `<queue.h>`, `<cstdint>`, `<cstring>`

- [x] Task 3: Create `App/Tasks/ExternalComm.cpp` (AC: #1, #2, #3, #5, #6)
  - [x] Define `ExternalComm::CommSnapshot ExternalComm::latestSnapshot{}`
  - [x] Implement constructor: store injected pointers, create 4 TX queues via `xQueueCreate`
  - [x] Implement `publish()`: look up topic policy in `BUS_CONFIG`, call `xQueueOverwrite` (OVERWRITE) or `xQueueSend(timeout=0)` (DROP_SILENT)
  - [x] Implement `txTask()`: poll 4 queues in priority order ALT→HLT→TEL→LOG, call `_transmitAll()` for each dequeued message
  - [x] Implement `rxTask()`: poll `_uart` and `_usb` channels for complete lines, call `_processRxLine()`, apply UART priority
  - [x] Implement `_processRxLine()`: sscanf on first token, dispatch CMD MOVE to motionMailbox, CMD ACTUATOR to actuatorMgr, update CommSnapshot
  - [x] Implement `_transmitAll()`: call `_uart->transmit()`, `_usb->transmit()`, `_eth->transmit()` — each guarded by null check

- [x] Task 4: Create `Tests/Unit/ExternalCommTest.cpp` (AC: #4)
  - [x] Test `publish(Topic::TELEMETRY, ...)` enqueues and txTask transmits to mock channels
  - [x] Test `CMD MOVE 0.5 0.3\n` → xQueueOverwrite called on motion mailbox with correct values
  - [x] Test `CMD ACTUATOR PUMP_1 ON\n` → commandById(id, "ON") called on mock actuator manager
  - [x] Test invalid CMD → no crash, LOG published via publish()
  - [x] Test UART priority: UART CMD overwrites USB CMD in motion mailbox
  - [x] NOTE: Requires Story 1.6 infrastructure (CMakeLists.txt, MockBus.h) to compile and run

- [x] Task 5: Verify NFR compliance (AC: #1–#6)
  - [x] `grep -r "new\|delete\|malloc\|free" App/Tasks/ExternalComm.cpp` → zero results
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/Tasks/ExternalComm.h App/Tasks/ExternalComm.cpp` → zero results
  - [x] Confirm all private members prefixed `_`
  - [x] Confirm guard `#ifndef APP_TASKS_EXTERNALCOMM_H` (NFR-06)
  - [x] `grep -r "snprintf" App/Tasks/ExternalComm.cpp` → zero results (BusFormat is used by callers, not here)

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Interfaces/
│   └── ICommChannel.h          ← NEW (pure abstract comm channel HAL)
└── Tasks/
    ├── ExternalComm.h          ← REPLACE stub (full class, inherits IBus)
    └── ExternalComm.cpp        ← NEW (implementation)
Tests/
└── Unit/
    └── ExternalCommTest.cpp    ← NEW (pending Story 1.6 infra to run)
```

**Do NOT touch:** `App/Config.h`, `App/BusFormat.h/.cpp`, `App/Interfaces/IBus.h`, `App/SystemInit/`, `App/Tasks/OdoControl.h`, `App/Tasks/SensorManager.h`. These are created by Stories 1.1–1.3.

### Exact `App/Interfaces/ICommChannel.h` Content

```cpp
#ifndef APP_INTERFACES_ICOMMCHANNEL_H
#define APP_INTERFACES_ICOMMCHANNEL_H

#include <cstdint>

class ICommChannel {
public:
    virtual void     transmit(const char* data, uint16_t len) = 0;
    virtual uint16_t receive(char* buf, uint16_t maxLen, uint32_t timeoutMs) = 0;
    virtual ~ICommChannel() = default;
};

#endif // APP_INTERFACES_ICOMMCHANNEL_H
```

`timeoutMs = 0` means non-blocking (returns immediately with 0 bytes if nothing available). `transmit` is fire-and-forget — no return value, caller must not block on confirmation at 200Hz-path.

### Exact `App/Tasks/ExternalComm.h` Content (REPLACES STUB)

```cpp
#ifndef APP_TASKS_EXTERNALCOMM_H
#define APP_TASKS_EXTERNALCOMM_H

#include "Interfaces/IBus.h"
#include "Interfaces/IActuatorManager.h"
#include "Interfaces/ICommChannel.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <cstdint>
#include <cstring>

class ExternalComm : public IBus {
public:
    struct CommSnapshot {
        uint32_t rxCount{};
        uint32_t txCount{};
        char     lastCmd[32]{};
        uint32_t timestamp{};
    };
    static CommSnapshot latestSnapshot;

    enum class QueuePolicy : uint8_t { OVERWRITE, DROP_SILENT };
    struct BusConfig {
        Topic       topic;
        QueuePolicy policy;
    };
    static constexpr BusConfig BUS_CONFIG[] = {
        { Topic::TELEMETRY, QueuePolicy::OVERWRITE   },
        { Topic::ALERT,     QueuePolicy::OVERWRITE   },
        { Topic::HEALTH,    QueuePolicy::DROP_SILENT },
        { Topic::LOG,       QueuePolicy::DROP_SILENT },
    };

    ExternalComm(ICommChannel* uart,
                 ICommChannel* usb,
                 ICommChannel* eth,
                 IActuatorManager* actuatorMgr,
                 QueueHandle_t motionMailbox);

    void publish(Topic topic, const char* payload) override;

    static void rxTask(void* arg);
    static void txTask(void* arg);

private:
    struct TxEntry { char buf[80]; };

    ICommChannel*    _uart;
    ICommChannel*    _usb;
    ICommChannel*    _eth;
    IActuatorManager* _actuatorMgr;
    QueueHandle_t    _motionMailbox;

    // One queue per topic — depth 1 for OVERWRITE, depth 4/2 for DROP_SILENT
    QueueHandle_t _telQueue;   // depth 1, TELEMETRY
    QueueHandle_t _altQueue;   // depth 1, ALERT
    QueueHandle_t _hltQueue;   // depth 2, HEALTH
    QueueHandle_t _logQueue;   // depth 4, LOG

    QueueHandle_t _queueForTopic(Topic t) const;
    void _processRxLine(const char* line, bool uartSource);
    void _transmitAll(const char* msg, uint16_t len);
};

#endif // APP_TASKS_EXTERNALCOMM_H
```

**Why one queue per topic:** `xQueueOverwrite` only works on depth-1 queues. Having dedicated queues per topic is the only way to implement true overwrite (latest telemetry always replaces stale reading) without a custom ring buffer. This also gives txTask clear priority ordering without complex logic.

### `ExternalComm.cpp` — Key Implementation Details

**Static member definition (at file top, outside any function):**
```cpp
ExternalComm::CommSnapshot ExternalComm::latestSnapshot{};
```

**Constructor — queue creation (no `new`, xQueueCreate uses FreeRTOS heap internally — acceptable as it happens once in SystemInit before tasks start):**
```cpp
ExternalComm::ExternalComm(ICommChannel* uart, ICommChannel* usb, ICommChannel* eth,
                           IActuatorManager* actuatorMgr, QueueHandle_t motionMailbox)
    : _uart(uart), _usb(usb), _eth(eth),
      _actuatorMgr(actuatorMgr), _motionMailbox(motionMailbox)
{
    _telQueue = xQueueCreate(1, sizeof(TxEntry));
    _altQueue = xQueueCreate(1, sizeof(TxEntry));
    _hltQueue = xQueueCreate(2, sizeof(TxEntry));
    _logQueue = xQueueCreate(4, sizeof(TxEntry));
}
```

**`publish()` — called from any task, must be non-blocking:**
```cpp
void ExternalComm::publish(Topic topic, const char* payload) {
    TxEntry entry;
    strncpy(entry.buf, payload, sizeof(entry.buf) - 1);
    entry.buf[sizeof(entry.buf) - 1] = '\0';

    QueueHandle_t q = _queueForTopic(topic);
    if (q == nullptr) return;

    // Look up policy
    for (const auto& cfg : BUS_CONFIG) {
        if (cfg.topic == topic) {
            if (cfg.policy == QueuePolicy::OVERWRITE)
                xQueueOverwrite(q, &entry);       // always succeeds for depth-1 queue
            else
                xQueueSend(q, &entry, 0);         // drop-silent: timeout=0
            return;
        }
    }
}
```

**`txTask()` — polls queues in priority order, transmits messages:**
```cpp
void ExternalComm::txTask(void* arg) {
    ExternalComm* self = static_cast<ExternalComm*>(arg);
    TxEntry entry;
    for (;;) {
        bool sent = false;
        // Priority order: ALERT > HEALTH > TELEMETRY > LOG
        QueueHandle_t order[] = { self->_altQueue, self->_hltQueue,
                                  self->_telQueue, self->_logQueue };
        for (auto q : order) {
            if (xQueueReceive(q, &entry, 0) == pdTRUE) {
                uint16_t len = static_cast<uint16_t>(strlen(entry.buf));
                self->_transmitAll(entry.buf, len);
                sent = true;
                break; // one message per loop iteration
            }
        }
        if (!sent) vTaskDelay(pdMS_TO_TICKS(1)); // yield when idle
    }
}
```

**`_transmitAll()` — send to all channels, null-check each:**
```cpp
void ExternalComm::_transmitAll(const char* msg, uint16_t len) {
    if (_usb) _usb->transmit(msg, len);
    if (_eth) _eth->transmit(msg, len);
    // UART is RX-priority for CMD — not used for TX output (PC monitors via USB/ETH)
    // If UART TX is needed in future, add here
}
```

**`rxTask()` — polls UART and USB, applies UART priority for CMD:**
```cpp
void ExternalComm::rxTask(void* arg) {
    ExternalComm* self = static_cast<ExternalComm*>(arg);
    char uartBuf[80]{};
    char usbBuf[80]{};
    for (;;) {
        uint16_t uartLen = 0, usbLen = 0;
        if (self->_uart) uartLen = self->_uart->receive(uartBuf, sizeof(uartBuf), 0);
        if (self->_usb)  usbLen  = self->_usb->receive(usbBuf,  sizeof(usbBuf),  0);

        if (uartLen > 0) self->_processRxLine(uartBuf, true);   // UART = priority
        if (usbLen  > 0) self->_processRxLine(usbBuf,  false);  // USB = non-priority
        
        vTaskDelay(pdMS_TO_TICKS(1)); // 1ms polling — CMD latency acceptable
    }
}
```

**`_processRxLine()` — parse and dispatch:**
```cpp
void ExternalComm::_processRxLine(const char* line, bool uartSource) {
    char cmdToken[16]{};
    char verb[16]{};

    if (sscanf(line, "%15s %15s", cmdToken, verb) < 2) return;
    if (strncmp(cmdToken, "CMD", 3) != 0) return;

    if (strncmp(verb, "MOVE", 4) == 0) {
        float left = 0.0f, right = 0.0f;
        sscanf(line, "%*s %*s %f %f", &left, &right);
        struct { float left; float right; } sp = { left, right };
        xQueueOverwrite(_motionMailbox, &sp); // UART and USB both overwrite — UART wins by processing order
    } else if (strncmp(verb, "ACTUATOR", 8) == 0) {
        char actorId[16]{}, actCmd[32]{};
        sscanf(line, "%*s %*s %15s %31s", actorId, actCmd);
        // Map string ID to uint8_t — convention: numeric suffix or lookup
        uint8_t id = static_cast<uint8_t>(actorId[0]); // simplified — real impl uses lookup
        if (_actuatorMgr) _actuatorMgr->commandById(id, actCmd);
    }

    // Update CommSnapshot (ARM Cortex-M word writes are atomic — no critical section needed on writer side)
    latestSnapshot.rxCount++;
    strncpy(latestSnapshot.lastCmd, line, sizeof(latestSnapshot.lastCmd) - 1);
    latestSnapshot.lastCmd[sizeof(latestSnapshot.lastCmd) - 1] = '\0';
    latestSnapshot.timestamp = HAL_GetTick();
}
```

**UART priority explained:** `rxTask` processes `uartBuf` BEFORE `usbBuf` in the same loop iteration. Both call `xQueueOverwrite(_motionMailbox)`. Since UART is processed first, if USB also has a CMD MOVE, it overwrites — but in practice simultaneous arrival within the same 1ms poll is extremely unlikely, and the requirement is about UART winning over USB, which is achieved by processing UART first (its `xQueueOverwrite` runs first, USB's runs second and would overwrite it — **INVERSION**). 

**Correct UART priority implementation:** Process USB first, then UART (UART's xQueueOverwrite runs last and wins):

```cpp
if (usbLen  > 0) self->_processRxLine(usbBuf,  false);  // USB first
if (uartLen > 0) self->_processRxLine(uartBuf, true);   // UART last = wins
```

**Critical:** Do NOT use the naive order shown in the draft rxTask above. UART must be processed LAST so its `xQueueOverwrite` is the final write and wins.

### ACTUATOR ID Mapping — Design Decision

The AC says `CMD ACTUATOR PUMP_1 ON\n` maps to `commandById(id, "ON")`. The story doesn't prescribe how `PUMP_1` maps to a `uint8_t id`. Two options:
1. **String format convention:** `PUMP_1` → extract numeric suffix `1` → id = 1
2. **Lookup table in SystemInit:** SystemInit registers name→id mappings

**Use option 1 (simpler):** Extract last char(s) as decimal id: `id = atoi(&actorId[strlen(actorId) - 1])` or scan for last `_` and parse digits after it.

```cpp
// Extract id from "PUMP_1" → 1, "SERVO_3" → 3
const char* underscore = strrchr(actorId, '_');
uint8_t id = underscore ? static_cast<uint8_t>(atoi(underscore + 1)) : 0;
```

Include `<cstdlib>` in ExternalComm.cpp for `atoi`/`strrchr`.

### CommSnapshot — Thread Safety

`CommSnapshot latestSnapshot` is a static struct written by `rxTask` and read by `Monitoring` (lower priority). Per the architecture:

- **Writer (`rxTask`, higher priority):** No critical section needed — preemption by a lower-priority task is impossible on ARM Cortex-M for the writer
- **Reader (`Monitoring`, lower priority):** MUST use `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` when copying the full struct — see architecture decision

**Do NOT add critical sections in `rxTask` or `ExternalComm` code.** This is Monitoring's responsibility (established in architecture).

`lastCmd[32]` invariant: always null-terminated via `strncpy` + explicit `[sizeof-1] = '\0'`.

### HAL_GetTick() in App/ Code

`latestSnapshot.timestamp = HAL_GetTick()` requires `stm32xxxx_hal.h`. On STM32CubeIDE this resolves via `Core/Inc/main.h` which is auto-included. On host (Google Test), `HAL_GetTick` must be mocked:

```cpp
// In Tests/Mocks/MockHAL.h (created in Story 1.6)
extern uint32_t mockTick;
inline uint32_t HAL_GetTick() { return mockTick; }
```

For ExternalCommTest.cpp, wrap the include:
```cpp
#include "MockHAL.h"  // must come before ExternalComm.cpp in test build
```

### Tests/Unit/ExternalCommTest.cpp — Structure

Test file requires Story 1.6 infrastructure (CMakeLists.txt + MockBus.h). Write the file now, it will compile when Story 1.6 is complete.

```cpp
#include <gtest/gtest.h>
#include "MockBus.h"          // from Story 1.6: Tests/Mocks/MockBus.h
#include "MockHAL.h"          // provides HAL_GetTick mock
#include "Tasks/ExternalComm.h"

// MockCommChannel for testing
class MockCommChannel : public ICommChannel {
public:
    std::string lastTransmitted;
    std::string rxBuffer;
    
    void transmit(const char* data, uint16_t len) override {
        lastTransmitted = std::string(data, len);
    }
    uint16_t receive(char* buf, uint16_t maxLen, uint32_t) override {
        if (rxBuffer.empty()) return 0;
        uint16_t n = std::min((uint16_t)rxBuffer.size(), maxLen);
        memcpy(buf, rxBuffer.c_str(), n);
        rxBuffer.clear();
        return n;
    }
};

// Tests cover:
// - publish(TELEMETRY) → txTask → mock channel transmit
// - CMD MOVE 0.5 0.3 → xQueueOverwrite with {0.5f, 0.3f}
// - CMD ACTUATOR PUMP_1 ON → commandById(1, "ON")
// - invalid CMD → no crash, LOG published
// - UART priority: USB CMD overwritten by UART CMD
```

### What Already Exists — Do Not Recreate

From Stories 1.1–1.3:
- `App/Tasks/ExternalComm.h` — **REPLACE** the stub (stub has CommSnapshot only, no IBus inheritance)
- `App/Config.h` — `Config::PRIO_EXTCOMM_RX = 4`, `Config::PRIO_EXTCOMM_TX = 3`, `Config::STACK_EXTCOMM_RX = 256`, `Config::STACK_EXTCOMM_TX = 256`
- `App/Interfaces/IBus.h` — `enum class Topic { TELEMETRY=0, ALERT=1, LOG=2, HEALTH=3 }` + `IBus` pure abstract with `publish(Topic, const char*)`
- `App/Interfaces/IActuatorManager.h` — `commandById(uint8_t id, const char* cmd)`
- `App/BusFormat.h/.cpp` — all ASCII formatting; `ExternalComm` does NOT call BusFormat internally (callers do)
- `App/SystemInit/SystemInit.h/.cpp` — will be updated in Story 1.5 to wire ExternalComm; do NOT modify SystemInit in this story

**Story 1.5** will update SystemInit to instantiate ExternalComm and call xTaskCreate for rxTask/txTask. **Do NOT add xTaskCreate calls in this story.**

### Constraints Enforcement Checklist

| Constraint | How to verify |
|---|---|
| NFR-02: no new/delete | `grep -r "new\|delete\|malloc\|free" App/Tasks/ExternalComm.cpp` → 0 |
| NFR-06: guard APP_TASKS_EXTERNALCOMM_H | Check header |
| NFR-06: private members prefixed `_` | Visual review of all private members |
| NFR-07: only App/ and Tests/ modified | git status should show nothing in Core/, Drivers/, Middlewares/ |
| NFR-08: English identifiers only | `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/Tasks/ExternalComm.h App/Tasks/ExternalComm.cpp` → 0 |
| FR-09: no snprintf in ExternalComm.cpp | `grep "snprintf" App/Tasks/ExternalComm.cpp` → 0 |
| FR-04: sscanf on first token | Verify `_processRxLine` uses sscanf |
| NFR-04: publish() non-blocking | xQueueOverwrite and xQueueSend(timeout=0) — both non-blocking |

### Architecture Compliance

- `ExternalComm` is the **only** class implementing `IBus` in the entire codebase — all other modules receive `IBus*` pointer (which is `ExternalComm` at runtime)
- `ExternalComm` does NOT know `OdoControl`, `SensorManager`, or `Monitoring` — those modules call `publish()` via their `IBus*` pointer
- `ExternalComm` communicates with `MotionPlanner` indirectly via `_motionMailbox` (FreeRTOS queue handle injected by SystemInit)
- `ExternalComm` communicates with `ActuatorManager` via `IActuatorManager*` interface
- No direct task-to-task method calls — all inter-task communication via FreeRTOS primitives or interfaces

### FreeRTOS API Used in This Story

| Function | Purpose |
|---|---|
| `xQueueCreate(depth, itemSize)` | Create TX queues in constructor |
| `xQueueOverwrite(queue, &item)` | Overwrite-policy publish (TELEMETRY, ALERT) |
| `xQueueSend(queue, &item, 0)` | Drop-silent-policy publish (LOG, HEALTH) |
| `xQueueReceive(queue, &item, 0)` | Non-blocking dequeue in txTask |
| `vTaskDelay(pdMS_TO_TICKS(1))` | Yield when no messages to transmit/receive |

**All FreeRTOS calls here are non-blocking (timeout=0 or vTaskDelay).** `publish()` must never block — it is called from high-priority tasks (OdoControl priority 5).

### Review Findings to Address (from Story 1.1)

Story 1.1 deferred: `lastCmd[32]` without documented null-termination invariant. **This story closes it:** `_processRxLine` must always null-terminate `latestSnapshot.lastCmd` via `strncpy` + explicit null.

Story 1.1 deferred: static member definitions in SystemInit.cpp. **Resolution for ExternalComm:** define `ExternalComm::CommSnapshot ExternalComm::latestSnapshot{}` in `ExternalComm.cpp` (NOT in SystemInit.cpp). Pattern for future stories: each class defines its own static members in its own `.cpp`.

### Project Structure Notes

- `ICommChannel.h` goes in `App/Interfaces/` alongside `IBus.h`, `ISensor.h`, `IActuator.h`, `IActuatorManager.h`
- `ExternalComm.h` and `ExternalComm.cpp` go in `App/Tasks/` (same directory as the existing stub)
- `ExternalCommTest.cpp` goes in `Tests/Unit/` — folder may not exist yet; create it if needed
- Mock for ICommChannel (`MockCommChannel`) is inline in the test file — not a shared mock (unlike `MockBus` which is shared across all test files in Tests/Mocks/)

### References

- Story requirements: [epics.md — Story 1.4](_bmad-output/planning-artifacts/epics.md)
- Architecture ExternalComm: [architecture.md — Data Flow](_bmad-output/planning-artifacts/architecture.md#data-flow)
- IBus interface: [App/Interfaces/IBus.h] (created Story 1.2)
- IActuatorManager interface: [App/Interfaces/IActuatorManager.h] (created Story 1.2)
- Config constants (PRIO/STACK for EXTCOMM): [architecture.md — FreeRTOS Configuration](_bmad-output/planning-artifacts/architecture.md#freertos-configuration)
- Snapshot sync pattern: [architecture.md — Monitoring Pull Model](_bmad-output/planning-artifacts/architecture.md#monitoring---accès-aux-données-pull-model)
- BusFormat formatting: [App/BusFormat.h] (created Story 1.3)
- Previous story dev notes: [1-3-app-busformat-h-cpp-formatage-ascii-ibus-centralise.md](_bmad-output/implementation-artifacts/1-3-app-busformat-h-cpp-formatage-ascii-ibus-centralise.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

Aucun blocage majeur. Note critique : `rxTask` traite USB en premier et UART en dernier afin que `xQueueOverwrite` UART s'exécute en dernier et gagne (priorité UART correcte — inversion par rapport au brouillon naïf dans les Dev Notes).

### Completion Notes List

- Créé `App/Interfaces/ICommChannel.h` — interface pure avec `transmit` et `receive`
- Remplacé le stub `App/Tasks/ExternalComm.h` — héritage `IBus`, 4 queues par topic, `BUS_CONFIG[]`, `CommSnapshot` statique
- Créé `App/Tasks/ExternalComm.cpp` — `publish()` non-bloquant, `txTask()` priorité ALT→HLT→TEL→LOG, `rxTask()` priorité UART correcte (USB traité en premier, UART en dernier), `_processRxLine()` avec sscanf + dispatch MOVE/ACTUATOR, mapping `PUMP_1 → id=1` via `strrchr('_')` + `atoi`
- Créé `Tests/Unit/ExternalCommTest.cpp` — 6 tests couvrant publish, CMD MOVE, CMD ACTUATOR, entrée invalide, priorité UART, CommSnapshot (compilera avec l'infra Story 1.6)
- Tous les contrôles NFR passés : no new/delete, no snprintf, no caractères français, guards corrects, membres privés préfixés `_`

### File List

- `robot-cdr/App/Interfaces/ICommChannel.h` — NOUVEAU
- `robot-cdr/App/Tasks/ExternalComm.h` — REMPLACÉ (stub → classe complète)
- `robot-cdr/App/Tasks/ExternalComm.cpp` — NOUVEAU
- `robot-cdr/Tests/Unit/ExternalCommTest.cpp` — NOUVEAU

## Change Log

- 2026-05-10 : Story 1.4 implémentée — ICommChannel.h créé, ExternalComm.h remplacé (héritage IBus, 4 queues, BUS_CONFIG), ExternalComm.cpp créé (publish/rxTask/txTask/_processRxLine/_transmitAll), ExternalCommTest.cpp créé (6 tests, en attente infra Story 1.6)
