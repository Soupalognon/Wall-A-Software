#include "Drivers/KernelMonitor.h"
#include <cstring>

uint32_t KernelMonitor::heapFreeBytes() const {
	return static_cast<uint32_t>(xPortGetFreeHeapSize());
}

uint32_t KernelMonitor::heapMinFreeBytes() const {
	return static_cast<uint32_t>(xPortGetMinimumEverFreeHeapSize());
}

uint8_t KernelMonitor::tasks(RtosTaskInfo *out, uint8_t maxTasks) const {
	// uxTaskGetSystemState snapshots every live task in one pass (needs
	// configUSE_TRACE_FACILITY); it returns 0 if the array is too small.
	TaskStatus_t status[Config::MAX_RTOS_TASKS];
	uint8_t cap = (maxTasks < Config::MAX_RTOS_TASKS) ? maxTasks : Config::MAX_RTOS_TASKS;
	UBaseType_t n = uxTaskGetSystemState(status, cap, nullptr);

	for (UBaseType_t i = 0; i < n; i++) {
		strncpy(out[i].name, status[i].pcTaskName, sizeof(out[i].name) - 1);
		out[i].name[sizeof(out[i].name) - 1] = '\0';
		out[i].stackFreeWords = status[i].usStackHighWaterMark;
	}
	return static_cast<uint8_t>(n);
}
