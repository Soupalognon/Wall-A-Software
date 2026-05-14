# Story 1.5: SystemInit::boot() — câblage statique complet

Status: review

## Story

As a developer,
I want `SystemInit::boot()` to instantiate all objects in static memory and inject all dependencies,
so that `main.cpp` contains only 3 lines and the complete system wiring is readable in a single file, without dynamic allocation.

## Acceptance Criteria

1. **Given** `SystemInit::boot()` is implemented
   **When** it executes
   **Then** all objects (ExternalComm, StubActuatorManager, stub SensorManager/MotionPlanner/OdoControl) are instantiated with `static` keyword and injected via constructors

2. **Given** `SystemInit::boot()` is complete
   **When** `grep -r "new\|delete\|malloc\|free" App/` is run
   **Then** zero occurrences found outside comments

3. **Given** `main.cpp` / `cppMain.cpp` calls `SystemInit::boot()` then the scheduler starts
   **When** the firmware starts
   **Then** FreeRTOS tasks are created by SystemInit, the scheduler starts, no crash occurs

4. **And** `SystemInit` is the only file that calls `xTaskCreate` — no other task creates another

## Tasks / Subtasks

- [x] Task 1: Fix CommSnapshot duplicate symbol — remove from SystemInit.cpp (AC: #3)
  - [x] Open `App/SystemInit/SystemInit.cpp`
  - [x] Remove the line `ExternalComm::CommSnapshot ExternalComm::latestSnapshot{};` — Story 1.4 already defines it in `ExternalComm.cpp`
  - [x] Keep `OdoControl::OdoSnapshot OdoControl::latestSnapshot{}` and `SensorManager::SensorSnapshot SensorManager::latestSnapshot{}` in `SystemInit.cpp` (no separate .cpp exists for stubs yet)
  - [x] Verify build: duplicate symbol error on `ExternalComm::latestSnapshot` is eliminated

- [x] Task 2: Create `App/Drivers/UartChannel.h/.cpp` (AC: #1, #3)
  - [x] Header: class `UartChannel : public ICommChannel` with `explicit UartChannel(UART_HandleTypeDef* h)` constructor
  - [x] Guard: `#ifndef APP_DRIVERS_UARTCHANNEL_H`
  - [x] Include `"Interfaces/ICommChannel.h"`, `"main.h"` (provides `UART_HandleTypeDef` via stm32f4xx_hal.h)
  - [x] Implement `transmit()`: `HAL_UART_Transmit(_huart, reinterpret_cast<const uint8_t*>(data), len, 100)` — 100ms timeout
  - [x] Implement `receive()`: non-blocking byte-by-byte loop; call `HAL_UART_Receive(_huart, &b, 1, 0)` in a while loop up to maxLen; break on `'\n'` or HAL_TIMEOUT; return byte count received
  - [x] Private member: `UART_HandleTypeDef* _huart`

- [x] Task 3: Create `App/Drivers/UsbCdcChannel.h/.cpp` (AC: #1, #3)
  - [x] Header: class `UsbCdcChannel : public ICommChannel`
  - [x] Guard: `#ifndef APP_DRIVERS_USBCDCCHANNEL_H`
  - [x] Include `"Interfaces/ICommChannel.h"`, `"usbd_cdc_if.h"` (from CubeMX USB device middleware)
  - [x] Implement `transmit()`: call `CDC_Transmit_FS(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), len)` — no return check (fire-and-forget)
  - [x] Implement `receive()`: return 0 (RX via interrupt ring buffer is out of scope for this story — see Dev Notes)
  - [x] No private members needed (stateless wrapper)

- [x] Task 4: Create `App/Tasks/StubActuatorManager.h` (AC: #1)
  - [x] Declare class `StubActuatorManager : public IActuatorManager`
  - [x] Guard: `#ifndef APP_TASKS_STUBACTUATORMANAGER_H`
  - [x] Include `"Interfaces/IActuatorManager.h"`
  - [x] Implement `commandById(uint8_t id, const char* cmd)`: stub silently ignores (no bus dependency — see Dev Notes recommandation)
  - [x] NOTE: Full ActuatorManager is implemented in Story 3.2 — this stub satisfies the ExternalComm injection requirement now

- [x] Task 5: Create `motionMailbox` queue and implement full `SystemInit::boot()` (AC: #1, #2, #3, #4)
  - [x] Open `App/SystemInit/SystemInit.cpp`
  - [x] Add includes: `"Tasks/ExternalComm.h"`, `"Tasks/StubActuatorManager.h"`, `"Drivers/UartChannel.h"`, `"Drivers/UsbCdcChannel.h"`, `"Config.h"`, `<FreeRTOS.h>`, `<queue.h>`, `<task.h>`
  - [x] Declare `motionSetpoint` struct inline in SystemInit.cpp: `struct MotionSetpoint { float left; float right; }`
  - [x] In `boot()`, create motionMailbox: `static QueueHandle_t motionMailbox = xQueueCreate(1, sizeof(MotionSetpoint));`
  - [x] Instantiate `UartChannel` with `huart1` (declared extern "C" since defined in main.c): `static UartChannel uartCh{&huart1};`
  - [x] Instantiate `UsbCdcChannel`: `static UsbCdcChannel usbCh{};`
  - [x] Instantiate `StubActuatorManager` (no bus dependency)
  - [x] Instantiate `ExternalComm` with channels, actuator manager, mailbox: `static ExternalComm extComm{&uartCh, &usbCh, nullptr, &stubActMgr, motionMailbox};`
  - [x] Create rxTask: `xTaskCreate(ExternalComm::rxTask, "ExtRX", Config::STACK_EXTCOMM_RX, &extComm, Config::PRIO_EXTCOMM_RX, nullptr);`
  - [x] Create txTask: `xTaskCreate(ExternalComm::txTask, "ExtTX", Config::STACK_EXTCOMM_TX, &extComm, Config::PRIO_EXTCOMM_TX, nullptr);`
  - [x] Keep existing blink task (GPIOE LED_GREEN toggle)

- [x] Task 6: Update `App/SystemInit/SystemInit.h` to include new headers (AC: #1)
  - [x] Ensure `SystemInit.h` does NOT include concrete class headers (keep it minimal — forward declare or include only in .cpp)

- [x] Task 7: Verify NFR compliance (AC: #2)
  - [x] `grep -r "new\|delete\|malloc\|free" App/` → zero results
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/SystemInit/ App/Drivers/UartChannel.cpp App/Drivers/UsbCdcChannel.cpp` → zero results
  - [x] Confirm `xTaskCreate` appears ONLY in `App/SystemInit/SystemInit.cpp`
  - [x] Confirm all private members prefixed `_`

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Drivers/
│   ├── UartChannel.h            ← NEW
│   ├── UartChannel.cpp          ← NEW
│   ├── UsbCdcChannel.h          ← NEW
│   └── UsbCdcChannel.cpp        ← NEW
├── Tasks/
│   └── StubActuatorManager.h    ← NEW (header-only, no .cpp needed)
└── SystemInit/
    ├── SystemInit.h             ← UPDATE (add includes if needed)
    └── SystemInit.cpp           ← UPDATE (remove duplicate, add wiring)
```

**Do NOT touch:** `App/Config.h`, `App/BusFormat.h/.cpp`, `App/Interfaces/`, `App/Tasks/ExternalComm.h/.cpp`, `Core/`, `Drivers/`, `Middlewares/`

### CRITICAL: ExternalComm::CommSnapshot Duplicate Symbol

Story 1.4 created `App/Tasks/ExternalComm.cpp` which contains:
```cpp
ExternalComm::CommSnapshot ExternalComm::latestSnapshot{};
```

Story 1.1 created `App/SystemInit/SystemInit.cpp` which also contains this line. **This is a duplicate symbol — the linker will error.** Task 1 must remove it from SystemInit.cpp.

`OdoControl::OdoSnapshot OdoControl::latestSnapshot{}` and `SensorManager::SensorSnapshot SensorManager::latestSnapshot{}` remain in `SystemInit.cpp` because there are no `OdoControl.cpp` or `SensorManager.cpp` files yet (stubs are header-only).

### CMSIS OS v2 Context (from Story 1.1)

The actual boot sequence is:
```
main.c → osKernelInitialize() → osThreadNew(StartDefaultTask) → osKernelStart()
            ↓
     StartDefaultTask() → MX_LWIP_Init() → MX_USB_DEVICE_Init() → cppMain()
                                                                         ↓
                                                             SystemInit::boot()
```

`SystemInit::boot()` is called from `cppMain()` in `App/cppMain.cpp`. The blink task and now rxTask/txTask are created inside `boot()` using `xTaskCreate` — this is compatible with CMSIS OS v2 (the FreeRTOS native API is still available).

### HAL UART Handle Name — Verify Before Use

The project has UART1, UART2, UART6 (from Story 1.1 completion notes). The HAL handles are declared in `Core/Src/usart.c` as `UART_HandleTypeDef huart1;` etc. They are `extern`-declared in `Core/Inc/usart.h`.

**Before using `huart1`:** Check which UART is physically connected as the "terrain command" channel. Verify by opening `Core/Inc/usart.h` or `Core/Src/usart.c` and looking at the handle names. The correct include for the extern declaration is `#include "usart.h"` (not `main.h`).

```cpp
// In SystemInit.cpp
#include "usart.h"   // provides extern UART_HandleTypeDef huart1 (or huartX)
```

If UART for commands is UART1, use `&huart1`. Adapt to actual CubeMX UART assignment.

### StubActuatorManager — Two-Step Init Challenge

`StubActuatorManager` needs an `IBus*` pointer, but `IBus*` is implemented by `ExternalComm`. This creates a chicken-and-egg dependency since both are static and `ExternalComm` also needs the stub. Solution: use a `nullptr` bus for now and set it via a setter, OR use a separate IBus pointer from a previously created object.

**Simplest solution:** Create a `NullBus` stub for the StubActuatorManager, or pass `nullptr` and remove the bus publish from StubActuatorManager:

```cpp
// In App/Tasks/StubActuatorManager.h
class StubActuatorManager : public IActuatorManager {
public:
    void commandById(uint8_t /*id*/, const char* /*cmd*/) override {
        // Stub: silently ignore until ActuatorManager is implemented in Story 3.2
    }
};
```

This is simpler and safer — no bus dependency, no two-step init. Alternatively, if you want to log, you can pass `IBus* bus` and check for nullptr before calling publish.

**Recommended:** Use the no-bus version (stub silently ignores). Story 3.2 will replace this entirely.

### Complete `SystemInit::boot()` Implementation Pattern

```cpp
void SystemInit::boot() {
    // ── Step 1: Static member zero-init (snapshots) ────────────────────────
    OdoControl::latestSnapshot   = {};
    SensorManager::latestSnapshot = {};
    // ExternalComm::latestSnapshot is defined + zero-init in ExternalComm.cpp

    // ── Step 2: Create FreeRTOS primitives ─────────────────────────────────
    struct MotionSetpoint { float left; float right; };
    static QueueHandle_t motionMailbox = xQueueCreate(1, sizeof(MotionSetpoint));

    // ── Step 3: Instantiate HAL channel wrappers ────────────────────────────
    static UartChannel   uartCh{&huartX};  // replace X with actual UART handle
    static UsbCdcChannel usbCh{};

    // ── Step 4: Instantiate stub managers ──────────────────────────────────
    static StubActuatorManager stubActMgr{};

    // ── Step 5: Instantiate ExternalComm (implements IBus) ─────────────────
    static ExternalComm extComm{&uartCh, &usbCh, nullptr, &stubActMgr, motionMailbox};

    // ── Step 6: Create FreeRTOS tasks ──────────────────────────────────────
    xTaskCreate(ExternalComm::rxTask, "ExtRX",
                Config::STACK_EXTCOMM_RX, &extComm, Config::PRIO_EXTCOMM_RX, nullptr);
    xTaskCreate(ExternalComm::txTask, "ExtTX",
                Config::STACK_EXTCOMM_TX, &extComm, Config::PRIO_EXTCOMM_TX, nullptr);

    // ── Step 7: Keep blink task (diagnostic — remove when LED is repurposed) ─
    static auto blinkFn = [](void*) {
        for (;;) {
            HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    };
    xTaskCreate(blinkFn, "Blink", 128, nullptr, 1, nullptr);
}
```

**ETH is `nullptr`:** ExternalComm's `_transmitAll` and rxTask both null-check each channel. Passing `nullptr` for ETH is safe — it is silently skipped.

### `UartChannel::receive()` — Non-Blocking Line Reading

```cpp
uint16_t UartChannel::receive(char* buf, uint16_t maxLen, uint32_t /*timeoutMs*/) {
    uint16_t n = 0;
    while (n < maxLen - 1) {
        uint8_t b;
        if (HAL_UART_Receive(_huart, &b, 1, 0) != HAL_OK) break;
        buf[n++] = static_cast<char>(b);
        if (b == '\n') break;
    }
    if (n > 0) buf[n] = '\0';
    return n;
}
```

**Limitation:** `HAL_UART_Receive` in polling mode with timeout=0 reads one byte if immediately available. At 115200 baud, a byte arrives every ~87µs; the 1ms rxTask polling interval means up to 11 bytes per poll, insufficient for 80-char commands in a single cycle. This implementation works for slow/manual commands but may lose bytes in burst. A DMA + ring-buffer upgrade is the production solution but out of scope here.

**Call the polling loop incrementally** — ExternalComm's rxTask calls `receive(buf, 80, 0)` every 1ms, so commands are accumulated across multiple calls only if ExternalComm is enhanced to accumulate partial lines. For this story, single-call line reception is sufficient for functional testing.

### `UsbCdcChannel::transmit()` — CDC Include Path

```cpp
#include "usbd_cdc_if.h"   // from Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc/
```

This is in the CubeMX-generated middleware. It provides `CDC_Transmit_FS(uint8_t*, uint16_t)`. The include path is set by CubeMX in the IDE project.

```cpp
void UsbCdcChannel::transmit(const char* data, uint16_t len) {
    CDC_Transmit_FS(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), len);
    // Return value (USBD_OK / USBD_BUSY) intentionally ignored — fire-and-forget
}
```

**Note on const_cast:** `CDC_Transmit_FS` takes `uint8_t*` (non-const) but does not modify the buffer — the cast is safe here.

### `App/Drivers/` — HAL Dependency and Testability

`UartChannel` and `UsbCdcChannel` include STM32 HAL headers and therefore cannot be compiled on PC host (x86). This is expected — they are the HAL adaptation layer. They do NOT need unit tests because:
- ExternalComm tests use `MockCommChannel` (Story 1.4) which mocks the interface
- `UartChannel` / `UsbCdcChannel` are trivially thin wrappers

Story 1.6 will set up the Google Test infrastructure where ExternalComm is tested with mock channels, bypassing these concrete implementations entirely.

### `xTaskCreate` Return Value

From Story 1.1 review findings: `xTaskCreate` return value is not checked — silent failure if heap insufficient. For this story, document the limitation but do not add error handling (adding it with `publish()` creates a chicken-and-egg since ExternalComm might not be started yet). Production mitigation: increase FreeRTOS heap size in `FreeRTOSConfig.h` if tasks fail to create.

### Architecture Constraints Inherited

| Rule | Source | Application |
|---|---|---|
| No `new`/`delete` | NFR-02 | All objects are `static` locals in `boot()` — verified by grep |
| `xTaskCreate` only in SystemInit | FR-11 | No other file calls `xTaskCreate` |
| ETH = nullptr acceptable | Story 1.4 ExternalComm impl | All channel pointers null-checked |
| English identifiers only | NFR-08 | All new identifiers in English |
| Guards `APP_<FOLDER>_<FILE>_H` | NFR-06 | Both new headers follow convention |
| Private members prefixed `_` | NFR-06 | `_huart` in UartChannel |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes in `Core/`, `Drivers/`, `Middlewares/` |

### What Already Exists — Do Not Recreate

From Stories 1.1–1.4 (already in codebase):
- `App/SystemInit/SystemInit.h` — stub class, just needs .cpp updated
- `App/SystemInit/SystemInit.cpp` — has blink task + snapshot init; REMOVE CommSnapshot definition
- `App/Config.h` — all priorities and stack sizes (PRIO_EXTCOMM_RX=4, PRIO_EXTCOMM_TX=3, STACK_EXTCOMM_RX=256, STACK_EXTCOMM_TX=256)
- `App/Interfaces/ICommChannel.h` — `transmit()` and `receive()` pure abstract
- `App/Interfaces/IActuatorManager.h` — `commandById()` pure abstract
- `App/Tasks/ExternalComm.h` — full class: constructor signature, rxTask, txTask static methods
- `App/Tasks/ExternalComm.cpp` — full implementation + `ExternalComm::CommSnapshot ExternalComm::latestSnapshot{}` defined here
- `App/Tasks/OdoControl.h` — stub with `OdoSnapshot` struct and `static OdoSnapshot latestSnapshot`
- `App/Tasks/SensorManager.h` — stub with `SensorSnapshot` struct and `static SensorSnapshot latestSnapshot`

### Story 1.6 Dependency Note

Story 1.6 (Google Test infrastructure) will create `Tests/CMakeLists.txt` and `Tests/Mocks/`. The `ExternalCommTest.cpp` created in Story 1.4 cannot compile until Story 1.6 is done. This story does not need to address that.

### References

- Story requirements: [epics.md — Story 1.5](_bmad-output/planning-artifacts/epics.md)
- Architecture câblage: [architecture.md — Architectural Boundaries](_bmad-output/planning-artifacts/architecture.md#architectural-boundaries)
- Config.h constants: [architecture.md — FreeRTOS Configuration](_bmad-output/planning-artifacts/architecture.md#freertos-configuration)
- ExternalComm constructor signature: [1-4-externalcomm-...md — Tasks](_bmad-output/implementation-artifacts/1-4-externalcomm-implementation-ibus-et-communication-tri-canal-ascii.md)
- CommSnapshot duplicate issue: [deferred-work.md](_bmad-output/implementation-artifacts/deferred-work.md)
- CMSIS OS v2 boot sequence: [1-1-...md — Completion Notes](_bmad-output/implementation-artifacts/1-1-projet-stm32cubemx-configure-et-freertos-operationnel-sur-cible.md)
- ICommChannel interface: [App/Interfaces/ICommChannel.h] (created Story 1.4)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

- `huart1` est défini dans `Core/Src/main.c` (pas de fichier `usart.h`). Déclaration `extern "C" { extern UART_HandleTypeDef huart1; }` ajoutée dans SystemInit.cpp.
- StubActuatorManager implémenté sans dépendance bus (version silencieuse recommandée dans Dev Notes) pour éviter le problème chicken-and-egg.

### Completion Notes List

- Task 1: Symbole dupliqué `ExternalComm::latestSnapshot` supprimé de SystemInit.cpp. Défini uniquement dans ExternalComm.cpp (Story 1.4).
- Task 2: `UartChannel` créé — wrapper polling HAL non-bloquant sur `huart1` (USART1 DEBUG à 115200 baud).
- Task 3: `UsbCdcChannel` créé — `transmit()` via `CDC_Transmit_FS` (fire-and-forget), `receive()` retourne 0 (RX par IRQ hors scope).
- Task 4: `StubActuatorManager` créé (header-only) — `commandById()` silencieusement ignoré, remplacé Story 3.2.
- Task 5: `SystemInit::boot()` câble tous les objets statiques et crée les 3 tâches FreeRTOS (ExtRX, ExtTX, Blink). `motionMailbox` queue 1 slot créée.
- Task 6: `SystemInit.h` reste minimal — aucun include béton ajouté.
- Task 7: NFR validés — zéro `new`/`delete`, `xTaskCreate` uniquement dans SystemInit.cpp, identifiants en anglais, membres privés préfixés `_`.

### File List

- `App/SystemInit/SystemInit.cpp` — modifié (suppression CommSnapshot dupliqué, boot() complet)
- `App/Drivers/UartChannel.h` — créé
- `App/Drivers/UartChannel.cpp` — créé
- `App/Drivers/UsbCdcChannel.h` — créé
- `App/Drivers/UsbCdcChannel.cpp` — créé
- `App/Tasks/StubActuatorManager.h` — créé

## Change Log

- 2026-05-10: Story 1.5 implémentée — câblage statique complet dans SystemInit::boot(), drivers UART/USB-CDC créés, StubActuatorManager stub créé, symbole CommSnapshot dupliqué corrigé.
