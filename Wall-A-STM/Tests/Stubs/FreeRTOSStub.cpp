#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "HalStub.h"
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
BaseType_t xQueueReceive(QueueHandle_t xQueue, void* buf, TickType_t xTicksToWait) {
    auto* q = static_cast<FakeQueue*>(xQueue);
    if (!q || q->items.empty()) {
        if (xTicksToWait == portMAX_DELAY) throw TaskDelayEscape{};
        return pdFALSE;
    }
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
void vQueueDelete(QueueHandle_t xQueue) {
    delete static_cast<FakeQueue*>(xQueue);
}

// QueueSet stubs — ExternalComm uses a set to wait on multiple queues
struct FakeQueueSet {
    std::vector<QueueHandle_t> members;
};

QueueSetHandle_t xQueueCreateSet(UBaseType_t) { return new FakeQueueSet{}; }

BaseType_t xQueueAddToSet(QueueSetMemberHandle_t q, QueueSetHandle_t set) {
    auto* s = static_cast<FakeQueueSet*>(set);
    if (s) s->members.push_back(static_cast<QueueHandle_t>(q));
    return pdTRUE;
}

// Returns the first member queue that has data; throws TaskDelayEscape if all empty (blocking semantics).
QueueSetMemberHandle_t xQueueSelectFromSet(QueueSetHandle_t set, TickType_t) {
    auto* s = static_cast<FakeQueueSet*>(set);
    if (s) {
        for (auto* q : s->members) {
            auto* fq = static_cast<FakeQueue*>(q);
            if (fq && !fq->items.empty()) return q;
        }
    }
    throw TaskDelayEscape{};
}

BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void* item, BaseType_t* pxWoken) {
    if (pxWoken) *pxWoken = pdFALSE;
    return xQueueSend(xQueue, item, 0);
}

// Throws TaskDelayEscape so task for(;;) loops exit after one iteration in tests
void vTaskDelay(TickType_t) {
    throw TaskDelayEscape{};
}
void vTaskDelayUntil(TickType_t* pxPreviousWakeTime, TickType_t xTimeIncrement) {
    if (pxPreviousWakeTime) *pxPreviousWakeTime += xTimeIncrement;
    throw TaskDelayEscape{};
}
TickType_t xTaskGetTickCount(void) { return 0; }
BaseType_t xTaskCreate(TaskFunction_t, const char*, uint16_t, void*, UBaseType_t, TaskHandle_t*) { return pdTRUE; }

uint32_t& getMockTick() {
    static uint32_t tick = 0;
    return tick;
}

static uint32_t g_notifyBits = 0;

void resetTestNotifications() { g_notifyBits = 0; }

BaseType_t xTaskNotify(TaskHandle_t, uint32_t ulValue, eNotifyAction eAction) {
    if (eAction == eSetBits) g_notifyBits |= ulValue;
    return pdTRUE;
}
BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
                           uint32_t* pulNotificationValue, TickType_t xTicksToWait) {
    g_notifyBits &= ~ulBitsToClearOnEntry;
    if (pulNotificationValue) *pulNotificationValue = g_notifyBits;
    g_notifyBits &= ~ulBitsToClearOnExit;
    if (xTicksToWait == portMAX_DELAY) throw TaskDelayEscape{};
    return pdTRUE;
}
