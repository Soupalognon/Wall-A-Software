#ifndef APP_DRIVERS_KERNELMONITOR_H
#define APP_DRIVERS_KERNELMONITOR_H

#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"
#include "Config.h"
#include "Interfaces/IKernelHAL.h"

// Wraps the FreeRTOS introspection API (uxTaskGetSystemState + heap counters) behind
// IKernelHAL so the Monitoring task can report kernel health without touching the kernel.
class KernelMonitor: public IKernelHAL {
public:
	uint32_t heapFreeBytes() const override;
	uint32_t heapMinFreeBytes() const override;
	uint8_t tasks(RtosTaskInfo *out, uint8_t maxTasks) const override;
};

#endif // APP_DRIVERS_KERNELMONITOR_H
