#include <Services/Motor.h>
#include <Services/Odometry.h>
#include "SystemInit.h"
#include "Tasks/MotionPlanner.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Tasks/StubActuatorManager.h"
#include "Drivers/UartChannel.h"
#include "Drivers/UsbCdcChannel.h"
#include "Interfaces/IEncoderHAL.h"
#include "Interfaces/IMotorHAL.h"
#include "Config.h"
#include "main.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

extern UART_HandleTypeDef huart1;

// Static snapshot storage
ExternalComm::CommSnapshot ExternalComm::latestSnapshot { };

// Stub HAL wrappers - replace with concrete drivers when hardware is wired
namespace {

struct StubEncoderHAL: public IEncoderHAL {
	int32_t getTicksLeft() override {
		return 0;
	}
	int32_t getTicksRight() override {
		return 0;
	}
};

struct StubMotorHAL: public IMotorHAL {
	void setMotors(float, float) override {
	}
};

} // namespace

static UartChannel uartCh { &huart1 };
static UsbCdcChannel usbCh { };

static StubEncoderHAL stubEncL { }, stubEncR { };
static ConcreteOdomHAL odomHAL { &stubEncL, &stubEncR };
static StubMotorHAL stubMotorHAL { };

static QueueHandle_t cmdMailbox = xQueueCreate(1, sizeof(MoveCmd));
static QueueHandle_t setpointMailbox = xQueueCreate(1, sizeof(Setpoint));

static StubActuatorManager stubActMgr { };
static ExternalComm extComm { &uartCh, &usbCh, nullptr, &stubActMgr, cmdMailbox };
static OdoControl odoCtrl { &odomHAL, &stubMotorHAL, &extComm, setpointMailbox };
static MotionPlanner motionPlanner { &extComm, cmdMailbox, setpointMailbox };

TaskHandle_t motionPlannerHandle = nullptr;

static uint8_t uartIsrBuf[1];

static void blinkTaskFn(void*) {
	for (;;) {
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void SystemInit::boot() {
	SensorManager::latestSnapshot = { };

	xTaskCreate(ExternalComm::rxTask, "ExtRX", Config::STACK_EXTCOMM_RX, &extComm,
		Config::PRIO_EXTCOMM_RX, nullptr);
	xTaskCreate(ExternalComm::txTask, "ExtTX", Config::STACK_EXTCOMM_TX, &extComm,
		Config::PRIO_EXTCOMM_TX, nullptr);
	xTaskCreate(OdoControl::task, "OdoCtrl", Config::STACK_ODO_CONTROL, &odoCtrl,
		Config::PRIO_ODO_CONTROL, nullptr);
	xTaskCreate(MotionPlanner::task, "MoPlan", Config::STACK_MOTION_PLANNER, &motionPlanner,
		Config::PRIO_MOTION_PLANNER, &motionPlannerHandle);

	MotionPlanner::handle = motionPlannerHandle;

	static ISensor* sensors[Config::MAX_SENSORS] = {};
	static uint8_t  sensorCount = 0;
	static SensorManager sensorManager{sensors, sensorCount, MotionPlanner::handle, &extComm};
	xTaskCreate(SensorManager::task, "SensorMgr",
		Config::STACK_SENSOR_MANAGER, &sensorManager,
		Config::PRIO_SENSOR_MANAGER, nullptr);

	HAL_UART_Receive_IT(&huart1, uartIsrBuf, 1);

	xTaskCreate(blinkTaskFn, "Blink", configMINIMAL_STACK_SIZE, nullptr, 1, nullptr);

	ExternalComm::log_info("Program start");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == uartCh.getInstance()) {
		BaseType_t woken = pdFALSE;
		xQueueSendFromISR(extComm.rxByteQueue(), uartIsrBuf, &woken);
		HAL_UART_Receive_IT(huart, uartIsrBuf, 1);
		portYIELD_FROM_ISR(woken);
	}
}
