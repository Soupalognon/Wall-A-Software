# Story 3.4: Drivers concrets — ISensor et IActuator

Status: review

## Story

As a developer,
I want six concrete drivers implementing `ISensor` or `IActuator` in `App/Drivers/`,
so that the extensibility pattern is validated: each driver is a new class only, zero modification of existing classes.

## Acceptance Criteria

1. **Given** `ProximitySensor`, `TemperatureSensor`, `CurrentSensor` implement `ISensor`
   **When** each is instantiated and injected into `SensorManager` by `SystemInit`
   **Then** `SensorManager` orchestrates them without any modification to its code

2. **Given** `Pump`, `Servo`, `LinearTransducer` implement `IActuator`
   **When** each is instantiated and injected into `ActuatorManager` by `SystemInit`
   **Then** `ActuatorManager` routes commands to them without any modification to its code

3. **Given** a concrete driver is added
   **When** `grep -r "new\|delete" App/Drivers/` is run
   **Then** zero occurrences — drivers do not allocate dynamically

4. **And** each driver accesses hardware via an injected HAL interface (`ISensorHAL*` for sensors, `IActuatorHAL*` for actuators) — never a direct HAL CubeMX call

5. **Given** `Tests/Unit/SensorDriversTest.cpp` and `Tests/Unit/ActuatorDriversTest.cpp` exist
   **When** tests run on PC host with mock HALs
   **Then** all tests pass without STM32 hardware

## Tasks / Subtasks

- [x] Task 1: Create `App/Interfaces/IActuatorHAL.h` (AC: #4)
  - [x] Guard: `#ifndef APP_INTERFACES_IACTUATORHAL_H`
  - [x] Single pure virtual: `virtual void set(float value) = 0;` — wraps GPIO/PWM/DAC HAL
  - [x] Virtual destructor

- [x] Task 2: Add alarm thresholds to `App/Config.h` (AC: #1)
  - [x] Add after `MONITORING_STALE_MS`:
    ```cpp
    static constexpr float PROXIMITY_ALARM_M  = 0.20f;
    static constexpr float TEMP_ALARM_C       = 60.0f;
    static constexpr float CURRENT_ALARM_A    = 2.0f;
    ```

- [x] Task 3: Create `App/Drivers/ProximitySensor.h/.cpp` (AC: #1, #3, #4)
  - [x] Implements `ISensor`, injects `ISensorHAL*` via constructor
  - [x] `read()` delegates to `_hal->read()`, stores `_lastValue`
  - [x] `isAlarm()` returns `_lastValue < Config::PROXIMITY_ALARM_M` (too close = alarm)
  - [x] `name()` returns `"PROXIMITY"` — all-caps English, no accented chars (NFR-08)

- [x] Task 4: Create `App/Drivers/TemperatureSensor.h/.cpp` (AC: #1, #3, #4)
  - [x] Implements `ISensor`, injects `ISensorHAL*` via constructor
  - [x] `read()` delegates to `_hal->read()`, stores `_lastValue`
  - [x] `isAlarm()` returns `_lastValue > Config::TEMP_ALARM_C`
  - [x] `name()` returns `"TEMPERATURE"`

- [x] Task 5: Create `App/Drivers/CurrentSensor.h/.cpp` (AC: #1, #3, #4)
  - [x] Implements `ISensor`, injects `ISensorHAL*` via constructor
  - [x] `read()` delegates to `_hal->read()`, stores `_lastValue`
  - [x] `isAlarm()` returns `_lastValue > Config::CURRENT_ALARM_A`
  - [x] `name()` returns `"CURRENT"`

- [x] Task 6: Create `App/Drivers/Pump.h/.cpp` (AC: #2, #3, #4)
  - [x] Implements `IActuator`, injects `IActuatorHAL*` via constructor
  - [x] `command("ON")` → `_hal->set(1.0f)` ; `command("OFF")` → `_hal->set(0.0f)`
  - [x] Unknown commands are silently ignored (no crash)
  - [x] `name()` returns `"PUMP"`

- [x] Task 7: Create `App/Drivers/Servo.h/.cpp` (AC: #2, #3, #4)
  - [x] Implements `IActuator`, injects `IActuatorHAL*` via constructor
  - [x] `command(cmd)` parses angle via `sscanf(cmd, "%f", &angle)`, calls `_hal->set(angle)`
  - [x] `name()` returns `"SERVO"`

- [x] Task 8: Create `App/Drivers/LinearTransducer.h/.cpp` (AC: #2, #3, #4)
  - [x] Implements `IActuator`, injects `IActuatorHAL*` via constructor
  - [x] `command(cmd)` parses position via `sscanf(cmd, "%f", &pos)`, calls `_hal->set(pos)`
  - [x] `name()` returns `"LINEAR_TRANSDUCER"`

- [x] Task 9: Create `Tests/Mocks/MockActuatorHAL.h` (AC: #5)
  - [x] Implements `IActuatorHAL`, stores last set value and call count
  - [x] Pattern: identical to `MockSensorHAL.h` but with `set(float v)` instead of `read()`

- [x] Task 10: Create `Tests/Unit/SensorDriversTest.cpp` (AC: #5)
  - [x] Tests for `ProximitySensor`, `TemperatureSensor`, `CurrentSensor` in one file
  - [x] Inject `MockSensorHAL` for each — no FreeRTOS stub needed (pure classes)
  - [x] See Dev Notes for required test list

- [x] Task 11: Create `Tests/Unit/ActuatorDriversTest.cpp` (AC: #5)
  - [x] Tests for `Pump`, `Servo`, `LinearTransducer` in one file
  - [x] Inject `MockActuatorHAL` for each — no FreeRTOS stub needed
  - [x] See Dev Notes for required test list

- [x] Task 12: Update `App/SystemInit/SystemInit.cpp` — wire concrete drivers (AC: #1, #2)
  - [x] Add includes for all 6 new driver headers + `IActuatorHAL.h`
  - [x] Add stub HAL structs in anonymous namespace (for `ISensorHAL` and `IActuatorHAL`)
  - [x] Add static instances of all 6 concrete drivers at file scope
  - [x] Replace empty `sensors[]` / `actuators[]` arrays with populated ones
  - [x] Update `sensorCount` and `actuatorCount` to match actual counts
  - [x] See Dev Notes for exact code structure

- [x] Task 13: Update `Tests/CMakeLists.txt` — add two new test targets (AC: #5)
  - [x] `SensorDriversTest`: sources = `SensorDriversTest.cpp` + 3 `.cpp` driver files (no FreeRTOSStub)
  - [x] `ActuatorDriversTest`: sources = `ActuatorDriversTest.cpp` + 3 `.cpp` driver files (no FreeRTOSStub)
  - [x] Add `gtest_discover_tests()` for each

- [x] Task 14: Update `Tests/run_tests.sh` — add both new tests to TESTS array (AC: #5)
  - [x] Add `SensorDriversTest` et `ActuatorDriversTest` après `ActuatorManagerTest`

- [x] Task 15: Verify NFR compliance
  - [x] No `new`/`delete`/`malloc`/`free` in any `App/Drivers/` file
  - [x] No accented identifiers — English only (NFR-08)
  - [x] Guards follow `APP_DRIVERS_<FILENAME>_H` convention (NFR-06)
  - [x] Private members prefixed `_` (NFR-06)
  - [x] No direct HAL CubeMX calls in drivers (`HAL_GPIO_WritePin`, `HAL_ADC_*`, etc.)
  - [x] No `snprintf` inline in drivers (NFR, FR-09)
  - [x] All thresholds come from `Config::` (NFR-10, no magic numbers)

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
├── Config.h                              ← UPDATE (add 3 alarm threshold constants)
├── Interfaces/
│   └── IActuatorHAL.h                    ← NEW
├── Drivers/
│   ├── ProximitySensor.h/.cpp            ← NEW
│   ├── TemperatureSensor.h/.cpp          ← NEW
│   ├── CurrentSensor.h/.cpp              ← NEW
│   ├── Pump.h/.cpp                       ← NEW
│   ├── Servo.h/.cpp                      ← NEW
│   └── LinearTransducer.h/.cpp           ← NEW
└── SystemInit/
    └── SystemInit.cpp                    ← UPDATE (wire concrete drivers)

Tests/
├── CMakeLists.txt                        ← UPDATE (add 2 test targets)
├── run_tests.sh                          ← UPDATE (add to TESTS array)
├── Mocks/
│   └── MockActuatorHAL.h                 ← NEW
└── Unit/
    ├── SensorDriversTest.cpp             ← NEW
    └── ActuatorDriversTest.cpp           ← NEW
```

**Do NOT touch:** `App/Tasks/SensorManager.h/.cpp`, `App/Tasks/ActuatorManager.h/.cpp`, `App/Interfaces/ISensor.h`, `App/Interfaces/IActuator.h`, `App/Interfaces/ISensorHAL.h`, `Core/`, `Drivers/` (CubeMX), `Middlewares/`

---

### CRITICAL: Extensibility Pattern Validation

The entire point of this story is to prove that `SensorManager` and `ActuatorManager` require **zero modification**. The driver is a new class only. Before submitting, verify that `SensorManager.h/.cpp` and `ActuatorManager.h/.cpp` are byte-for-byte identical to what they were before this story.

---

### CRITICAL: IActuatorHAL — New Interface Required

`IActuatorHAL` is NOT in the existing interfaces list (architecture was written before this story). It must be created in `App/Interfaces/IActuatorHAL.h`:

```cpp
#ifndef APP_INTERFACES_IACTUATORHAL_H
#define APP_INTERFACES_IACTUATORHAL_H

class IActuatorHAL {
public:
    virtual void set(float value) = 0;
    virtual ~IActuatorHAL() = default;
};

#endif // APP_INTERFACES_IACTUATORHAL_H
```

`set(float value)` maps semantically to:
- Pump: 0.0f = OFF, 1.0f = ON (GPIO)
- Servo: angle in degrees 0.0–180.0 (PWM duty cycle)
- LinearTransducer: position 0.0–1.0 or mm (DAC/PWM)

The actual hardware mapping is done in `SystemInit`'s stub (and later in concrete HAL drivers). The driver class doesn't need to know which peripheral is used.

---

### Sensor Driver — Exact Pattern

All three sensor drivers follow the identical pattern. Only the `name()` return and the `isAlarm()` threshold direction differ.

```cpp
// App/Drivers/ProximitySensor.h
#ifndef APP_DRIVERS_PROXIMITYSENSOR_H
#define APP_DRIVERS_PROXIMITYSENSOR_H

#include "Interfaces/ISensor.h"
#include "Interfaces/ISensorHAL.h"
#include "Config.h"
#include <cstdint>

class ProximitySensor : public ISensor {
public:
    ProximitySensor(uint8_t id, ISensorHAL* hal);
    uint8_t     id()   const override;
    const char* name() const override;
    float       read()       override;
    bool        isAlarm()    override;
private:
    uint8_t     _id;
    ISensorHAL* _hal;
    float       _lastValue;
};

#endif // APP_DRIVERS_PROXIMITYSENSOR_H
```

```cpp
// App/Drivers/ProximitySensor.cpp
#include "Drivers/ProximitySensor.h"

ProximitySensor::ProximitySensor(uint8_t id, ISensorHAL* hal)
    : _id(id), _hal(hal), _lastValue(0.0f) {}

uint8_t     ProximitySensor::id()   const { return _id; }
const char* ProximitySensor::name() const { return "PROXIMITY"; }

float ProximitySensor::read() {
    _lastValue = _hal->read();
    return _lastValue;
}

bool ProximitySensor::isAlarm() {
    return _lastValue < Config::PROXIMITY_ALARM_M;  // too close = alarm
}
```

**TemperatureSensor:** identical structure, `name()` = `"TEMPERATURE"`, `isAlarm()` = `_lastValue > Config::TEMP_ALARM_C`

**CurrentSensor:** identical structure, `name()` = `"CURRENT"`, `isAlarm()` = `_lastValue > Config::CURRENT_ALARM_A`

**Header guard convention:**
- `APP_DRIVERS_PROXIMITYSENSOR_H`
- `APP_DRIVERS_TEMPERATURESENSOR_H`
- `APP_DRIVERS_CURRENTSENSOR_H`

---

### Actuator Driver — Exact Pattern

```cpp
// App/Drivers/Pump.h
#ifndef APP_DRIVERS_PUMP_H
#define APP_DRIVERS_PUMP_H

#include "Interfaces/IActuator.h"
#include "Interfaces/IActuatorHAL.h"
#include <cstdint>
#include <cstring>

class Pump : public IActuator {
public:
    Pump(uint8_t id, IActuatorHAL* hal);
    uint8_t     id()   const override;
    const char* name() const override;
    void command(const char* cmd) override;
private:
    uint8_t       _id;
    IActuatorHAL* _hal;
};

#endif // APP_DRIVERS_PUMP_H
```

```cpp
// App/Drivers/Pump.cpp
#include "Drivers/Pump.h"

Pump::Pump(uint8_t id, IActuatorHAL* hal) : _id(id), _hal(hal) {}
uint8_t     Pump::id()   const { return _id; }
const char* Pump::name() const { return "PUMP"; }

void Pump::command(const char* cmd) {
    if (strcmp(cmd, "ON") == 0)        _hal->set(1.0f);
    else if (strcmp(cmd, "OFF") == 0)  _hal->set(0.0f);
    // unknown commands silently ignored — no crash, no log here (ActuatorManager handles unknown IDs)
}
```

```cpp
// App/Drivers/Servo.cpp
#include "Drivers/Servo.h"
#include <cstdio>

void Servo::command(const char* cmd) {
    float angle = 0.0f;
    sscanf(cmd, "%f", &angle);
    _hal->set(angle);
}
```

```cpp
// App/Drivers/LinearTransducer.cpp
void LinearTransducer::command(const char* cmd) {
    float pos = 0.0f;
    sscanf(cmd, "%f", &pos);
    _hal->set(pos);
}
```

**Header guard convention:**
- `APP_DRIVERS_PUMP_H`
- `APP_DRIVERS_SERVO_H`
- `APP_DRIVERS_LINEARTRANSDUCER_H`

---

### MockActuatorHAL — Exact Structure

```cpp
// Tests/Mocks/MockActuatorHAL.h
#ifndef TESTS_MOCKS_MOCKACTUATORHAL_H
#define TESTS_MOCKS_MOCKACTUATORHAL_H

#include "Interfaces/IActuatorHAL.h"

class MockActuatorHAL : public IActuatorHAL {
public:
    float    lastValue   = 0.0f;
    uint32_t callCount   = 0;

    void set(float value) override {
        lastValue = value;
        ++callCount;
    }

    void reset() { lastValue = 0.0f; callCount = 0; }
};

#endif // TESTS_MOCKS_MOCKACTUATORHAL_H
```

Pattern mirrors `MockSensorHAL.h` (which has `returnValue` and `read()`).

---

### SensorDriversTest.cpp — Full Structure

No FreeRTOS dependency — sensor drivers are pure C++ classes. Do NOT include FreeRTOS stubs.

```cpp
#include <gtest/gtest.h>
#include "Mocks/MockSensorHAL.h"
#include "Drivers/ProximitySensor.h"
#include "Drivers/TemperatureSensor.h"
#include "Drivers/CurrentSensor.h"
```

**Required tests (minimum 9, 3 per driver):**

| Test | Driver | What it validates |
|------|--------|-------------------|
| `ProxRead_DelegatesToHAL` | ProximitySensor | `read()` returns HAL value |
| `ProxAlarm_WhenTooClose` | ProximitySensor | `isAlarm()` true when value < PROXIMITY_ALARM_M |
| `ProxNoAlarm_WhenFar` | ProximitySensor | `isAlarm()` false when value >= PROXIMITY_ALARM_M |
| `TempRead_DelegatesToHAL` | TemperatureSensor | `read()` returns HAL value |
| `TempAlarm_WhenOverThreshold` | TemperatureSensor | `isAlarm()` true when value > TEMP_ALARM_C |
| `TempNoAlarm_WhenCool` | TemperatureSensor | `isAlarm()` false when value <= TEMP_ALARM_C |
| `CurrRead_DelegatesToHAL` | CurrentSensor | `read()` returns HAL value |
| `CurrAlarm_WhenOverThreshold` | CurrentSensor | `isAlarm()` true when value > CURRENT_ALARM_A |
| `CurrNoAlarm_WhenNormal` | CurrentSensor | `isAlarm()` false when value <= CURRENT_ALARM_A |

Example tests:
```cpp
TEST(ProximitySensorTest, ProxRead_DelegatesToHAL) {
    MockSensorHAL hal;
    hal.returnValue = 0.35f;
    ProximitySensor sensor{1, &hal};
    EXPECT_FLOAT_EQ(0.35f, sensor.read());
}

TEST(ProximitySensorTest, ProxAlarm_WhenTooClose) {
    MockSensorHAL hal;
    hal.returnValue = 0.10f;  // < 0.20m threshold
    ProximitySensor sensor{1, &hal};
    sensor.read();
    EXPECT_TRUE(sensor.isAlarm());
}

TEST(ProximitySensorTest, ProxNoAlarm_WhenFar) {
    MockSensorHAL hal;
    hal.returnValue = 0.30f;  // > 0.20m threshold
    ProximitySensor sensor{1, &hal};
    sensor.read();
    EXPECT_FALSE(sensor.isAlarm());
}
```

**Important: always call `sensor.read()` before `sensor.isAlarm()`** — `isAlarm()` uses `_lastValue` which is only updated by `read()`. Initial value is `0.0f`.

---

### ActuatorDriversTest.cpp — Full Structure

```cpp
#include <gtest/gtest.h>
#include "Mocks/MockActuatorHAL.h"
#include "Drivers/Pump.h"
#include "Drivers/Servo.h"
#include "Drivers/LinearTransducer.h"
```

**Required tests (minimum 7):**

| Test | Driver | What it validates |
|------|--------|-------------------|
| `PumpOn_SetsHALTo1` | Pump | `command("ON")` → `hal.lastValue == 1.0f` |
| `PumpOff_SetsHALTo0` | Pump | `command("OFF")` → `hal.lastValue == 0.0f` |
| `PumpUnknown_NoHALCall` | Pump | `command("SPIN")` → `hal.callCount == 0` |
| `ServoAngle_SetsHALValue` | Servo | `command("90.0")` → `hal.lastValue == 90.0f` |
| `ServoZero_SetsHALToZero` | Servo | `command("0")` → `hal.lastValue == 0.0f` |
| `TransducerPos_SetsHALValue` | LinearTransducer | `command("0.5")` → `hal.lastValue == 0.5f` |
| `ActuatorIds_AreCorrect` | all | `id()` and `name()` return injected values |

Example tests:
```cpp
TEST(PumpTest, PumpOn_SetsHALTo1) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    pump.command("ON");
    EXPECT_FLOAT_EQ(1.0f, hal.lastValue);
    EXPECT_EQ(1u, hal.callCount);
}

TEST(PumpTest, PumpUnknown_NoHALCall) {
    MockActuatorHAL hal;
    Pump pump{1, &hal};
    pump.command("SPIN");
    EXPECT_EQ(0u, hal.callCount);  // unknown command: HAL not called
}

TEST(ServoTest, ServoAngle_SetsHALValue) {
    MockActuatorHAL hal;
    Servo servo{2, &hal};
    servo.command("90.0");
    EXPECT_FLOAT_EQ(90.0f, hal.lastValue);
}
```

---

### CMakeLists.txt — Exact Additions

Add BEFORE the `# ── Test discovery` block:

```cmake
# ── SensorDriversTest ─────────────────────────────────────────────────────
add_executable(SensorDriversTest
    Unit/SensorDriversTest.cpp
    ${APP_DIR}/Drivers/ProximitySensor.cpp
    ${APP_DIR}/Drivers/TemperatureSensor.cpp
    ${APP_DIR}/Drivers/CurrentSensor.cpp
)
target_include_directories(SensorDriversTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(SensorDriversTest PRIVATE GTest::gtest_main)

# ── ActuatorDriversTest ───────────────────────────────────────────────────
add_executable(ActuatorDriversTest
    Unit/ActuatorDriversTest.cpp
    ${APP_DIR}/Drivers/Pump.cpp
    ${APP_DIR}/Drivers/Servo.cpp
    ${APP_DIR}/Drivers/LinearTransducer.cpp
)
target_include_directories(ActuatorDriversTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(ActuatorDriversTest PRIVATE GTest::gtest_main)
```

Add at the bottom of `# ── Test discovery` block:
```cmake
gtest_discover_tests(SensorDriversTest)
gtest_discover_tests(ActuatorDriversTest)
```

**Why no FreeRTOSStub.cpp?** Sensor and actuator drivers are pure C++ classes — no `vTaskDelay`, no `xTaskNotify`, no FreeRTOS primitives. Including FreeRTOSStub would add unnecessary compile time and link time with zero benefit.

---

### SystemInit.cpp — Exact Changes

**Current state of SystemInit (relevant sections):**

Currently, the sensors and actuators arrays are empty stubs:
```cpp
// Current — file scope
static IActuator* actuators[Config::MAX_ACTUATORS] = {};
static uint8_t    actuatorCount = 0;
static ActuatorManager actuatorMgr{actuators, actuatorCount, nullptr};

// Current — inside boot()
static ISensor* sensors[Config::MAX_SENSORS] = {};
static uint8_t  sensorCount = 0;
static SensorManager sensorManager{sensors, sensorCount, MotionPlanner::handle, &extComm};
```

**After this story:**

Add includes at the top (after `#include "Tasks/ActuatorManager.h"`):
```cpp
#include "Interfaces/IActuatorHAL.h"
#include "Drivers/ProximitySensor.h"
#include "Drivers/TemperatureSensor.h"
#include "Drivers/CurrentSensor.h"
#include "Drivers/Pump.h"
#include "Drivers/Servo.h"
#include "Drivers/LinearTransducer.h"
```

Add stub HALs to the anonymous namespace:
```cpp
namespace {
    // ... existing StubEncoderHAL and StubMotorHAL ...

    struct StubSensorHAL : public ISensorHAL {
        float read() override { return 0.0f; }
    };

    struct StubActuatorHAL : public IActuatorHAL {
        void set(float) override {}
    };
}
```

Replace the current empty arrays at file scope:
```cpp
// Stub HAL instances — replace with concrete HAL drivers when hardware is wired
static StubSensorHAL stubProxHAL, stubTempHAL, stubCurrentHAL;
static ProximitySensor    proxSensor{1, &stubProxHAL};
static TemperatureSensor  tempSensor{2, &stubTempHAL};
static CurrentSensor      currentSensor{3, &stubCurrentHAL};

static ISensor* sensors[Config::MAX_SENSORS] = {
    &proxSensor, &tempSensor, &currentSensor
};
static uint8_t sensorCount = 3;

static StubActuatorHAL stubPumpHAL, stubServoHAL, stubTransducerHAL;
static Pump             pump{1, &stubPumpHAL};
static Servo            servo{2, &stubServoHAL};
static LinearTransducer linearTransducer{3, &stubTransducerHAL};

static IActuator* actuators[Config::MAX_ACTUATORS] = {
    &pump, &servo, &linearTransducer
};
static uint8_t actuatorCount = 3;

static ActuatorManager actuatorMgr{actuators, actuatorCount, nullptr};
```

In `boot()`, remove the static local sensor declarations and replace with:
```cpp
static SensorManager sensorManager{sensors, sensorCount, MotionPlanner::handle, &extComm};
xTaskCreate(SensorManager::task, "SensorMgr",
    Config::STACK_SENSOR_MANAGER, &sensorManager,
    Config::PRIO_SENSOR_MANAGER, nullptr);
```

**Declaration order is critical:** Stub HALs → concrete drivers → arrays → actuatorMgr (because actuatorMgr holds a pointer to the array, and drivers must exist before their pointers are stored).

---

### run_tests.sh — Exact Addition

In the `TESTS=(...)` array, add after `ActuatorManagerTest`:
```bash
SensorDriversTest
ActuatorDriversTest
```

---

### Architecture Constraints

| Rule | Source | Application |
|------|--------|-------------|
| No `new`/`delete` in `App/` | NFR-02 | All drivers allocated statically in `SystemInit` |
| English identifiers only | NFR-08 | `"PROXIMITY"`, `"TEMPERATURE"`, `"CURRENT"`, `"PUMP"`, `"SERVO"`, `"LINEAR_TRANSDUCER"` |
| Guards: `APP_DRIVERS_<FILENAME>_H` | NFR-06 | See each driver header above |
| Private members prefixed `_` | NFR-06 | `_id`, `_hal`, `_lastValue` |
| No magic numbers | NFR-10 | All thresholds from `Config::` |
| No direct HAL CubeMX calls | Arch boundary #1 | Only `ISensorHAL*` and `IActuatorHAL*` |
| No snprintf in drivers | FR-09 | Drivers don't format messages — only `ActuatorManager` publishes via `BusFormat` |
| SystemInit sole wiring point | FR-11 | Only `SystemInit.cpp` instantiates and injects drivers |
| Develop only in `App/` and `Tests/` | NFR-07 | No changes to `Core/`, `Drivers/` (CubeMX), `Middlewares/` |
| Drivers belong in `App/Drivers/` | Arch boundary | Drivers call HAL (stub for now) — they are NOT tasks, services, or controllers |

---

### What Already Exists — Do Not Recreate

- `App/Interfaces/ISensor.h` — `id()`, `name()`, `read()`, `isAlarm()` (Story 1.2)
- `App/Interfaces/IActuator.h` — `id()`, `name()`, `command(const char*)` (Story 1.2)
- `App/Interfaces/ISensorHAL.h` — `read()` → float (Story 1.2)
- `Tests/Mocks/MockSensor.h` — `MockSensor` with settable value and alarm — **do NOT recreate**; this is for testing `SensorManager`, not for testing concrete drivers (concrete drivers use `MockSensorHAL`, not `MockSensor`)
- `Tests/Mocks/MockActuator.h` — `MockActuator` for `ActuatorManagerTest` — do NOT recreate
- `Tests/Mocks/MockSensorHAL.h` — `MockSensorHAL` with `returnValue` + `read()` — use directly in `SensorDriversTest`
- `App/Config.h` — `MAX_SENSORS=15`, `MAX_ACTUATORS=10` already defined; only the 3 threshold constants are new
- `App/SystemInit/SystemInit.cpp` — anonymous namespace already has `StubEncoderHAL` and `StubMotorHAL` — follow the same pattern for `StubSensorHAL` and `StubActuatorHAL`

---

### Previous Story Learnings (from Story 3.3)

- `HalStub.h` must be included FIRST in test files that use `HAL_GetTick()` — sensor/actuator driver tests do NOT use `HAL_GetTick()`, so this ordering is not required here
- Sensor/actuator driver tests do NOT need `FreeRTOS.h`, `FreeRTOSStub.cpp`, or `StaticDefs.cpp` — pure class tests with no OS primitives
- `BusFormat.cpp` is NOT needed in driver test targets — drivers don't format messages
- Pattern for `_lastValue`: always initialize to `0.0f` in constructor initializer list; `isAlarm()` depends on `_lastValue` being updated by `read()` first (safe because `SensorManager::pollOnce()` always calls `read()` before `isAlarm()`)

---

### References

- Story requirements: [epics.md — Story 3.4](..//planning-artifacts/epics.md)
- ISensor interface: [ISensor.h](Wall-A-STM/App/Interfaces/ISensor.h)
- IActuator interface: [IActuator.h](Wall-A-STM/App/Interfaces/IActuator.h)
- ISensorHAL interface: [ISensorHAL.h](Wall-A-STM/App/Interfaces/ISensorHAL.h)
- MockSensorHAL (reuse): [MockSensorHAL.h](Wall-A-STM/Tests/Mocks/MockSensorHAL.h)
- MockSensor (do not use for driver tests): [MockSensor.h](Wall-A-STM/Tests/Mocks/MockSensor.h)
- MockActuator (do not use for driver tests): [MockActuator.h](Wall-A-STM/Tests/Mocks/MockActuator.h)
- SensorManager (must not change): [SensorManager.cpp](Wall-A-STM/App/Tasks/SensorManager.cpp)
- ActuatorManager (must not change): [ActuatorManager.cpp](Wall-A-STM/App/Tasks/ActuatorManager.cpp)
- SystemInit current state: [SystemInit.cpp](Wall-A-STM/App/SystemInit/SystemInit.cpp)
- CMakeLists current state: [CMakeLists.txt](Wall-A-STM/Tests/CMakeLists.txt)
- run_tests.sh current state: [run_tests.sh](Wall-A-STM/Tests/run_tests.sh)
- Config.h current state: [Config.h](Wall-A-STM/App/Config.h)
- Previous story pattern: [3-3-monitoring.md](_bmad-output/implementation-artifacts/3-3-monitoring-supervision-par-pull-sur-shared-memory-horodatee.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

Aucun obstacle rencontré — implémentation directe conforme aux spécifications des Dev Notes.

### Completion Notes List

- ✅ IActuatorHAL.h créé dans App/Interfaces/ — interface pure avec `set(float)` et destructeur virtuel
- ✅ Config.h mis à jour — 3 constantes d'alarme ajoutées avant MONITORING_STALE_MS
- ✅ 3 drivers capteurs créés (ProximitySensor, TemperatureSensor, CurrentSensor) — pattern identique, seulement `name()` et direction de `isAlarm()` diffèrent
- ✅ 3 drivers actionneurs créés (Pump, Servo, LinearTransducer) — Pump utilise strcmp, Servo et LinearTransducer utilisent sscanf
- ✅ MockActuatorHAL.h créé — miroir de MockSensorHAL avec `set(float)` au lieu de `read()`
- ✅ 12 tests capteurs + 11 tests actionneurs créés (total 23 tests, minimum requis : 9+7=16)
- ✅ SystemInit.cpp mis à jour — StubSensorHAL et StubActuatorHAL dans namespace anonyme, 6 drivers filescope statiques, tableaux sensors[]/actuators[] peuplés
- ✅ CMakeLists.txt mis à jour — 2 nouvelles cibles sans FreeRTOSStub
- ✅ run_tests.sh mis à jour — SensorDriversTest et ActuatorDriversTest ajoutés
- ✅ SensorManager.h/.cpp et ActuatorManager.h/.cpp non modifiés — pattern d'extensibilité validé
- ✅ NFR compliance vérifiée : zéro new/delete, identifiants anglais, guards corrects, membres privés avec `_`, pas de HAL CubeMX direct, pas de snprintf, seuils via Config::

### File List

- `Wall-A-STM/App/Interfaces/IActuatorHAL.h` — NEW
- `Wall-A-STM/App/Config.h` — MODIFIED (3 constantes alarme)
- `Wall-A-STM/App/Drivers/ProximitySensor.h` — NEW
- `Wall-A-STM/App/Drivers/ProximitySensor.cpp` — NEW
- `Wall-A-STM/App/Drivers/TemperatureSensor.h` — NEW
- `Wall-A-STM/App/Drivers/TemperatureSensor.cpp` — NEW
- `Wall-A-STM/App/Drivers/CurrentSensor.h` — NEW
- `Wall-A-STM/App/Drivers/CurrentSensor.cpp` — NEW
- `Wall-A-STM/App/Drivers/Pump.h` — NEW
- `Wall-A-STM/App/Drivers/Pump.cpp` — NEW
- `Wall-A-STM/App/Drivers/Servo.h` — NEW
- `Wall-A-STM/App/Drivers/Servo.cpp` — NEW
- `Wall-A-STM/App/Drivers/LinearTransducer.h` — NEW
- `Wall-A-STM/App/Drivers/LinearTransducer.cpp` — NEW
- `Wall-A-STM/App/SystemInit/SystemInit.cpp` — MODIFIED (wiring concret)
- `Wall-A-STM/Tests/Mocks/MockActuatorHAL.h` — NEW
- `Wall-A-STM/Tests/Unit/SensorDriversTest.cpp` — NEW
- `Wall-A-STM/Tests/Unit/ActuatorDriversTest.cpp` — NEW
- `Wall-A-STM/Tests/CMakeLists.txt` — MODIFIED (2 nouvelles cibles)
- `Wall-A-STM/Tests/run_tests.sh` — MODIFIED (2 tests ajoutés)

### Change Log

- 2026-05-14 : Implémentation complète Story 3.4 — 6 drivers concrets (ISensor × 3, IActuator × 3), IActuatorHAL, MockActuatorHAL, 23 tests unitaires, wiring SystemInit, CMake et run_tests mis à jour
