# Architecture — Sensors dans SensorManager

```mermaid
classDiagram
    direction TB

    %% ── Interfaces ──────────────────────────────────────────────────────────
    class ISensor {
        <<interface>>
        +id() uint8_t
        +name() const char*
        +read() float
        +isAlarm() bool
    }

    class IAnalogSource {
        <<interface>>
        +read(ch uint8_t) float
    }

    class IAdcGroup {
        <<interface>>
        +bind()
        +trigger()
        +wait()
    }

    class IBus {
        <<interface>>
        +publish(topic, payload)
    }

    %% ── AdcSequencer (base class) ────────────────────────────────────────────
    class AdcSequencer {
        <<abstract>>
        -ADC_HandleTypeDef* hadc
        -uint32_t doneFlag
        -uint32_t* channels
        -uint8_t maxChannel
        -uint16_t* rawValues
        -TaskHandle_t notifyThreadId
        -uint8_t conversionIndex
        +bind()
        +trigger()
        +wait()
        +read(ch uint8_t) float*
        +onConversionComplete()
        +getInstance() ADC_HandleTypeDef*
    }

    IAnalogSource <|.. AdcSequencer
    IAdcGroup     <|.. AdcSequencer

    %% ── Drivers hardware (spécialisent AdcSequencer) ────────────────────────
    class InternalTemperature {
        -uint16_t rawBuf[3]
        +read(ch uint8_t) float
        __channels: PRIMARY_MOTOR__
        __         SECONDARY_MOTOR__
        __         POWER_SUPPLIES__
    }

    class MotorCurrentSense {
        -uint16_t rawBuf[4]
        +read(ch uint8_t) float
        __channels: PL / PR / SL / SR__
    }

    AdcSequencer <|-- InternalTemperature
    AdcSequencer <|-- MotorCurrentSense

    %% ── Sensor générique ────────────────────────────────────────────────────
    class AnalogSensor {
        -uint8_t id
        -const char* name
        -IAnalogSource* src
        -uint8_t channel
        -float alarmThreshold
        -float lastValue
        +id() uint8_t
        +name() const char*
        +read() float
        +isAlarm() bool
    }

    ISensor       <|.. AnalogSensor
    AnalogSensor  ..>  IAnalogSource : uses

    %% ── Tâche FreeRTOS ───────────────────────────────────────────────────────
    class SensorManager {
        -ISensor** sensors
        -uint8_t sensorCount
        -IAdcGroup** adcGroups
        -uint8_t adcGroupCount
        -IBus* bus
        -TaskHandle_t motionPlannerHandle
        +pollOnce()
        +task(param)$ void
        __pollOnce()__
        1. bind all adcGroups
        2. trigger all adcGroups
        3. wait all adcGroups
        4. read + publish all sensors
    }

    SensorManager --> ISensor   : polls[]
    SensorManager --> IAdcGroup : trigger[] wait[]
    SensorManager --> IBus      : publish

    %% ── Mocks (tests) ────────────────────────────────────────────────────────
    class MockSensor {
        <<test mock>>
        +value, alarm configurables
    }
    class MockAnalogSource {
        <<test mock>>
        +values[4] configurables
    }
    class MockAdcGroup {
        <<test mock>>
        +triggerCallCount
        +waitCallCount
    }

    ISensor       <|.. MockSensor
    IAnalogSource <|.. MockAnalogSource
    IAdcGroup     <|.. MockAdcGroup
```

## Flux ISR (callbacks ADC)

```mermaid
sequenceDiagram
    participant SM as SensorManager::pollOnce()
    participant IT as InternalTemperature (hadc3)
    participant MC as MotorCurrentSense (hadc1)
    participant ISR as HAL_ADC_ConvCpltCallback

    SM->>IT: bind() — enregistre la tâche FreeRTOS
    SM->>MC: bind()

    SM->>IT: trigger() — démarre ch0
    SM->>MC: trigger() — démarre ch0
    note over IT,MC: Les deux ADC tournent en parallèle

    ISR->>IT: onConversionComplete() ch0 → ch1 → ch2
    IT-->>SM: xTaskNotifyFromISR(flag=0x01)
    ISR->>MC: onConversionComplete() ch0 → ch1 → ch2 → ch3
    MC-->>SM: xTaskNotifyFromISR(flag=0x02)

    SM->>IT: wait() → xTaskNotifyWait(flag=0x01)
    SM->>MC: wait() → xTaskNotifyWait(flag=0x02)
    note over SM: Lit les sensors via AnalogSensor.read()<br/>qui appelle IAnalogSource.read(channel)
```

## Relation IAnalogSource / ISensor

Un même `AdcSequencer` (ex: `InternalTemperature`) est injecté dans plusieurs `AnalogSensor`, chacun lisant un canal différent :

```
InternalTemperature (IAnalogSource)
    ├── AnalogSensor(id=4, "TEMP_PRI",  src, ch=0)  →  ISensor
    ├── AnalogSensor(id=5, "TEMP_SEC",  src, ch=1)  →  ISensor
    └── AnalogSensor(id=6, "TEMP_PWR",  src, ch=2)  →  ISensor

MotorCurrentSense (IAnalogSource)
    ├── AnalogSensor(id=7,  "CUR_PL", src, ch=0)   →  ISensor
    ├── AnalogSensor(id=8,  "CUR_PR", src, ch=1)   →  ISensor
    ├── AnalogSensor(id=9,  "CUR_SL", src, ch=2)   →  ISensor
    └── AnalogSensor(id=10, "CUR_SR", src, ch=3)   →  ISensor
```
