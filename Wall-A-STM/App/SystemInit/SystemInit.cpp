#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "SystemInit.h"

#include "Tasks/MotionPlanner.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Tasks/ActuatorManager.h"
#include "Tasks/Monitoring.h"

#include "Drivers/Drv8262.h"
#include "Drivers/UartChannel.h"
#include "Drivers/UsbCdcChannel.h"
#include "Drivers/Encoder.h"
#include "Drivers/Stubs/ProximitySensor.h"
#include "Drivers/Stubs/TemperatureSensor.h"
#include "Drivers/Stubs/CurrentSensor.h"
#include "Drivers/Stubs/Pump.h"
#include "Drivers/Stubs/Servo.h"
#include "Drivers/Stubs/LinearTransducer.h"

#include "Interfaces/IActuatorHAL.h"
#include "Interfaces/IEncoderHAL.h"
#include "Interfaces/IMotorHAL.h"

#include "Services/Motor.h"
#include "Services/Odometry.h"

#include "Config.h"
#include "main.h"

#include "cmsis_os2.h"

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim8;

// Static snapshot storage
ExternalComm::CommSnapshot ExternalComm::latestSnapshot { };

// Stub HAL wrappers - replace with concrete drivers when hardware is wired
namespace {
struct StubSensorHAL: public ISensorHAL {
	float read() override {
		return 0.0f;
	}
};

struct StubActuatorHAL: public IActuatorHAL {
	void set(float) override {
	}
};

} // namespace

static UartChannel uartCh { &huart1 };
static UsbCdcChannel usbCh { };

static Encoder encL { &htim4 }, encR { &htim8 };
static Odometry odomHAL { &encL, &encR };

static Drv8262 drv { };
static Motor motorHAL { &drv };

static QueueHandle_t cmdMailbox = xQueueCreate(1, sizeof(MoveCmd));
static QueueHandle_t setpointMailbox = xQueueCreate(1, sizeof(Setpoint));

static StubSensorHAL stubProxHAL, stubTempHAL, stubCurrentHAL;
static ProximitySensor proxSensor { 1, &stubProxHAL };
static TemperatureSensor tempSensor { 2, &stubTempHAL };
static CurrentSensor currentSensor { 3, &stubCurrentHAL };

//static ISensor *sensors[Config::MAX_SENSORS] = { &proxSensor, &tempSensor, &currentSensor };
//static uint8_t sensorCount = 3;

static StubActuatorHAL stubPumpHAL, stubServoHAL, stubTransducerHAL;
static Pump pump { 1, &stubPumpHAL };
static Servo servo { 2, &stubServoHAL };
static LinearTransducer linearTransducer { 3, &stubTransducerHAL };

static IActuator *actuators[Config::MAX_ACTUATORS] = { &pump, &servo, &linearTransducer };
static uint8_t actuatorCount = 3;
static ActuatorManager actuatorMgr { actuators, actuatorCount, nullptr };
static ExternalComm extComm { &uartCh, &usbCh, nullptr, &actuatorMgr, cmdMailbox };
static Monitoring monitoring { &extComm };
static OdoControl odoCtrl { &odomHAL, &motorHAL, &extComm, setpointMailbox };
static MotionPlanner motionPlanner { &extComm, cmdMailbox, setpointMailbox };

TaskHandle_t motionPlannerHandle = nullptr;

static uint8_t uartIsrBuf[1];

#include "stm32f4xx_hal.h"
static void blinkTaskFn(void*) {
	for (;;) {
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void createTask(TaskFunction_t fn, const char *name, uint16_t stack, void *arg,
	UBaseType_t prio, TaskHandle_t *handle) {
	if (xTaskCreate(fn, name, stack, arg, prio, handle) != pdPASS)
		ExternalComm::log_error("Task '%s' failed to start (heap?)", name);
}

void enable(bool en) {
    GPIO_PinState state = en ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(ENABLE_POWER_SUPPLIES_GPIO_Port, ENABLE_POWER_SUPPLIES_Pin, state);
}

void SystemInit::boot() {
	SensorManager::latestSnapshot = { };
	OdoControl::latestSnapshot = { };
	actuatorMgr.setBus(&extComm);

	createTask(ExternalComm::rxTask, "ExtRX", Config::STACK_EXTCOMM_RX, &extComm,
		Config::PRIO_EXTCOMM_RX, nullptr);
	createTask(ExternalComm::txTask, "ExtTX", Config::STACK_EXTCOMM_TX, &extComm,
		Config::PRIO_EXTCOMM_TX, nullptr);
	createTask(OdoControl::task, "OdoCtrl", Config::STACK_ODO_CONTROL, &odoCtrl,
		Config::PRIO_ODO_CONTROL, nullptr);
	createTask(MotionPlanner::task, "MoPlan", Config::STACK_MOTION_PLANNER, &motionPlanner,
		Config::PRIO_MOTION_PLANNER, &motionPlannerHandle);

//	MotionPlanner::handle = motionPlannerHandle;

//	static SensorManager sensorManager { sensors, sensorCount, MotionPlanner::handle, &extComm };
//	xTaskCreate(SensorManager::task, "SensorMgr", Config::STACK_SENSOR_MANAGER, &sensorManager,
//		Config::PRIO_SENSOR_MANAGER, nullptr);
//	xTaskCreate(Monitoring::task, "Monitor", Config::STACK_MONITORING, &monitoring,
//		Config::PRIO_MONITORING, nullptr);

	HAL_UART_Receive_IT(&huart1, uartIsrBuf, 1);

	createTask(blinkTaskFn, "Blink", configMINIMAL_STACK_SIZE, nullptr, 1, nullptr);

	ExternalComm::log_info("Program start");


	enable(true);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == uartCh.getInstance()) {
		BaseType_t woken = pdFALSE;
		xQueueSendFromISR(extComm.rxByteQueue(), uartIsrBuf, &woken);
		HAL_UART_Receive_IT(huart, uartIsrBuf, 1);
		portYIELD_FROM_ISR(woken);
	}
}
