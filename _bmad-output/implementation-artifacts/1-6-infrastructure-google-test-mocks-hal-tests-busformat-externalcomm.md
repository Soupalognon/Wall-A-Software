# Story 1.6: Infrastructure Google Test + mocks HAL + tests BusFormat/ExternalComm

Status: review

## Story

As a developer,
I want a Google Test infrastructure configured for PC host with HAL mocks,
so that all business classes can be tested without STM32 hardware or ARM toolchain.

## Acceptance Criteria

1. **Given** `Tests/CMakeLists.txt` is created
   **When** `cmake .. && make` runs on PC host (x86)
   **Then** all unit tests compile and link without ARM toolchain

2. **Given** `Tests/Mocks/MockBus.h` exists and implements `IBus`
   **When** a test instantiates `MockBus` and passes it as `IBus*`
   **Then** published messages are captured in a buffer for assertion

3. **Given** `Tests/Unit/BusFormatTest.cpp` exists
   **When** all BusFormat factory methods are tested
   **Then** output strings match the exact expected ASCII format (including `\n` terminator)

4. **Given** `Tests/Unit/ExternalCommTest.cpp` exists (created in Story 1.4)
   **When** CMD parsing tests run with mock UART
   **Then** all valid variants dispatch correctly and invalid inputs are handled

5. **And** `MockMotorHAL.h` and `MockSensorHAL.h` exist in `Tests/Mocks/` — ready for Epic 2 and Epic 3

## Tasks / Subtasks

- [x] Task 1: Create FreeRTOS and HAL stub headers in `Tests/Stubs/` (AC: #1, #4)
  - [x] Create `Tests/Stubs/FreeRTOS.h` — type definitions: `QueueHandle_t`, `TaskHandle_t`, `UBaseType_t`, `BaseType_t`, `TickType_t`, `portMAX_DELAY`, `pdTRUE`, `pdFALSE`, `pdPASS`
  - [x] Create `Tests/Stubs/queue.h` — declarations: `xQueueCreate`, `xQueueOverwrite`, `xQueueSend`, `xQueueReceive`, `xQueueReset`, `xQueuePeek` (implemented in FreeRTOSStub.cpp)
  - [x] Create `Tests/Stubs/task.h` — declarations: `vTaskDelay`, `xTaskCreate`, `xTaskNotify`, `pdMS_TO_TICKS`, `taskENTER_CRITICAL`, `taskEXIT_CRITICAL`, `eNotifyAction` enum (at minimum `eSetBits`)
  - [x] Create `Tests/Stubs/FreeRTOSStub.cpp` — implement queue functions using `std::deque` under the hood (see Dev Notes for exact content)
  - [x] Create `Tests/Stubs/MockHAL.h` — `inline uint32_t& getMockTick()` + `inline uint32_t HAL_GetTick()` — needed by ExternalComm.cpp

- [x] Task 2: Create `Tests/Mocks/MockBus.h` (AC: #2)
  - [x] Class `MockBus : public IBus` with `void publish(Topic, const char*) override`
  - [x] Store published messages in `std::vector<Published>` where `Published = { Topic topic; std::string payload; }`
  - [x] Add helper methods: `clear()`, `hasPublished(Topic, const std::string& substr = "")`, `count(Topic)` (see Dev Notes)
  - [x] Include `"Interfaces/IBus.h"`, `<vector>`, `<string>`
  - [x] Guard: no prefix needed since it's in Tests/ — use `#ifndef TESTS_MOCKS_MOCKBUS_H`

- [x] Task 3: Create `Tests/Mocks/MockCommChannel.h` (AC: #4)
  - [x] Class `MockCommChannel : public ICommChannel`
  - [x] `transmit()`: appends to `std::vector<std::string> transmitted` for multi-call assertion
  - [x] `receive()`: pops front from `std::queue<std::string> rxQueue`; returns 0 if empty; copies to buf and returns byte count
  - [x] Helper: `void pushRx(const std::string& line)` — adds to rxQueue
  - [x] Guard: `#ifndef TESTS_MOCKS_MOCKCOMMCHANNEL_H`
  - [x] Note: MockCommChannel is also declared inline in `Tests/Unit/ExternalCommTest.cpp` (Story 1.4). The shared header version is canonical — ExternalCommTest.cpp should use `#include "MockCommChannel.h"` instead of its inline version

- [x] Task 4: Create `Tests/Mocks/MockMotorHAL.h` and interface stubs for Epic 2 (AC: #5)
  - [x] Create `App/Interfaces/IMotorHAL.h` — pure abstract: `virtual void setLeft(float pwm) = 0`, `virtual void setRight(float pwm) = 0` — guard `#ifndef APP_INTERFACES_IMOTORHAL_H`
  - [x] Create `App/Interfaces/IEncoderHAL.h` — pure abstract: `virtual int32_t readLeft() = 0`, `virtual int32_t readRight() = 0` — guard `#ifndef APP_INTERFACES_IENCODERHAL_H`
  - [x] Create `Tests/Mocks/MockMotorHAL.h` — implements `IMotorHAL`, stores last `pwmLeft` and `pwmRight` floats — guard `#ifndef TESTS_MOCKS_MOCKMOTORHAL_H`
  - [x] Create `Tests/Mocks/MockEncoderHAL.h` — implements `IEncoderHAL`, stores configurable return values — guard `#ifndef TESTS_MOCKS_MOCKENCODERHAL_H`

- [x] Task 5: Create `Tests/Mocks/MockSensorHAL.h` and interface stub for Epic 3 (AC: #5)
  - [x] Create `App/Interfaces/ISensorHAL.h` — pure abstract: `virtual float read() = 0` — guard `#ifndef APP_INTERFACES_ISENSORHAL_H`
  - [x] Create `Tests/Mocks/MockSensorHAL.h` — implements `ISensorHAL`, stores configurable `returnValue` float — guard `#ifndef TESTS_MOCKS_MOCKSENSORHAL_H`

- [x] Task 6: Create `Tests/Unit/BusFormatTest.cpp` (AC: #3)
  - [x] Include `<gtest/gtest.h>` and `"BusFormat.h"`
  - [x] Test `telOdo(1.23f, 0.45f, 90.0f)` → `"TEL ODO 1.23 0.45 90.00\n"` (exact string match)
  - [x] Test `altProximity(0.12f)` → starts with `"ALT "` and ends with `"\n"`
  - [x] Test `logInfo("SystemInit OK")` → `"LOG INFO SystemInit OK\n"`
  - [x] Test `hltTemp(36.5f)` → starts with `"HLT "` and ends with `"\n"`
  - [x] Test return value is never nullptr (all methods return non-null)
  - [x] Test `\n` terminator is always present (last char of returned string)

- [x] Task 7: Update `Tests/Unit/ExternalCommTest.cpp` to use shared mocks (AC: #4)
  - [x] Replace inline `MockCommChannel` class definition with `#include "MockCommChannel.h"`
  - [x] Add `#include "Stubs/MockHAL.h"` before ExternalComm headers
  - [x] Initialize `getMockTick() = 0` in test setUp
  - [x] Verify all 6 existing tests still compile (publish, CMD MOVE, CMD ACTUATOR, invalid input, UART priority, CommSnapshot)

- [x] Task 8: Create `Tests/CMakeLists.txt` (AC: #1)
  - [x] See Dev Notes for exact CMakeLists.txt content
  - [x] Use `FetchContent` to get Google Test v1.14.0 (or `find_package(GTest)` if system-installed)
  - [x] Add two test executables: `BusFormatTest` and `ExternalCommTest`
  - [x] Include `Tests/Stubs/` first in include path so stub headers shadow real FreeRTOS/HAL headers
  - [x] Link both executables against `GTest::gtest_main`

- [x] Task 9: Verify build and tests pass (AC: #1, #2, #3, #4)
  - [x] `cmake .. && ninja` from `Tests/build/` — zero errors
  - [x] `./BusFormatTest` — 9 tests pass
  - [x] `./ExternalCommTest` — 6 tests pass
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" Tests/` → zero results in identifiers

## Dev Notes

### Scope — Files This Story Creates/Modifies

```
App/
└── Interfaces/
    ├── IMotorHAL.h              ← NEW (minimal, for Epic 2 OdoControl)
    ├── IEncoderHAL.h            ← NEW (minimal, for Epic 2 OdoControl)
    └── ISensorHAL.h             ← NEW (minimal, for Epic 3 drivers)

Tests/
├── CMakeLists.txt               ← NEW
├── Mocks/
│   ├── MockBus.h                ← NEW
│   ├── MockCommChannel.h        ← NEW (shared; ExternalCommTest.cpp inline replaced)
│   ├── MockMotorHAL.h           ← NEW
│   ├── MockEncoderHAL.h         ← NEW
│   └── MockSensorHAL.h          ← NEW
├── Stubs/
│   ├── FreeRTOS.h               ← NEW (stub, shadows real header)
│   ├── queue.h                  ← NEW (stub, shadows real header)
│   ├── task.h                   ← NEW (stub, shadows real header)
│   ├── FreeRTOSStub.cpp         ← NEW (queue implementation using std::deque)
│   └── MockHAL.h                ← NEW (HAL_GetTick stub)
└── Unit/
    ├── BusFormatTest.cpp        ← NEW
    └── ExternalCommTest.cpp     ← UPDATE (use shared MockCommChannel, add MockHAL include)
```

**Do NOT touch:** `App/Tasks/`, `App/SystemInit/`, `App/Config.h`, `App/BusFormat.h/.cpp`, `Core/`, `Drivers/`, `Middlewares/`

---

### CRITICAL: FreeRTOS Host Compilation Problem

`ExternalComm.h` includes `<FreeRTOS.h>` and `<queue.h>` — these are STM32-specific headers absent on x86 host. Without stubs, the test build fails at the include stage.

**Solution:** `Tests/Stubs/` directory with fake headers. CMakeLists.txt adds `Tests/Stubs/` as the FIRST include path, so host builds find stubs before the real (non-existent-on-host) FreeRTOS headers.

**The stubs must define all FreeRTOS types and functions used in ExternalComm.cpp:**

| Used in ExternalComm | Stub requirement |
|---|---|
| `QueueHandle_t` | typedef to `void*` (or struct pointer) |
| `xQueueCreate(depth, size)` | returns fake handle |
| `xQueueOverwrite(q, item)` | stores item in fake queue |
| `xQueueSend(q, item, 0)` | drops or stores |
| `xQueueReceive(q, item, 0)` | retrieves or returns pdFALSE |
| `xQueueReset(q)` | clears fake queue |
| `xQueuePeek(q, item, 0)` | copies without removing |
| `vTaskDelay(ticks)` | no-op |
| `pdMS_TO_TICKS(ms)` | `(ms)` macro |
| `pdTRUE`, `pdFALSE` | `1`, `0` |

**HAL_GetTick:** Called in `ExternalComm::_processRxLine()`. On host, provide via `Tests/Stubs/MockHAL.h`. The `ExternalComm.h` uses `HAL_GetTick` indirectly through `Core/Inc/main.h` on STM32 — on host, `main.h` doesn't exist, so the test compilation includes `MockHAL.h` explicitly before ExternalComm headers.

---

### `Tests/Stubs/FreeRTOS.h` — Exact Content

```cpp
#ifndef TESTS_STUBS_FREERTOS_H
#define TESTS_STUBS_FREERTOS_H

#include <cstdint>
#include <cstddef>

// FreeRTOS types for host build
typedef void*    QueueHandle_t;
typedef void*    TaskHandle_t;
typedef uint32_t UBaseType_t;
typedef int32_t  BaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE   ((BaseType_t)1)
#define pdFALSE  ((BaseType_t)0)
#define pdPASS   pdTRUE
#define portMAX_DELAY ((TickType_t)0xFFFFFFFF)
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

// taskENTER/EXIT_CRITICAL are no-ops on host (single-threaded tests)
#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()

#endif // TESTS_STUBS_FREERTOS_H
```

---

### `Tests/Stubs/queue.h` — Exact Content

```cpp
#ifndef TESTS_STUBS_QUEUE_H
#define TESTS_STUBS_QUEUE_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
BaseType_t    xQueueOverwrite(QueueHandle_t xQueue, const void* pvItemToQueue);
BaseType_t    xQueueSend(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);
BaseType_t    xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);
BaseType_t    xQueueReset(QueueHandle_t xQueue);
BaseType_t    xQueuePeek(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif

#endif // TESTS_STUBS_QUEUE_H
```

---

### `Tests/Stubs/task.h` — Exact Content

```cpp
#ifndef TESTS_STUBS_TASK_H
#define TESTS_STUBS_TASK_H

#include "FreeRTOS.h"

typedef enum { eSetBits = 0, eIncrement, eSetValueWithOverwrite, eSetValueWithoutOverwrite, eNoAction } eNotifyAction;

typedef void (*TaskFunction_t)(void*);

#ifdef __cplusplus
extern "C" {
#endif

void      vTaskDelay(TickType_t xTicksToDelay);
BaseType_t xTaskCreate(TaskFunction_t pvTaskCode, const char* pcName,
                       uint16_t usStackDepth, void* pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask);
BaseType_t xTaskNotify(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction);

#ifdef __cplusplus
}
#endif

#endif // TESTS_STUBS_TASK_H
```

---

### `Tests/Stubs/FreeRTOSStub.cpp` — Queue Implementation

The stub uses a global map from handle to an internal queue struct backed by `std::vector<std::vector<uint8_t>>`:

```cpp
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <vector>
#include <deque>
#include <cstring>
#include <new>

struct FakeQueue {
    UBaseType_t  itemSize;
    UBaseType_t  maxDepth;
    std::deque<std::vector<uint8_t>> items;
};

QueueHandle_t xQueueCreate(UBaseType_t depth, UBaseType_t itemSize) {
    return new FakeQueue{ itemSize, depth, {} };
}
BaseType_t xQueueOverwrite(QueueHandle_t xQueue, const void* item) {
    auto* q = static_cast<FakeQueue*>(xQueue);
    if (!q) return pdFALSE;
    q->items.clear();
    std::vector<uint8_t> buf(q->itemSize);
    std::memcpy(buf.data(), item, q->itemSize);
    q->items.push_back(std::move(buf));
    return pdTRUE;
}
BaseType_t xQueueSend(QueueHandle_t xQueue, const void* item, TickType_t) {
    auto* q = static_cast<FakeQueue*>(xQueue);
    if (!q || q->items.size() >= q->maxDepth) return pdFALSE;
    std::vector<uint8_t> buf(q->itemSize);
    std::memcpy(buf.data(), item, q->itemSize);
    q->items.push_back(std::move(buf));
    return pdTRUE;
}
BaseType_t xQueueReceive(QueueHandle_t xQueue, void* buf, TickType_t) {
    auto* q = static_cast<FakeQueue*>(xQueue);
    if (!q || q->items.empty()) return pdFALSE;
    std::memcpy(buf, q->items.front().data(), q->itemSize);
    q->items.pop_front();
    return pdTRUE;
}
BaseType_t xQueueReset(QueueHandle_t xQueue) {
    auto* q = static_cast<FakeQueue*>(xQueue);
    if (!q) return pdFALSE;
    q->items.clear();
    return pdTRUE;
}
BaseType_t xQueuePeek(QueueHandle_t xQueue, void* buf, TickType_t) {
    auto* q = static_cast<FakeQueue*>(xQueue);
    if (!q || q->items.empty()) return pdFALSE;
    std::memcpy(buf, q->items.front().data(), q->itemSize);
    return pdTRUE;
}
void      vTaskDelay(TickType_t) {}
BaseType_t xTaskCreate(TaskFunction_t, const char*, uint16_t, void*, UBaseType_t, TaskHandle_t*) { return pdTRUE; }
BaseType_t xTaskNotify(TaskHandle_t, uint32_t, eNotifyAction) { return pdTRUE; }
```

**Memory note:** `new FakeQueue` uses heap — this is test code only (not in `App/`), so NFR-02 does not apply. `xQueueCreate` in the real FreeRTOS also uses heap internally.

---

### `Tests/Stubs/MockHAL.h` — HAL_GetTick for Host

```cpp
#ifndef TESTS_STUBS_MOCKHAL_H
#define TESTS_STUBS_MOCKHAL_H

#include <cstdint>

// Controlled tick counter for tests — set before each test that checks timestamps
inline uint32_t& getMockTick() {
    static uint32_t tick = 0;
    return tick;
}
inline uint32_t HAL_GetTick() { return getMockTick(); }

#endif // TESTS_STUBS_MOCKHAL_H
```

**Usage in ExternalCommTest.cpp:** Add `#include "Stubs/MockHAL.h"` as the FIRST include (before any App/ includes). Set `getMockTick() = 1000` before tests that verify CommSnapshot timestamps.

---

### `Tests/Mocks/MockBus.h` — Full Content

```cpp
#ifndef TESTS_MOCKS_MOCKBUS_H
#define TESTS_MOCKS_MOCKBUS_H

#include "Interfaces/IBus.h"
#include <vector>
#include <string>

class MockBus : public IBus {
public:
    struct Published {
        Topic       topic;
        std::string payload;
    };

    std::vector<Published> published;

    void publish(Topic t, const char* payload) override {
        published.push_back({ t, payload ? payload : "" });
    }

    void clear() { published.clear(); }

    bool hasPublished(Topic t, const std::string& substr = "") const {
        for (const auto& p : published) {
            if (p.topic != t) continue;
            if (substr.empty() || p.payload.find(substr) != std::string::npos)
                return true;
        }
        return false;
    }

    size_t count(Topic t) const {
        size_t n = 0;
        for (const auto& p : published) if (p.topic == t) ++n;
        return n;
    }

    const Published* last(Topic t) const {
        for (int i = static_cast<int>(published.size()) - 1; i >= 0; --i)
            if (published[static_cast<size_t>(i)].topic == t)
                return &published[static_cast<size_t>(i)];
        return nullptr;
    }
};

#endif // TESTS_MOCKS_MOCKBUS_H
```

---

### `Tests/Mocks/MockCommChannel.h` — Full Content

```cpp
#ifndef TESTS_MOCKS_MOCKCOMMCHANNEL_H
#define TESTS_MOCKS_MOCKCOMMCHANNEL_H

#include "Interfaces/ICommChannel.h"
#include <queue>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

class MockCommChannel : public ICommChannel {
public:
    std::vector<std::string> transmitted;
    std::queue<std::string>  rxQueue;

    void transmit(const char* data, uint16_t len) override {
        transmitted.emplace_back(data, len);
    }

    uint16_t receive(char* buf, uint16_t maxLen, uint32_t) override {
        if (rxQueue.empty()) return 0;
        const std::string& front = rxQueue.front();
        uint16_t n = static_cast<uint16_t>(std::min(front.size(), static_cast<size_t>(maxLen)));
        std::memcpy(buf, front.c_str(), n);
        rxQueue.pop();
        return n;
    }

    void pushRx(const std::string& line) { rxQueue.push(line); }
    void clearTx() { transmitted.clear(); }
};

#endif // TESTS_MOCKS_MOCKCOMMCHANNEL_H
```

**Note:** The existing inline `MockCommChannel` in `Tests/Unit/ExternalCommTest.cpp` (from Story 1.4) must be removed and replaced with `#include "MockCommChannel.h"`. The interface is identical — this is a clean consolidation.

---

### `Tests/CMakeLists.txt` — Full Content

```cmake
cmake_minimum_required(VERSION 3.14)
project(RobotCDRTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Match firmware compiler flags
add_compile_options(-fno-exceptions -fno-rtti -Wall -Wextra)

# Google Test — prefer system installation; fall back to FetchContent
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/v1.14.0.tar.gz
        URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Directory shortcuts
set(APP_DIR   "${CMAKE_SOURCE_DIR}/../App")
set(STUBS_DIR "${CMAKE_SOURCE_DIR}/Stubs")
set(MOCKS_DIR "${CMAKE_SOURCE_DIR}/Mocks")

# Common include paths — Stubs FIRST to shadow real FreeRTOS/HAL headers
set(COMMON_INCLUDES
    ${STUBS_DIR}      # FreeRTOS.h, queue.h, task.h stubs (must be first)
    ${APP_DIR}        # App/ classes: BusFormat.h, Tasks/ExternalComm.h, etc.
    ${MOCKS_DIR}      # MockBus.h, MockCommChannel.h, etc.
    ${CMAKE_SOURCE_DIR}  # Tests/ root — so #include "Stubs/MockHAL.h" resolves
)

# ── BusFormatTest ──────────────────────────────────────────────────────────
add_executable(BusFormatTest
    Unit/BusFormatTest.cpp
    ${APP_DIR}/BusFormat.cpp
)
target_include_directories(BusFormatTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(BusFormatTest PRIVATE GTest::gtest_main)

# ── ExternalCommTest ───────────────────────────────────────────────────────
add_executable(ExternalCommTest
    Unit/ExternalCommTest.cpp
    ${APP_DIR}/Tasks/ExternalComm.cpp
    ${APP_DIR}/BusFormat.cpp
    Stubs/FreeRTOSStub.cpp
)
target_include_directories(ExternalCommTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(ExternalCommTest PRIVATE GTest::gtest_main)

# ── Test discovery ─────────────────────────────────────────────────────────
include(GoogleTest)
gtest_discover_tests(BusFormatTest)
gtest_discover_tests(ExternalCommTest)
```

**Build steps from project root:**
```bash
cd Tests
mkdir -p build && cd build
cmake ..
make
./BusFormatTest
./ExternalCommTest
```

---

### `Tests/Unit/BusFormatTest.cpp` — Structure

```cpp
#include <gtest/gtest.h>
#include <cstring>
#include "BusFormat.h"

// telOdo: "TEL ODO x.xx y.yy a.aa\n"
TEST(BusFormat, telOdoFormat) {
    const char* s = BusFormat::telOdo(1.23f, 0.45f, 90.0f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s).substr(0, 8), "TEL ODO ");
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

// altProximity: "ALT PROXIMITY_LOW x.xx\n"
TEST(BusFormat, altProximityFormat) {
    const char* s = BusFormat::altProximity(0.12f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s).substr(0, 4), "ALT ");
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

// logInfo: "LOG INFO msg\n"
TEST(BusFormat, logInfoFormat) {
    const char* s = BusFormat::logInfo("SystemInit OK");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s), "LOG INFO SystemInit OK\n");
}

// hltTemp: "HLT TEMP x.x\n"
TEST(BusFormat, hltTempFormat) {
    const char* s = BusFormat::hltTemp(36.5f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s).substr(0, 4), "HLT ");
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

// All methods return non-null
TEST(BusFormat, returnsNonNull) {
    EXPECT_NE(BusFormat::telOdo(0, 0, 0), nullptr);
    EXPECT_NE(BusFormat::altProximity(0), nullptr);
    EXPECT_NE(BusFormat::logInfo(""), nullptr);
    EXPECT_NE(BusFormat::hltTemp(0), nullptr);
}
```

**Adapt exact string expectations to match BusFormat.cpp actual format** — check the method implementations in `App/BusFormat.cpp` before writing the expected strings. The exact float format (`%.2f` vs `%.1f`) must match.

---

### `Tests/Unit/ExternalCommTest.cpp` — Required Changes from Story 1.4

The file was created in Story 1.4 with an inline `MockCommChannel`. Make these changes:

1. Add at the very top (before all other includes):
   ```cpp
   #include "Stubs/MockHAL.h"   // provides HAL_GetTick() — must come first
   ```

2. Remove the inline `MockCommChannel` class definition.

3. Add:
   ```cpp
   #include "MockCommChannel.h"
   ```

4. In test setUp or per-test, reset tick: `getMockTick() = 0;`

No other changes needed — the 6 existing tests cover: publish dispatch, CMD MOVE parsing, CMD ACTUATOR dispatch, invalid input handling, UART priority (USB first then UART wins), CommSnapshot update.

---

### Interface Stubs for Epic 2/3 — Minimal Design

These interfaces are created NOW to unblock mock creation, but will be expanded in Epic 2 (OdoControl needs both) and Epic 3 (driver classes need ISensorHAL).

**`App/Interfaces/IMotorHAL.h`:**
```cpp
#ifndef APP_INTERFACES_IMOTORHAL_H
#define APP_INTERFACES_IMOTORHAL_H

class IMotorHAL {
public:
    virtual void setLeft(float pwm)  = 0;  // pwm: -1.0 to +1.0
    virtual void setRight(float pwm) = 0;  // pwm: -1.0 to +1.0
    virtual ~IMotorHAL() = default;
};

#endif // APP_INTERFACES_IMOTORHAL_H
```

**`App/Interfaces/IEncoderHAL.h`:**
```cpp
#ifndef APP_INTERFACES_IENCODERHAL_H
#define APP_INTERFACES_IENCODERHAL_H

#include <cstdint>

class IEncoderHAL {
public:
    virtual int32_t readLeft()  = 0;  // tick count since last read
    virtual int32_t readRight() = 0;  // tick count since last read
    virtual ~IEncoderHAL() = default;
};

#endif // APP_INTERFACES_IENCODERHAL_H
```

**`App/Interfaces/ISensorHAL.h`:**
```cpp
#ifndef APP_INTERFACES_ISENSORHAL_H
#define APP_INTERFACES_ISENSORHAL_H

class ISensorHAL {
public:
    virtual float read() = 0;  // raw sensor value
    virtual ~ISensorHAL() = default;
};

#endif // APP_INTERFACES_ISENSORHAL_H
```

**MockMotorHAL.h:**
```cpp
#ifndef TESTS_MOCKS_MOCKMOTORHAL_H
#define TESTS_MOCKS_MOCKMOTORHAL_H
#include "Interfaces/IMotorHAL.h"
class MockMotorHAL : public IMotorHAL {
public:
    float lastLeft = 0.0f;
    float lastRight = 0.0f;
    int   callCount = 0;
    void setLeft(float pwm)  override { lastLeft  = pwm; ++callCount; }
    void setRight(float pwm) override { lastRight = pwm; ++callCount; }
};
#endif // TESTS_MOCKS_MOCKMOTORHAL_H
```

**MockEncoderHAL.h:**
```cpp
#ifndef TESTS_MOCKS_MOCKENCODERHAL_H
#define TESTS_MOCKS_MOCKENCODERHAL_H
#include "Interfaces/IEncoderHAL.h"
class MockEncoderHAL : public IEncoderHAL {
public:
    int32_t leftTicks  = 0;
    int32_t rightTicks = 0;
    int32_t readLeft()  override { return leftTicks;  }
    int32_t readRight() override { return rightTicks; }
};
#endif // TESTS_MOCKS_MOCKENCODERHAL_H
```

**MockSensorHAL.h:**
```cpp
#ifndef TESTS_MOCKS_MOCKSENSORHAL_H
#define TESTS_MOCKS_MOCKSENSORHAL_H
#include "Interfaces/ISensorHAL.h"
class MockSensorHAL : public ISensorHAL {
public:
    float returnValue = 0.0f;
    float read() override { return returnValue; }
};
#endif // TESTS_MOCKS_MOCKSENSORHAL_H
```

---

### What Already Exists — Do Not Recreate

From Stories 1.1–1.5 (already in codebase):
- `App/Interfaces/IBus.h` — `Topic` enum, `IBus` pure abstract
- `App/Interfaces/ICommChannel.h` — `transmit()` + `receive()` (created Story 1.4)
- `App/Interfaces/IActuatorManager.h` — `commandById()` (created Story 1.2)
- `App/BusFormat.h/.cpp` — all factory methods (created Story 1.3)
- `App/Tasks/ExternalComm.h` — full class with `IBus` inheritance (created Story 1.4)
- `App/Tasks/ExternalComm.cpp` — full implementation including `_processRxLine` (created Story 1.4)
- `Tests/Unit/ExternalCommTest.cpp` — 6 tests, cannot compile yet — **this story makes it compile**
- `Tests/` directory itself — created in Story 1.4 when ExternalCommTest.cpp was written

**BusFormat method signatures (to align BusFormatTest.cpp)** — verify from `App/BusFormat.h`:
- `static const char* telOdo(float x, float y, float angle)`
- `static const char* altProximity(float dist)`
- `static const char* logInfo(const char* msg)`
- `static const char* hltTemp(float t)`

---

### Architecture Constraints

| Rule | Source | Application to this story |
|---|---|---|
| Develop only in `App/` and `Tests/` | NFR-07 | All new files are in `App/Interfaces/` or `Tests/` |
| No `new`/`delete` in `App/` | NFR-02 | `FreeRTOSStub.cpp` uses `new` — it's in `Tests/Stubs/`, not `App/` — acceptable |
| English identifiers only | NFR-08 | All new files: English identifiers, English comments |
| Guards `APP_<DOSSIER>_<FICHIER>_H` | NFR-06 | Applies to new `App/Interfaces/` files; `Tests/` uses `TESTS_*` prefix |
| Private members prefixed `_` | NFR-06 | Mock classes store data in public members (test helpers, not production classes) |
| Tests compile without STM32 toolchain | NFR-03 | Stubs directory solves this — entire story goal |

---

### Deferred Work from Previous Stories — Closed by This Story

From `deferred-work.md` (Story 1.1 review):
> Static members in `SystemInit.cpp` — risk of SIOF if constructors become non-trivial

This story does not directly close it, but the pattern is established: each class defines its own static members in its own `.cpp` (e.g., `ExternalComm::latestSnapshot` in `ExternalComm.cpp`). Future stories (2.1, 3.1) should follow this pattern when implementing OdoControl.cpp and SensorManager.cpp.

---

### Testing Note — ExternalComm FreeRTOS Behavior

The FreeRTOS stub enables testing `ExternalComm::publish()` and queue interactions. However, `rxTask` and `txTask` have `for(;;)` loops — tests should NOT call these directly as infinite loops. Instead:

- Test `publish()` by calling it directly and then calling `xQueueReceive` on the internal queue to verify message was enqueued (or use `txTask` called once via a thread, or refactor to test `_processRxLine` directly).
- The existing ExternalCommTest.cpp from Story 1.4 was designed with this in mind — verify its approach before implementing CMakeLists.txt.

If `txTask` loops prevent testing, the test may need a mechanism to break out after one iteration (e.g., a `static bool stopRequested` flag set by test). This design decision is left to the implementation — do not redesign ExternalComm architecture for testability in this story.

---

### References

- Story requirements: [epics.md — Story 1.6](_bmad-output/planning-artifacts/epics.md)
- ExternalComm full implementation: [1-4-externalcomm-...md](_bmad-output/implementation-artifacts/1-4-externalcomm-implementation-ibus-et-communication-tri-canal-ascii.md)
- SystemInit wiring: [1-5-systeminit-...md](_bmad-output/implementation-artifacts/1-5-systeminit-boot-cablage-statique-complet.md)
- Architecture NFR-03 (testability): [architecture.md — NonFunctional Requirements](_bmad-output/planning-artifacts/architecture.md)
- Deferred work (static member SIOF): [deferred-work.md](_bmad-output/implementation-artifacts/deferred-work.md)
- BusFormat methods: `App/BusFormat.h` (Story 1.3)
- ICommChannel interface: `App/Interfaces/ICommChannel.h` (Story 1.4)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

- **Problème 1** : `vTaskDelay` non déclaré dans ExternalComm.cpp — résolu en incluant `task.h` depuis `FreeRTOS.h` stub (comme le vrai FreeRTOS).
- **Problème 2** : `ExternalComm.cpp` inclut `<stm32f4xx_hal.h>` absent sur host — résolu par `Tests/Stubs/stm32f4xx_hal.h` qui inclut `MockHAL.h`.
- **Problème 3** : `ExternalComm::latestSnapshot` défini dans `SystemInit.cpp` (non compilé en tests) — résolu par `Tests/Stubs/StaticDefs.cpp`.
- **Problème 4** : `rxTask`/`txTask` ont des boucles infinies `for(;;)` — résolu : `vTaskDelay` lance `TaskDelayEscape` (exception), les tests attrapent cette exception. L'exception nécessite de retirer `-fno-exceptions` du CMakeLists.txt.
- **Problème 5** : `ExternalCommTest.cpp` (Story 1.4) utilisait `rxBuffer`/`lastTransmitted` incompatibles avec le `MockCommChannel` partagé — adapté pour utiliser `pushRx()`/`transmitted.back()`.

### Completion Notes List

- Tous les stubs FreeRTOS créés dans `Tests/Stubs/` (FreeRTOS.h, queue.h, task.h, FreeRTOSStub.cpp, MockHAL.h, stm32f4xx_hal.h)
- `StaticDefs.cpp` fournit `ExternalComm::latestSnapshot` (normalement dans SystemInit.cpp, non compilable sur host)
- `TaskDelayEscape` exception dans `vTaskDelay` permet aux tests de faire sortir les tâches FreeRTOS de leur boucle infinie après une itération
- `ExternalCommTest.cpp` adapté : `pushRx()`/`transmitted.back()`, helpers `runRxOnce()`/`runTxOnce()`, try/catch TaskDelayEscape
- 9 tests BusFormat + 6 tests ExternalComm → tous passent sur PC host x86 (MinGW64, g++ 15.2.0)
- Aucun caractère non-ASCII dans les identifiants (conforme NFR-08)
- cmake installé via MSYS2 : `"D:/msys64/mingw64.exe" pacman -S --noconfirm mingw-w64-x86_64-cmake`
- Commande de build : `cmake .. -G Ninja && ninja` depuis `Tests/build/`, avec `D:/msys64/mingw64/bin` dans PATH

### File List

- `robot-cdr/Tests/CMakeLists.txt` — NEW
- `robot-cdr/Tests/Stubs/FreeRTOS.h` — NEW
- `robot-cdr/Tests/Stubs/queue.h` — NEW
- `robot-cdr/Tests/Stubs/task.h` — NEW
- `robot-cdr/Tests/Stubs/FreeRTOSStub.cpp` — NEW
- `robot-cdr/Tests/Stubs/MockHAL.h` — NEW
- `robot-cdr/Tests/Stubs/stm32f4xx_hal.h` — NEW
- `robot-cdr/Tests/Stubs/StaticDefs.cpp` — NEW (définit ExternalComm::latestSnapshot)
- `robot-cdr/Tests/Mocks/MockBus.h` — NEW
- `robot-cdr/Tests/Mocks/MockCommChannel.h` — NEW
- `robot-cdr/Tests/Mocks/MockMotorHAL.h` — NEW
- `robot-cdr/Tests/Mocks/MockEncoderHAL.h` — NEW
- `robot-cdr/Tests/Mocks/MockSensorHAL.h` — NEW
- `robot-cdr/App/Interfaces/IMotorHAL.h` — NEW
- `robot-cdr/App/Interfaces/IEncoderHAL.h` — NEW
- `robot-cdr/App/Interfaces/ISensorHAL.h` — NEW
- `robot-cdr/Tests/Unit/BusFormatTest.cpp` — NEW
- `robot-cdr/Tests/Unit/ExternalCommTest.cpp` — UPDATE

### Change Log

- 2026-05-10 : Story 1.6 implémentée — infrastructure Google Test opérationnelle sur PC host x86 (15 fichiers créés, 1 mis à jour). BusFormatTest (9 tests) et ExternalCommTest (6 tests) passent. Stubs FreeRTOS/HAL résolvent la compilation hors-cible STM32.
