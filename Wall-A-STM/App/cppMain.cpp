#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <iterator>

#include "Tasks/MotionPlanner.h"
#include "Tasks/OdoControl.h"
#include "Tasks/SensorManager.h"
#include "Tasks/ExternalComm.h"
#include "Tasks/Monitoring.h"

#include "Drivers/Drv8262.h"
#include "Drivers/UartChannel.h"
#include "Drivers/UsbCdcChannel.h"
#include "Drivers/Encoder.h"
#include "Drivers/InputCapture.h"
#include "Drivers/Adc.h"
#include "Drivers/KernelMonitor.h"

#include "Services/Odometry.h"
#include "Services/ActuatorManager.h"
#include "Services/Sensors/MotorCurrentSense.h"
#include "Services/Sensors/B5WLB2101.h"
#include "Services/Sensors/InternalTemperature.h"
#include "Services/Sensors/Pololu5472.h"

#include "Interfaces/IEncoderHAL.h"
#include "Interfaces/IMotorHAL.h"

#include "Config.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim10;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;

static UartChannel uartCh { &huart1 };
static UsbCdcChannel usbCh { &hUsbDeviceFS };

static Encoder encL { &htim4 }, encR { &htim8 };
static Odometry odomHAL { &encL, &encR };

static Drv8262 drv { &htim1 };

static QueueHandle_t cmdMailbox = xQueueCreate(1, sizeof(MoveCmd));
static QueueHandle_t setpointMailbox = xQueueCreate(1, sizeof(Setpoint));

// ---- Sensors InternalTemperature (hadc3 - channels 4/5/6) -------------------
static Adc adcTempPri { &hadc3, ADC_CHANNEL_4 };
static Adc adcTempSec { &hadc3, ADC_CHANNEL_5 };
static Adc adcTempPwr { &hadc3, ADC_CHANNEL_6 };
static InternalTemperature intTempPri { adcTempPri, SensorType::PrimaryMotorTemp, "TEMP_PRI",
	Config::TEMP_ALARM_C };
static InternalTemperature intTempSec { adcTempSec, SensorType::SecondaryMotorTemp, "TEMP_SEC",
	Config::TEMP_ALARM_C };
static InternalTemperature intTempPwr { adcTempPwr, SensorType::PowerSupplyTemp, "TEMP_PWR",
	Config::TEMP_ALARM_C };

// ---- Sensors MotorCurrentSense (hadc1 - channels 3/4/6/8) -------------------
static Adc adcCurPL { &hadc1, ADC_CHANNEL_3 };
static Adc adcCurPR { &hadc1, ADC_CHANNEL_4 };
static Adc adcCurSL { &hadc1, ADC_CHANNEL_6 };
static Adc adcCurSR { &hadc1, ADC_CHANNEL_8 };
static MotorCurrentSense curPL { adcCurPL, MotorCurrentSense::MotorType::PRIMARY,
	SensorType::PrimaryMotorCurrentL, "CUR_PRI_L", Config::CURRENT_ALARM_MA };
static MotorCurrentSense curPR { adcCurPR, MotorCurrentSense::MotorType::PRIMARY,
	SensorType::PrimaryMotorCurrentR, "CUR_PRI_R", Config::CURRENT_ALARM_MA };
static MotorCurrentSense curSL { adcCurSL, MotorCurrentSense::MotorType::SECONDARY,
	SensorType::SecondaryMotorCurrentL, "CUR_SEC_L", Config::CURRENT_ALARM_MA };
static MotorCurrentSense curSR { adcCurSR, MotorCurrentSense::MotorType::SECONDARY,
	SensorType::SecondaryMotorCurrentR, "CUR_SEC_R", Config::CURRENT_ALARM_MA };

// ---- Sensors B5WLB2101 (hadc2 - channels 12/10/13/9) ------------------------
static Adc adcProx1 { &hadc2, ADC_CHANNEL_12 };
static Adc adcProx2 { &hadc2, ADC_CHANNEL_10 };
static Adc adcProx3 { &hadc2, ADC_CHANNEL_13 };
static Adc adcProx4 { &hadc2, ADC_CHANNEL_9 };
static B5WLB2101 proxCH1 { adcProx1, SensorType::ProximityCH1, "PROX_CH1", Config::PROXIMITY_ALARM_M };
static B5WLB2101 proxCH2 { adcProx2, SensorType::ProximityCH2, "PROX_CH2", Config::PROXIMITY_ALARM_M };
static B5WLB2101 proxCH3 { adcProx3, SensorType::ProximityCH3, "PROX_CH3", Config::PROXIMITY_ALARM_M };
static B5WLB2101 proxCH4 { adcProx4, SensorType::ProximityCH4, "PROX_CH4", Config::PROXIMITY_ALARM_M };

// ---- Sensors ProximeterPololu5472 (htim3 - 4 InputCapture channels) ---------
static InputCapture icPol1 { &htim3, TIM_CHANNEL_1 };
static InputCapture icPol2 { &htim3, TIM_CHANNEL_3 };
static InputCapture icPol3 { &htim3, TIM_CHANNEL_2 };
static InputCapture icPol4 { &htim3, TIM_CHANNEL_4 };
static Pololu5472 polCH1 { icPol1, SensorType::PololuProxCH1, "POL_CH1", Config::POLOLU5472_ALARM_US };
static Pololu5472 polCH2 { icPol2, SensorType::PololuProxCH2, "POL_CH2", Config::POLOLU5472_ALARM_US };
static Pololu5472 polCH3 { icPol3, SensorType::PololuProxCH3, "POL_CH3", Config::POLOLU5472_ALARM_US };
static Pololu5472 polCH4 { icPol4, SensorType::PololuProxCH4, "POL_CH4", Config::POLOLU5472_ALARM_US };

// ---- Sensor groups (each at its own frequency) ------------------------------
static ISensor *tempSensors[] = { &intTempPri, &intTempSec, &intTempPwr };
static ISensor *currentSensors[] = { &curPL, &curPR, &curSL, &curSR };
static ISensor *proximitySensors[] = { &proxCH1, &proxCH2, &proxCH3, &proxCH4 };
static ISensor *pololuSensors[] = { &polCH1, &polCH2, &polCH3, &polCH4 };

//@formatter:off
static SensorManager::SensorGroup sensorGroups[] = {
	{ tempSensors,      std::size(tempSensors),      1000 / Config::SENSOR_TEMP_FREQ_HZ,    0 },
	{ currentSensors,   std::size(currentSensors),   1000 / Config::SENSOR_CURRENT_FREQ_HZ, 0 },
	{ proximitySensors, std::size(proximitySensors), 1000 / Config::SENSOR_B5W_FREQ_HZ,     0 },
	{ pololuSensors,    std::size(pololuSensors),    1000 / Config::SENSOR_POLOLU5472_FREQ_HZ, 0 },
};
//@formatter:on

// ---- Actuators --------------------------------------------------------------
static ActuatorManager actuatorMgr { nullptr, 0, nullptr };

static ExternalComm extComm { &uartCh, &usbCh, nullptr, &actuatorMgr, cmdMailbox };
static OdoControl odoCtrl { &odomHAL, &drv, &extComm, setpointMailbox };
static MotionPlanner motionPlanner { &extComm, cmdMailbox, setpointMailbox };

static void blinkTaskFn(void*) {
	for (;;) {
		HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
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

extern "C" void cppMain(void) {
	actuatorMgr.setBus(&extComm); // circular dep resolution: actuatorMgr constructed before extComm
	InputCapture::initAll();

	createTask(ExternalComm::rxTask, "ExtRX", Config::STACK_EXTCOMM_RX, &extComm,
		Config::PRIO_EXTCOMM_RX, nullptr);
	createTask(ExternalComm::txTask, "ExtTX", Config::STACK_EXTCOMM_TX, &extComm,
		Config::PRIO_EXTCOMM_TX, nullptr);
	createTask(OdoControl::task, "OdoCtrl", Config::STACK_ODO_CONTROL, &odoCtrl,
		Config::PRIO_ODO_CONTROL, nullptr);
	createTask(MotionPlanner::task, "MoPlan", Config::STACK_MOTION_PLANNER, &motionPlanner,
		Config::PRIO_MOTION_PLANNER, nullptr);

	static KernelMonitor kernelMonitor;
	static Monitoring monitoring { &extComm, &kernelMonitor };
	createTask(Monitoring::task, "Monitor", Config::STACK_MONITORING, &monitoring,
		Config::PRIO_MONITORING, nullptr);

	static SensorManager sensorManager { sensorGroups, std::size(sensorGroups),
		MotionPlanner::handle, &extComm };
	createTask(SensorManager::task, "SensorMgr", Config::STACK_SENSOR_MANAGER, &sensorManager,
		Config::PRIO_SENSOR_MANAGER, nullptr);

	xTaskCreate(blinkTaskFn, "Blink", configMINIMAL_STACK_SIZE, nullptr, 1, nullptr);

	ExternalComm::log_info("Program start");

	enable(true);

	vTaskDelete(nullptr);
}
