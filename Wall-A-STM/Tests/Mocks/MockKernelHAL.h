#ifndef TESTS_MOCKS_MOCKKERNELHAL_H
#define TESTS_MOCKS_MOCKKERNELHAL_H

#include "Interfaces/IKernelHAL.h"
#include <vector>
#include <cstring>

// Settable fake of IKernelHAL for host tests. Defaults are healthy (high stack/heap) so
// tests that don't exercise the RTOS path see no spurious ALERTs.
class MockKernelHAL : public IKernelHAL {
public:
    uint32_t heapFree    = 16384;
    uint32_t heapMinFree = 8192;
    std::vector<RtosTaskInfo> taskList;

    uint32_t heapFreeBytes()    const override { return heapFree; }
    uint32_t heapMinFreeBytes() const override { return heapMinFree; }

    uint8_t tasks(RtosTaskInfo* out, uint8_t maxTasks) const override {
        uint8_t n = static_cast<uint8_t>(taskList.size() < maxTasks ? taskList.size() : maxTasks);
        for (uint8_t i = 0; i < n; i++) out[i] = taskList[i];
        return n;
    }

    // Convenience helper to push a task entry from a test.
    void addTask(const char* name, uint16_t stackFreeWords) {
        RtosTaskInfo t{};
        strncpy(t.name, name, sizeof(t.name) - 1);
        t.name[sizeof(t.name) - 1] = '\0';
        t.stackFreeWords = stackFreeWords;
        taskList.push_back(t);
    }
};

#endif // TESTS_MOCKS_MOCKKERNELHAL_H
