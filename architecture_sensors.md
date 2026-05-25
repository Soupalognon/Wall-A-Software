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

    class ISensorSource {
        <<interface>>
        +bind()
        +trigger()
        +doneFlag() uint32_t
    }

    class IInputCaptureHAL {
        <<interface>>
    }

    class IBus {
        <<interface>>
        +publish(topic, payload)
    }

    %% ── SensorManager ───────────────────────────────────────────────────────
    class SensorManager {
        +latestSnapshot$ SensorSnapshot
        +sensorNames$[] const char*
        -SensorGroup* _groups
        -uint8_t _groupCount
        -TaskHandle_t _motionPlannerHandle
        -IBus* _bus
        +task(param)$ void
        +pollDueGroups()
    }

    class SensorGroup {
        <<struct>>
        +ISensorSource* source
        +ISensor** sensors
        +uint8_t sensorCount
        +uint32_t periodMs
        +uint32_t nextDueMs
    }

    class SensorSnapshot {
        <<struct>>
        +values[MAX_SENSORS] float
        +timestamps[MAX_SENSORS] uint32_t
        +alarmMask uint32_t
        +count uint8_t
    }

    SensorManager *-- SensorGroup     : _groups[]
    SensorManager *-- SensorSnapshot  : latestSnapshot
    SensorManager --> IBus            : _bus
    SensorGroup   --> ISensorSource   : source
    SensorGroup   --> ISensor         : sensors[]

    %% ── AdcSequencer (base class) ────────────────────────────────────────────
    class AdcSequencer {
        <<abstract>>
        -ADC_HandleTypeDef* _hadc
        -uint32_t _doneFlag
        -uint32_t* _channels
        -uint8_t _maxChannel
        -uint16_t* _rawValues
        -TaskHandle_t _notifyThreadId
        +bind()
        +trigger()
        +doneFlag() uint32_t
        +read(ch uint8_t) float
        +onConversionComplete()
    }

    IAnalogSource <|.. AdcSequencer
    ISensorSource <|.. AdcSequencer

    %% ── Drivers ADC ─────────────────────────────────────────────────────────
    class InternalTemperature {
        -uint16_t rawBuf[3]
        +read(ch uint8_t) float
        __channels: TEMP_PRI / SEC / PWR__
    }

    class MotorCurrentSense {
        -uint16_t rawBuf[4]
        +read(ch uint8_t) float
        __channels: CUR_PL / PR / SL / SR__
    }

    AdcSequencer <|-- InternalTemperature
    AdcSequencer <|-- MotorCurrentSense

    %% ── Driver InputCapture ──────────────────────────────────────────────────
    class ProximeterPololu5472 {
        +read(ch uint8_t) float
        +trigger()
        +doneFlag() uint32_t
        __channels: CH1..CH4 (largeur µs)__
    }

    IAnalogSource <|.. ProximeterPololu5472
    ISensorSource <|.. ProximeterPololu5472
    ProximeterPololu5472 --> IInputCaptureHAL : _ic

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
        +id() uint8_t
        +name() const char*
        +read() float
        +isAlarm() bool
    }

    ISensor      <|.. AnalogSensor
    AnalogSensor --> IAnalogSource : _src

    %% ── Dépendances externes ─────────────────────────────────────────────────
    class ExternalComm {
        <<extern>>
        +log_info(msg)$
    }

    class HAL {
        <<extern>>
        +GetTick() uint32_t
    }

    class FreeRTOS {
        <<extern>>
        +xTaskNotifyWait()
        +vTaskDelay()
    }

    SensorManager ..> ExternalComm : log_info
    SensorManager ..> HAL          : GetTick
    SensorManager ..> FreeRTOS     : notify / delay

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
        ProximityCH1..4 = 7..10
        PololuProxCH1..4 = 11..14
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
    class MockSensorSource {
        <<test mock>>
        +doneFlag() uint32_t
        +triggerCallCount
    }

    ISensor       <|.. MockSensor
    IAnalogSource <|.. MockAnalogSource
    ISensorSource <|.. MockSensorSource
```

## Flux ISR / polling (pollDueGroups)

```mermaid
sequenceDiagram
    participant SM as SensorManager::pollDueGroups()
    participant G  as SensorGroup (IT ou MC)
    participant S  as ISensorSource (AdcSequencer)
    participant ISR as HAL_ADC_ConvCpltCallback

    Note over SM: bind() appelé une seule fois dans task() au démarrage

    SM->>G: nextDueMs <= now ?
    G-->>SM: oui
    SM->>S: trigger() — démarre ch0
    Note over SM: flag = source.doneFlag()

    ISR->>S: onConversionComplete() ch0 → ch1 → ... → chN
    S-->>SM: xTaskNotifyFromISR(flag)

    loop flag != 0
        SM->>SM: xTaskNotifyWait(0, flag, &bits, MAX_DELAY)
        SM->>SM: flag &= ~bits
    end

    SM->>SM: lire tous les ISensor du groupe → latestSnapshot
    SM->>SM: nextDueMs += periodMs
```

> **Note :** `ProximeterPololu5472` retourne `doneFlag() == 0` (ISR InputCapture alimente les données en continu) — l'attente `xTaskNotifyWait` est donc ignorée pour ce groupe.

## Alarme `AnalogSensor` — deux modes

| `periodWindowMs` | Comportement |
|---|---|
| `0` (défaut) | Instantané : alarme si `lastValue > alarmThreshold` |
| `> 0` | Périodique : alarme si ≥ `periodMinCount` samples au-dessus du seuil dans la fenêtre `periodWindowMs` ms (ring buffer de 16 entrées) |

## Relation IAnalogSource / ISensor

Un même driver (ex: `InternalTemperature`) est injecté dans plusieurs `AnalogSensor`, chacun lisant un canal différent via `IAnalogSource::read(ch)` :

```
InternalTemperature (ISensorSource + IAnalogSource)
    ├── AnalogSensor(id=0, "TEMP_PRI",  src, ch=0)  →  ISensor  [SensorType::PrimaryMotorTemp]
    ├── AnalogSensor(id=1, "TEMP_SEC",  src, ch=1)  →  ISensor  [SensorType::SecondaryMotorTemp]
    └── AnalogSensor(id=2, "TEMP_PWR",  src, ch=2)  →  ISensor  [SensorType::PowerSupplyTemp]

MotorCurrentSense (ISensorSource + IAnalogSource)
    ├── AnalogSensor(id=3, "CUR_PL", src, ch=0)  →  ISensor  [SensorType::PrimaryMotorCurrentL]
    ├── AnalogSensor(id=4, "CUR_PR", src, ch=1)  →  ISensor  [SensorType::PrimaryMotorCurrentR]
    ├── AnalogSensor(id=5, "CUR_SL", src, ch=2)  →  ISensor  [SensorType::SecondaryMotorCurrentL]
    └── AnalogSensor(id=6, "CUR_SR", src, ch=3)  →  ISensor  [SensorType::SecondaryMotorCurrentR]

ProximeterPololu5472 (ISensorSource + IAnalogSource)
    ├── AnalogSensor(id=11, "PROX_P1", src, ch=0)  →  ISensor  [SensorType::PololuProxCH1]
    ├── AnalogSensor(id=12, "PROX_P2", src, ch=1)  →  ISensor  [SensorType::PololuProxCH2]
    ├── AnalogSensor(id=13, "PROX_P3", src, ch=2)  →  ISensor  [SensorType::PololuProxCH3]
    └── AnalogSensor(id=14, "PROX_P4", src, ch=3)  →  ISensor  [SensorType::PololuProxCH4]
```

> **Note :** Les IDs numériques correspondent aux constantes `SensorType::*` et à l'index dans `SensorSnapshot::values[]`.
