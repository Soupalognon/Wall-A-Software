# Architecture — InternalTemperature2

```mermaid
classDiagram
    direction TB

    %% ── Interfaces ──────────────────────────────────────────────────────────
    class ISensor2 {
        <<interface>>
        +id() uint8_t
        +name() const char*
        +read() float
        +isAlarm() bool
        +bind()
        +trigger()
        +doneFlag() uint32_t
    }

    %% ── Driver Adc (base class) ─────────────────────────────────────────────
    class Adc {
        <<abstract>>
        #_doneFlag uint32_t
        #_notifyThreadId TaskHandle_t
        -_hadc ADC_HandleTypeDef*
        -_sConfig ADC_ChannelConfTypeDef
        -_rawValue uint16_t
        -_isActive bool
        +getInstance() ADC_HandleTypeDef*
        +onConversionComplete()
        +isActive() bool
        #Adc(hadc, channel, doneFlag)
        #rawValue() uint16_t
        #start()
    }

    %% ── InternalTemperature2 ────────────────────────────────────────────────
    class InternalTemperature2 {
        +channelEnum PRIMARY_MOTOR=0 / SECONDARY_MOTOR=1 / POWER_SUPPLIES=2
        -_rawBuf uint16_t[3]
        -_id uint8_t
        -_name const char*
        -_alarmThreshold float
        -_lastValue float
        -_periodWindowMs uint32_t
        -_riseTime uint32_t
        -_wasAbove bool
        -RESISTANCE_REFERENCE = 10000
        -B_REFERENCE = 3434.0f
        -TEMPERATURE_REFERENCE = 25
        +id() uint8_t
        +name() const char*
        +read() float
        +isAlarm() bool
        +bind()
        +trigger()
        +doneFlag() uint32_t
        -voltageToCelsius(adcVal uint16_t) float
    }

    Adc       <|-- InternalTemperature2
    ISensor2  <|.. InternalTemperature2

    %% ── Dépendances externes ─────────────────────────────────────────────────
    class HAL {
        <<extern>>
        +ADC_ConfigChannel()
        +ADC_Start_IT()
        +ADC_GetValue() uint32_t
        +GetTick() uint32_t
    }

    class FreeRTOS {
        <<extern>>
        +xTaskGetCurrentTaskHandle() TaskHandle_t
        +xTaskNotifyFromISR(id, flag, eSetBits, &xHPTW)
        +portYIELD_FROM_ISR(xHPTW)
    }

    class math {
        <<extern stdlib>>
        +logf(x) float
    }

    Adc               ..> HAL      : ConfigChannel / Start_IT / GetValue
    InternalTemperature2 ..> HAL   : GetTick (isAlarm)
    InternalTemperature2 ..> FreeRTOS : xTaskGetCurrentTaskHandle (bind)\nxTaskNotifyFromISR (onConversionComplete)
    InternalTemperature2 ..> math  : logf (voltageToCelsius)
```

## Flux acquisition → lecture

```mermaid
sequenceDiagram
    participant C  as Appelant (SensorManager ou task)
    participant IT as InternalTemperature2
    participant A  as Adc (base)
    participant H  as HAL / ISR

    C->>IT: bind()
    IT->>A: _notifyThreadId = xTaskGetCurrentTaskHandle()

    C->>IT: trigger()
    IT->>A: start()
    A->>H: HAL_ADC_ConfigChannel() + HAL_ADC_Start_IT()

    H-->>A: HAL_ADC_ConvCpltCallback → onConversionComplete()
    A->>A: _rawValue = HAL_ADC_GetValue()
    A->>C: xTaskNotifyFromISR(_doneFlag)

    C->>IT: read()
    IT->>A: rawValue()
    A-->>IT: _rawValue (uint16_t)
    IT->>IT: voltageToCelsius(_rawValue) → °C
    IT-->>C: float (température en °C)
```

## Alarme — deux modes

| `periodWindowMs` | Comportement `isAlarm()` |
|---|---|
| `0` (défaut) | Instantané : `_lastValue > _alarmThreshold` |
| `> 0` | Temporel : alarme si `_wasAbove == true` **et** `HAL_GetTick() - _riseTime >= _periodWindowMs` |

> **Note :** contrairement à `AnalogSensor` (architecture v1), `InternalTemperature2` fusionne le driver ADC et la logique capteur dans une seule classe via double héritage `Adc` + `ISensor2`. Il n'y a pas d'intermédiaire `IAnalogSource`.

## Conversion NTC (voltageToCelsius)

```
adcVal (12 bits, 0–4095)
  └─► voltage = adcVal / 4096.0 × 3.3 V
        └─► R_ntc = voltage × R_ref / (3.3 − voltage)      (pont diviseur)
              └─► T_K = 1 / (1/T_ref_K + ln(R_ntc/R_ref) / B)   (équation Steinhart–Hart simplifiée)
                    └─► T_°C = T_K − 273.15
```

| Constante | Valeur |
|---|---|
| `RESISTANCE_REFERENCE` | 10 000 Ω |
| `B_REFERENCE` | 3434 K |
| `TEMPERATURE_REFERENCE` | 25 °C |
