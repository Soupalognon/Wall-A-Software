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

#include "Services/Odometry.h"
#include "Services/ActuatorManager.h"
#include "Services/InternalTemperature.h"
#include "Services/MotorCurrentSense.h"
#include "Services/B5WLB2101.h"
#include "Services/AnalogSensor.h"
#include "Services/ProximeterPololu5472.h"

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

static MotorCurrentSense motorsCurrentSense { &hadc1 };
static B5WLB2101 proximity { &hadc2 };
static InternalTemperature internalTemperatures { &hadc3 };

static Drv8262 drv { &htim1 };
static InputCapture inputCapture { &htim3 };
static ProximeterPololu5472 pololuProximity { &inputCapture };

static QueueHandle_t cmdMailbox = xQueueCreate(1, sizeof(MoveCmd));
static QueueHandle_t setpointMailbox = xQueueCreate(1, sizeof(Setpoint));

// ── Sensors InternalTemperature (hadc3 — 3 canaux) ───────────────────────────
static AnalogSensor intTempPri { SensorType::PrimaryMotorTemp, "TEMP_PRI", &internalTemperatures,
	InternalTemperature::PRIMARY_MOTOR, Config::TEMP_ALARM_C };
static AnalogSensor intTempSec { SensorType::SecondaryMotorTemp, "TEMP_SEC", &internalTemperatures,
	InternalTemperature::SECONDARY_MOTOR, Config::TEMP_ALARM_C };
static AnalogSensor intTempPwr { SensorType::PowerSupplyTemp, "TEMP_PWR", &internalTemperatures,
	InternalTemperature::POWER_SUPPLIES, Config::TEMP_ALARM_C };

// ── Sensors MotorCurrentSense (hadc1 — 4 canaux) ─────────────────────────────
static AnalogSensor curPL { SensorType::PrimaryMotorCurrentL, "CUR_PRI_L", &motorsCurrentSense,
	MotorCurrentSense::PRIMARY_MOTOR_LEFT, Config::CURRENT_ALARM_MA };
static AnalogSensor curPR { SensorType::PrimaryMotorCurrentR, "CUR_PRI_R", &motorsCurrentSense,
	MotorCurrentSense::PRIMARY_MOTOR_RIGHT, Config::CURRENT_ALARM_MA };
static AnalogSensor curSL { SensorType::SecondaryMotorCurrentL, "CUR_SEC_L", &motorsCurrentSense,
	MotorCurrentSense::SECONDARY_MOTOR_LEFT, Config::CURRENT_ALARM_MA };
static AnalogSensor curSR { SensorType::SecondaryMotorCurrentR, "CUR_SEC_R", &motorsCurrentSense,
	MotorCurrentSense::SECONDARY_MOTOR_RIGHT, Config::CURRENT_ALARM_MA };

// ── Sensors B5WLB2101 (hadc2 — 4 canaux) ─────────────────────────────────────
static AnalogSensor proxCH1 { SensorType::ProximityCH1, "PROX_CH1", &proximity,
	B5WLB2101::CH_1, Config::PROXIMITY_ALARM_M };
static AnalogSensor proxCH2 { SensorType::ProximityCH2, "PROX_CH2", &proximity,
	B5WLB2101::CH_2, Config::PROXIMITY_ALARM_M };
static AnalogSensor proxCH3 { SensorType::ProximityCH3, "PROX_CH3", &proximity,
	B5WLB2101::CH_3, Config::PROXIMITY_ALARM_M };
static AnalogSensor proxCH4 { SensorType::ProximityCH4, "PROX_CH4", &proximity,
	B5WLB2101::CH_4, Config::PROXIMITY_ALARM_M };

// ── Sensors ProximeterPololu5472 (htim3 — 4 canaux InputCapture) ──────────────
static AnalogSensor polCH1 { SensorType::PololuProxCH1, "POL_CH1", &pololuProximity,
	ProximeterPololu5472::CH_1, Config::POLOLU5472_ALARM_US };
static AnalogSensor polCH2 { SensorType::PololuProxCH2, "POL_CH2", &pololuProximity,
	ProximeterPololu5472::CH_2, Config::POLOLU5472_ALARM_US };
static AnalogSensor polCH3 { SensorType::PololuProxCH3, "POL_CH3", &pololuProximity,
	ProximeterPololu5472::CH_3, Config::POLOLU5472_ALARM_US };
static AnalogSensor polCH4 { SensorType::PololuProxCH4, "POL_CH4", &pololuProximity,
	ProximeterPololu5472::CH_4, Config::POLOLU5472_ALARM_US };

// ── Sensor groups (chacun à sa propre fréquence) ─────────────────────────────
static ISensor *tempSensors[] = { &intTempPri, &intTempSec, &intTempPwr };
static ISensor *currentSensors[] = { &curPL, &curPR, &curSL, &curSR };
//static ISensor *proximitySensors[] = { &proxCH1, &proxCH2, &proxCH3, &proxCH4 };
static ISensor *pololuSensors[] = { &polCH1, &polCH2, &polCH3, &polCH4 };

static SensorManager::SensorGroup sensorGroups[] = {
	{ &internalTemperatures, tempSensors,    std::size(tempSensors),    1000 / Config::TEMP_SENSOR_FREQ_HZ,    0 },
	{ &motorsCurrentSense,   currentSensors, std::size(currentSensors), 1000 / Config::CURRENT_SENSOR_FREQ_HZ, 0 },
//	{ &proximity,            proximitySensors, std::size(proximitySensors), 1000 / Config::B5W_SENSOR_FREQ_HZ, 0 },
	{ &pololuProximity,      pololuSensors,  std::size(pololuSensors),  1000 / Config::POLOLU5472_SENSOR_FREQ_HZ, 0 },
};

// ── Actuators ─────────────────────────────────────────────────────────────────
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
	inputCapture.init();

	createTask(ExternalComm::rxTask, "ExtRX", Config::STACK_EXTCOMM_RX, &extComm,
		Config::PRIO_EXTCOMM_RX, nullptr);
	createTask(ExternalComm::txTask, "ExtTX", Config::STACK_EXTCOMM_TX, &extComm,
		Config::PRIO_EXTCOMM_TX, nullptr);
	createTask(OdoControl::task, "OdoCtrl", Config::STACK_ODO_CONTROL, &odoCtrl,
		Config::PRIO_ODO_CONTROL, nullptr);
	createTask(MotionPlanner::task, "MoPlan", Config::STACK_MOTION_PLANNER, &motionPlanner,
		Config::PRIO_MOTION_PLANNER, nullptr);

	static Monitoring monitoring { &extComm };
	createTask(Monitoring::task, "Monitor", Config::STACK_MONITORING, &monitoring,
		Config::PRIO_MONITORING, nullptr);

	static SensorManager sensorManager { sensorGroups, std::size(sensorGroups), MotionPlanner::handle, &extComm };
	createTask(SensorManager::task, "SensorMgr", Config::STACK_SENSOR_MANAGER, &sensorManager,
		Config::PRIO_SENSOR_MANAGER, nullptr);

	xTaskCreate(blinkTaskFn, "Blink", configMINIMAL_STACK_SIZE, nullptr, 1, nullptr);

	ExternalComm::log_info("Program start");

	enable(true);

	vTaskDelete(nullptr);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	if (hadc == internalTemperatures.getInstance())
		internalTemperatures.onConversionComplete();
	else if (hadc == motorsCurrentSense.getInstance())
		motorsCurrentSense.onConversionComplete();
	else if (hadc == proximity.getInstance())
		proximity.onConversionComplete();
}
