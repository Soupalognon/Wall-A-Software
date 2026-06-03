#ifndef APP_TASKS_MONITORING_H
#define APP_TASKS_MONITORING_H

#include "Tasks/OdoControl.h"
#include "Tasks/ExternalComm.h"
#include "Interfaces/IBus.h"
#include "Interfaces/IKernelHAL.h"
#include "Services/BusFormat.h"
#include "Config.h"
#include <FreeRTOS.h>
#include <task.h>
#include "stm32f4xx_hal.h"

class Monitoring {
public:
	Monitoring(IBus *bus, IKernelHAL *kernel);
	static void task(void *param);
	void checkOnce();

private:
	void checkRtos(uint32_t now);

	IBus *_bus;
	IKernelHAL *_kernel;
	uint32_t _lastSensorTs[Config::MAX_SENSORS] = { };
	uint8_t _rtosDivCounter = 0;

	// Heap free fluctuates, so the low-heap alert re-arms once it recovers above threshold.
	bool _heapAlarmed = false;
};

#endif // APP_TASKS_MONITORING_H
