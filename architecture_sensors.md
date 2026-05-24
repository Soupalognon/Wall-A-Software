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
        +doneFlag() uint32_t
    }

    class IBus {
        <<interface>>
        +publish(topic, payload)
    }

    %% ── AdcSequencer (base class) ────────────────────────────────────────────
    class AdcSequencer {
        <<abstract>>
        -ADC_HandleTypeDef* _hadc
        -uint32_t _doneFlag
        -uint32_t* _channels
        -uint8_t _maxChannel
        -uint16_t* _rawValues
        -TaskHandle_t _notifyThreadId
        -uint8_t _conversionIndex
        #rawValues() uint16_t*
        +bind()
        +trigger()
        +doneFlag() uint32_t
        +getInstance() ADC_HandleTypeDef*
        +onConversionComplete()
        -configureAndStart(index uint8_t)
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
        +MAX_PERIOD_SAMPLES$ uint8_t = 16
        -uint8_t _id
        -const char* _name
        -IAnalogSource* _src
        -uint8_t _channel
        -float _alarmThreshold
        -float _lastValue
        -uint32_t _periodWindowMs
        -uint8_t _periodMinCount
        -uint32_t _timestamps[16]
        -uint8_t _tsHead
        -uint8_t _tsCount
        +id() uint8_t
        +name() const char*
        +read() float
        +isAlarm() bool
    }

    ISensor       <|.. AnalogSensor
    AnalogSensor  ..>  IAnalogSource : uses

    %% ── Tâche FreeRTOS ───────────────────────────────────────────────────────
    class SensorManager {
        +latestSnapshot$ SensorSnapshot
        +sensorNames$[] const char*
        -ISensor** _sensors
        -uint8_t _sensorCount
        -IAdcGroup** _adcGroups
        -uint8_t _adcGroupCount
        -IBus* _bus
        -TaskHandle_t _motionPlannerHandle
        +pollOnce()
        +task(param)$ void
        __pollOnce()__
        1. trigger all adcGroups
        2. OR all doneFlags → allFlags
        3. xTaskNotifyWait loop until remaining==0
        4. read + snapshot all sensors
    }

    class SensorSnapshot {
        +values[] float
        +alarmMask uint32_t
        +count uint8_t
        +timestamp uint32_t
    }

    SensorManager --> ISensor        : polls[]
    SensorManager --> IAdcGroup      : trigger[] doneFlag[]
    SensorManager --> IBus           : (réservé)
    SensorManager *-- SensorSnapshot : latestSnapshot

    %% ── Namespace SensorType ─────────────────────────────────────────────────
    class SensorType {
        <<namespace>>
        PrimaryMotorTemp = 0
        SecondaryMotorTemp = 1
        PowerSupplyTemp = 2
        PrimaryMotorCurrentL = 3
        PrimaryMotorCurrentR = 4
        SecondaryMotorCurrentL = 5
        SecondaryMotorCurrentR = 6
    }

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
        +doneFlag() uint32_t
        +triggerCallCount
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

    Note over SM: bind() appelé une seule fois dans task() au démarrage

    SM->>IT: trigger() — démarre ch0
    SM->>MC: trigger() — démarre ch0
    Note over SM: allFlags = IT.doneFlag() | MC.doneFlag()
    Note over IT,MC: Les deux ADC tournent en parallèle

    ISR->>IT: onConversionComplete() ch0 → ch1 → ch2
    IT-->>SM: xTaskNotifyFromISR(flag=0x01)
    ISR->>MC: onConversionComplete() ch0 → ch1 → ch2 → ch3
    MC-->>SM: xTaskNotifyFromISR(flag=0x02)

    loop remaining != 0
        SM->>SM: xTaskNotifyWait(0, remaining, &bits, MAX_DELAY)
        SM->>SM: remaining &= ~bits
    end

    Note over SM: Lit les sensors via AnalogSensor.read()<br/>qui appelle IAnalogSource.read(channel)<br/>Résultat stocké dans latestSnapshot
```

## Alarme `AnalogSensor` — deux modes

| `periodWindowMs` | Comportement |
|---|---|
| `0` (défaut) | Instantané : alarme si `lastValue > alarmThreshold` |
| `> 0` | Périodique : alarme si ≥ `periodMinCount` samples au-dessus du seuil dans la fenêtre `periodWindowMs` ms (ring buffer de 16 entrées) |

## Relation IAnalogSource / ISensor

Un même `AdcSequencer` (ex: `InternalTemperature`) est injecté dans plusieurs `AnalogSensor`, chacun lisant un canal différent :

```
InternalTemperature (IAnalogSource)
    ├── AnalogSensor(id=0, "TEMP_PRI",  src, ch=0)  →  ISensor  [SensorType::PrimaryMotorTemp]
    ├── AnalogSensor(id=1, "TEMP_SEC",  src, ch=1)  →  ISensor  [SensorType::SecondaryMotorTemp]
    └── AnalogSensor(id=2, "TEMP_PWR",  src, ch=2)  →  ISensor  [SensorType::PowerSupplyTemp]

MotorCurrentSense (IAnalogSource)
    ├── AnalogSensor(id=3, "CUR_PL", src, ch=0)  →  ISensor  [SensorType::PrimaryMotorCurrentL]
    ├── AnalogSensor(id=4, "CUR_PR", src, ch=1)  →  ISensor  [SensorType::PrimaryMotorCurrentR]
    ├── AnalogSensor(id=5, "CUR_SL", src, ch=2)  →  ISensor  [SensorType::SecondaryMotorCurrentL]
    └── AnalogSensor(id=6, "CUR_SR", src, ch=3)  →  ISensor  [SensorType::SecondaryMotorCurrentR]
```

> **Note :** Les IDs numériques ci-dessus correspondent aux constantes `SensorType::*` et à l'index dans `SensorSnapshot::values[]`.
