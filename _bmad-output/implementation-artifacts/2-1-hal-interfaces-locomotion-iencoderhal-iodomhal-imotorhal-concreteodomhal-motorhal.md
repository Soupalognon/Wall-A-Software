# Story 2.1: HAL Interfaces Locomotion — IEncoderHAL, IOdomHAL, IMotorHAL, ConcreteOdomHAL, MotorHAL + Config.h OdoControl

Status: review

## Story

As a developer,
I want the locomotion HAL interfaces (IEncoderHAL, IOdomHAL, IMotorHAL) and their concrete implementations (ConcreteOdomHAL wrapping Wall-A Encoder, MotorHAL wrapping Wall-A Drv8262) in place, with all OdoControl constants declared in Config.h,
so that OdoControl accesses odometry and motor control through clean abstractions that are testable without STM32 hardware via injected mocks.

## Acceptance Criteria

1. **Given** `App/Interfaces/IEncoderHAL.h` exists
   **When** `ConcreteOdomHAL` implements it
   **Then** `IEncoderHAL` exposes `int32_t getTicksLeft()` and `int32_t getTicksRight()` returning cumulative 32-bit tick counts — rollover handling via `(int16_t)(current - old)` cast is done inside `ConcreteOdomHAL::update()`, not in the interface

2. **Given** `App/Interfaces/IOdomHAL.h` exists
   **When** `OdoControl` receives `IOdomHAL*` by constructor injection
   **Then** `OdoControl` accesses `update()`, `getX()`, `getY()`, `getAngle()`, `getVLeft()`, `getVRight()`, `getV()`, `getW()`, `getDt()` — without ever knowing `IEncoderHAL` or `Encoder` directly

3. **Given** `App/Drivers/ConcreteOdomHAL.h/.cpp` exists
   **When** `ConcreteOdomHAL::update()` executes
   **Then** it reads ticks via `IEncoderHAL*`, computes `dL`, `dR`, integrates `(x, y, angle)` by differential Euler, computes speeds `(vL, vR, v, ω)`, and exposes `dt` via `getDt()` (hardware µs timer, implementation delegated to user via CubeMX TIM6 or equivalent)

4. **And** `ConcreteOdomHAL` holds `Encoder _encoderL, _encoderR` by value (no pointer indirection at this level) — the single virtual level `IOdomHAL` exists for testability of `OdoControl`, not in cascade

5. **And** angle normalization is applied after each `_angle += dTheta` integration:
   ```cpp
   if (_angle >  M_PI) _angle -= 2.0f * M_PI;
   if (_angle < -M_PI) _angle += 2.0f * M_PI;
   ```

6. **Given** `App/Interfaces/IMotorHAL.h` exists
   **When** `MotorHAL` implements it by wrapping `Drv8262::setMotors(float, float)`
   **Then** `IMotorHAL` exposes exactly `virtual void setMotors(float left, float right) = 0` — pure adapter pattern, zero rewrite of the Wall-A driver

7. **Given** `App/Config.h` is updated with OdoControl constants
   **When** `grep` is run on `App/Tasks/OdoControl*` for numeric literals related to control
   **Then** zero occurrences — all values come from `Config::*`:
   ```cpp
   constexpr float    ARRIVAL_THRESHOLD    = 0.02f;
   constexpr float    PID_I_MAX_SPEED      = 0.5f;
   constexpr float    PID_I_MAX_ANGLE      = 0.5f;
   constexpr float    MAX_DUTY             = 0.8f;
   constexpr float    STALL_DUTY_THRESHOLD = 0.5f;
   constexpr uint32_t STALL_TIME_MS        = 500;
   constexpr int8_t   ENCODER_L_SIGN       = +1;
   constexpr int8_t   ENCODER_R_SIGN       = +1;
   constexpr int8_t   MOTOR_L_SIGN         = +1;
   constexpr int8_t   MOTOR_R_SIGN         = +1;
   constexpr uint8_t  TELEM_DIVIDER        = 10;
   ```

8. **Given** Wall-A drivers are reused
   **When** the story starts
   **Then** `Encoder.cpp` and `Encoder.hpp` are copied from `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Encoder\` to `robot-cdr/App/Drivers/Encoder/` without modification

9. **And** `Drv8262.cpp` and `Drv8262.hpp` are copied from `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Motor\` to `robot-cdr/App/Drivers/Motor/` without modification

10. **Given** `Tests/Mocks/MockOdomHAL.h` and `Tests/Mocks/MockMotorHAL.h` (updated) exist
    **When** `ConcreteOdomHALTest.cpp` runs on PC host with injected mock `IEncoderHAL`
    **Then** Euler odometry is validated with known tick deltas: `dL = deltaL * D_PER_TICK`, `dR = deltaR * D_PER_TICK`, expected `x`, `y`, `angle` are verified — tests pass without STM32 hardware

## Tasks / Subtasks

- [x] Task 1: Copy Wall-A driver files into project (AC: #8, #9)
  - [x] Create directory `robot-cdr/App/Drivers/Encoder/` if it does not exist
  - [x] Copy `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Encoder\Encoder.cpp` → `robot-cdr/App/Drivers/Encoder/Encoder.cpp`
  - [x] Copy `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Encoder\Encoder.hpp` → `robot-cdr/App/Drivers/Encoder/Encoder.hpp`
  - [x] Create directory `robot-cdr/App/Drivers/Motor/` if it does not exist
  - [x] Copy `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Motor\Drv8262.cpp` → `robot-cdr/App/Drivers/Motor/Drv8262.cpp`
  - [x] Copy `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Motor\Drv8262.hpp` → `robot-cdr/App/Drivers/Motor/Drv8262.hpp`
  - [x] Verify both files compile in the new project (includes may need adjustment for CubeMX path differences)

- [x] Task 2: Create/Update `App/Interfaces/IEncoderHAL.h` (AC: #1)
  - [x] **BREAKING CHANGE** — Story 1.6 defined `readLeft()`/`readRight()`. The new interface uses `getTicksLeft()`/`getTicksRight()` returning cumulative 32-bit counts
  - [x] If `IEncoderHAL.h` already exists with `readLeft`/`readRight`, rename these methods to `getTicksLeft`/`getTicksRight` AND update `MockEncoderHAL.h` accordingly
  - [x] Guard: `#ifndef APP_INTERFACES_IENCODERHAL_H`
  - [x] Interface implemented

- [x] Task 3: Create `App/Interfaces/IOdomHAL.h` (AC: #2)
  - [x] Guard: `#ifndef APP_INTERFACES_IODOMHAL_H`
  - [x] Interface implemented

- [x] Task 4: Create/Update `App/Interfaces/IMotorHAL.h` (AC: #6)
  - [x] **BREAKING CHANGE** — replaced `setLeft`/`setRight` with `setMotors(float left, float right)`
  - [x] Updated `MockMotorHAL.h` accordingly
  - [x] Guard: `#ifndef APP_INTERFACES_IMOTORHAL_H`
  - [x] Interface implemented

- [x] Task 5: Create `App/Drivers/ConcreteOdomHAL.h` (AC: #2, #3, #4, #5)
  - [x] Guard: `#ifndef APP_DRIVERS_CONCRETEODOMHAL_H`
  - [x] Class declared with IEncoderHAL* injection

- [x] Task 6: Create `App/Drivers/ConcreteOdomHAL.cpp` (AC: #3, #4, #5)
  - [x] Euler differential odometry implemented
  - [x] `getDtFromTimer()` stub with `HAL_GetTick() * 1000u` fallback (0.005f default in tests)
  - [x] All getters implemented

- [x] Task 7: Add `D_PER_TICK` constexpr to `App/Config.h` and add physical constants (AC: #7)
  - [x] Added WHEEL_RADIUS_M, WHEEL_BASE_M, TICKS_PER_REV, D_PER_TICK
  - [x] Added 11 OdoControl constants (ARRIVAL_THRESHOLD, PID_I_MAX_*, MAX_DUTY, STALL_*, ENCODER_*_SIGN, MOTOR_*_SIGN, TELEM_DIVIDER)
  - [x] No duplicates with existing constants

- [x] Task 8: Create `App/Drivers/MotorHAL.h/.cpp` (AC: #6)
  - [x] Pure adapter wrapping Drv8262::setMotors with sign correction
  - [x] No double-clamping

- [x] Task 9: Update `Tests/Mocks/MockOdomHAL.h` — create new mock (AC: #10)
  - [x] MockOdomHAL created implementing IOdomHAL
  - [x] MockMotorHAL updated to setMotors(float, float)

- [x] Task 10: Create `Tests/Unit/ConcreteOdomHALTest.cpp` (AC: #10)
  - [x] MockEncoderHAL updated (getTicksLeft/getTicksRight)
  - [x] 5 tests: ZeroTicksNoDisplacement, EqualForwardTicksMovesStraight, LeftTickOnlyTurnsRight, AngleNormalizationStaysInBounds, SignFlipChangesTrajectory
  - [x] All 5 PASSED

- [x] Task 11: Update `Tests/CMakeLists.txt` (AC: #10)
  - [x] ConcreteOdomHALTest target added with correct sources and include paths

- [x] Task 12: Update `App/SystemInit/SystemInit.cpp` — update stubs for new interfaces (AC: #6)
  - [x] No StubMotorHAL/StubEncoderHAL existed — interfaces already updated in Tasks 2 and 4

- [x] Task 13: Verify NFR compliance
  - [x] No dynamic allocation in ConcreteOdomHAL.cpp / MotorHAL.cpp
  - [x] No accented characters in new files
  - [x] All include guards follow APP_* convention

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Config.h                          ← UPDATE (add D_PER_TICK + 11 OdoControl constants)
├── Interfaces/
│   ├── IEncoderHAL.h                 ← UPDATE (rename readLeft/readRight → getTicksLeft/getTicksRight)
│   ├── IOdomHAL.h                    ← NEW
│   └── IMotorHAL.h                   ← UPDATE (replace setLeft/setRight → setMotors)
├── Drivers/
│   ├── Encoder/
│   │   ├── Encoder.cpp               ← COPY from Wall-A (no modification)
│   │   └── Encoder.hpp               ← COPY from Wall-A (no modification)
│   ├── Motor/
│   │   ├── Drv8262.cpp               ← COPY from Wall-A (no modification)
│   │   └── Drv8262.hpp               ← COPY from Wall-A (no modification)
│   ├── ConcreteOdomHAL.h             ← NEW
│   ├── ConcreteOdomHAL.cpp           ← NEW
│   ├── MotorHAL.h                    ← NEW
│   └── MotorHAL.cpp                  ← NEW
└── SystemInit/
    └── SystemInit.cpp                ← UPDATE (fix stubs to new interface signatures)

Tests/
├── CMakeLists.txt                    ← UPDATE (add ConcreteOdomHALTest)
├── Mocks/
│   ├── MockOdomHAL.h                 ← NEW
│   ├── MockMotorHAL.h                ← UPDATE (setMotors signature)
│   └── MockEncoderHAL.h             ← UPDATE (getTicksLeft/getTicksRight)
└── Unit/
    └── ConcreteOdomHALTest.cpp       ← NEW
```

**Do NOT touch:** `App/Tasks/OdoControl.h/.cpp` (rewritten in Story 2.2), `Core/`, `Drivers/` (CubeMX generated), `Middlewares/`

---

### CRITICAL: Breaking Interface Changes from Story 1.6

Story 1.6 created placeholder interfaces. This story updates them with the real signatures from the brainstorm:

| Interface | Story 1.6 (old) | Story 2.1 (new) |
|-----------|-----------------|-----------------|
| `IEncoderHAL` | `readLeft() → int32_t` | `getTicksLeft() → int32_t` (cumulative) |
| `IEncoderHAL` | `readRight() → int32_t` | `getTicksRight() → int32_t` (cumulative) |
| `IMotorHAL` | `setLeft(float)` | `setMotors(float left, float right)` |
| `IMotorHAL` | `setRight(float)` | *(merged into setMotors)* |

**Callers to update after interface change:**
- `Tests/Mocks/MockMotorHAL.h` — update method signature
- `Tests/Mocks/MockEncoderHAL.h` — update method names
- `App/SystemInit/SystemInit.cpp` — update `StubMotorHAL` and `StubEncoderHAL`
- `Tests/Unit/OdoControlTest.cpp` — will be rewritten in Story 2.2 anyway

---

### Wall-A Encoder — 16-bit Rollover Handling

The Wall-A `Encoder` class reads the STM32 timer counter register (16-bit, 0–65535). The 32-bit cumulative tick count is built by calling `Encoder::refresh()` which accumulates signed 16-bit deltas:

```cpp
// Inside Encoder::refresh() (Wall-A original):
int16_t delta = (int16_t)(currentCount - _lastCount);
_cumulativeTicks += delta;
_lastCount = currentCount;
```

`ConcreteOdomHAL::update()` calls `Encoder::refresh()` then reads the cumulative count via `getTicksLeft()` / `getTicksRight()`. The `(int16_t)` cast is the key: it correctly handles wrap-around of the 16-bit counter at no extra cost.

---

### getDt() — Hardware µs Timer

`ConcreteOdomHAL::getDt()` returns the elapsed time in seconds since last `update()` call. The implementation uses a CubeMX-configured free-running µs timer (e.g., TIM6 at 1µs/tick):

```cpp
// Placeholder implementation — replace with real timer
uint32_t ConcreteOdomHAL::getDtFromTimer() {
    return HAL_GetTick() * 1000u;  // ms → µs (1ms resolution, not ideal)
}
```

**For real use:** Configure TIM6 in CubeMX as a 1µs free-running counter. Replace `getDtFromTimer()`:
```cpp
uint32_t ConcreteOdomHAL::getDtFromTimer() {
    return __HAL_TIM_GET_COUNTER(&htim6);  // 1µs resolution
}
```

At 200Hz, the real dt is ~4.9–5.1ms. The µs timer measures this to ±1µs precision.

On **PC host tests**, `getDt()` returns the fixed default `_dt = 0.005f` — no timer needed.

---

### Drv8262 — Existing Clamp Behavior

`Drv8262::setMotors(float left, float right)` already clamps each value to `[-1, 1]` in its `dutyToCCR()` method. `MotorHAL` does NOT need to re-clamp — `OdoControl` applies `Config::MAX_DUTY` clamping upstream (Story 2.2). The Drv8262 clamp is the final hardware safety net only.

---

### Config.h — D_PER_TICK Must Be Constexpr

`D_PER_TICK` is used in the hot path of `ConcreteOdomHAL::update()` at 200Hz. Declare as `constexpr` so the compiler substitutes the constant at compile time:

```cpp
constexpr float D_PER_TICK = (2.0f * 3.14159265f * WHEEL_RADIUS_M)
                              / static_cast<float>(TICKS_PER_REV);
```

If `TICKS_PER_REV` uses quadrature encoding (4× pulses), multiply by 4: `TICKS_PER_REV = 4 * base_ticks_per_rev`.

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| `OdoControl` only consumer of `IOdomHAL` | FR-01 adapted | `ConcreteOdomHAL` is not used outside `OdoControl` wiring in `SystemInit` |
| No HAL CubeMX direct calls in `App/` | Architecture boundary | `ConcreteOdomHAL` calls `Encoder::refresh()`, not `HAL_TIM_*` directly |
| No dynamic allocation | NFR-02 | `ConcreteOdomHAL` holds `Encoder` by value, all members on stack/static |
| English identifiers | NFR-08 | All new code in English |
| Guards convention | NFR-06 | `APP_INTERFACES_IODOMHAL_H`, `APP_DRIVERS_CONCRETEODOMHAL_H`, etc. |
| Develop only in `App/` and `Tests/` | NFR-07 | Copied Wall-A files go into `App/Drivers/`, not `Drivers/` (CubeMX) |

---

### References

- Story requirements: [epics.md — Story 2.1](_bmad-output/planning-artifacts/epics.md)
- Brainstorming decisions: [brainstorming-session-2026-05-13-1000.md](_bmad-output/brainstorming/brainstorming-session-2026-05-13-1000.md) — Thèmes A, D
- Architecture: [architecture.md](_bmad-output/planning-artifacts/architecture.md)
- Wall-A Encoder source: `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Encoder\`
- Wall-A Drv8262 source: `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Motor\`
- Mocks from Story 1.6: [1-6-infrastructure-google-test-mocks-hal-tests-busformat-externalcomm.md](_bmad-output/implementation-artifacts/1-6-infrastructure-google-test-mocks-hal-tests-busformat-externalcomm.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

### Completion Notes List

- Tous les 13 tasks complétés. 20/20 tests passent (5 nouveaux ConcreteOdomHALTest + 9 BusFormatTest + 6 ExternalCommTest).
- Breaking changes IEncoderHAL/IMotorHAL appliqués proprement — aucun caller STM32 existant à mettre à jour (stubs 2.2 à câbler en Story 2.2).
- `getDtFromTimer()` retourne `HAL_GetTick() * 1000u` comme stub ; en tests HAL_GetTick()=0 donc fallback _dt=0.005f utilisé automatiquement.
- Stub `xQueueReceive`/`xQueueSelectFromSet` amélioré pour supporter portMAX_DELAY (throw TaskDelayEscape) et QueueSets — correction pre-existante nécessaire pour ExternalCommTest.

### File List

- App/Config.h (modifié — +15 constantes)
- App/Interfaces/IEncoderHAL.h (modifié — getTicksLeft/getTicksRight)
- App/Interfaces/IOdomHAL.h (nouveau)
- App/Interfaces/IMotorHAL.h (modifié — setMotors)
- App/Drivers/Encoder/Encoder.cpp (copié depuis Wall-A)
- App/Drivers/Encoder/Encoder.hpp (copié depuis Wall-A)
- App/Drivers/Motor/Drv8262.cpp (copié depuis Wall-A)
- App/Drivers/Motor/Drv8262.hpp (copié depuis Wall-A)
- App/Drivers/ConcreteOdomHAL.h (nouveau)
- App/Drivers/ConcreteOdomHAL.cpp (nouveau)
- App/Drivers/MotorHAL.h (nouveau)
- App/Drivers/MotorHAL.cpp (nouveau)
- Tests/CMakeLists.txt (modifié — +ConcreteOdomHALTest)
- Tests/Mocks/MockOdomHAL.h (nouveau)
- Tests/Mocks/MockMotorHAL.h (modifié — setMotors)
- Tests/Mocks/MockEncoderHAL.h (modifié — getTicksLeft/getTicksRight)
- Tests/Unit/ConcreteOdomHALTest.cpp (nouveau)
- Tests/Stubs/queue.h (modifié — +QueueSetHandle_t et fonctions QueueSet)
- Tests/Stubs/FreeRTOSStub.cpp (modifié — +QueueSet impl + portMAX_DELAY blocking semantics)

### Change Log

- 2026-05-14 — Story 2.1 implémentée : interfaces HAL locomotion, ConcreteOdomHAL, MotorHAL, constantes Config.h, 5 tests unitaires Euler odométrie. (claude-sonnet-4-6)
