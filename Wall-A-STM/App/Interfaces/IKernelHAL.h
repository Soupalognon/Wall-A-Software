#ifndef APP_INTERFACES_IKERNELHAL_H
#define APP_INTERFACES_IKERNELHAL_H

#include <cstdint>

// Plain snapshot of one task — keeps FreeRTOS types out of the App layer.
struct RtosTaskInfo {
	char name[16];           // configMAX_TASK_NAME_LEN
	uint16_t stackFreeWords; // uxTaskGetStackHighWaterMark (min free stack ever, in words)
};

class IKernelHAL {
public:
	virtual ~IKernelHAL() = default;

	virtual uint32_t heapFreeBytes() const = 0;    // xPortGetFreeHeapSize
	virtual uint32_t heapMinFreeBytes() const = 0; // xPortGetMinimumEverFreeHeapSize

	// Fills out[0..n-1] with one entry per live task; returns n (clamped to maxTasks).
	virtual uint8_t tasks(RtosTaskInfo *out, uint8_t maxTasks) const = 0;
};

#endif // APP_INTERFACES_IKERNELHAL_H
