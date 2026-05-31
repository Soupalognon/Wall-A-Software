# Architecture — Capteurs ADC (Adc + ISensor)

Trois capteurs analogiques partagent la même base : le driver `Adc` (acquisition non-bloquante par interruption) et l'interface `ISensor` (contrat capteur). Chaque capteur fusionne les deux par double héritage `public Adc, public ISensor`.

| Capteur | Périphérique | Conversion | Unité |
|---|---|---|---|
| `InternalTemperature` | `hadc3` (3 canaux) | NTC → Steinhart–Hart | °C |
| `MotorCurrentSense` | `hadc1` (4 canaux) | pont shunt (primaire/secondaire) | mA |
| `B5WLB2101` | `hadc2` (4 canaux) | tension brute | V |
| `ProximeterPololu5472` | `htim3` InputCapture | *(planifié, commenté)* | µs |

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
        +bind()
        +trigger()
        +doneFlag() uint32_t
        +isActive() bool
    }

    class IAdcHAL {
        <<interface>>
        +start()
        +rawValue() uint16_t
        +isActive() bool
    }

    %% ── Driver Adc (base commune) ───────────────────────────────────────────
    class Adc {
        #_doneFlag uint32_t
        #_notifyThreadId TaskHandle_t
        -_hadc ADC_HandleTypeDef*
        -_sConfig ADC_ChannelConfTypeDef
        -_rawValue uint16_t
        -_isActive bool
        -s_head Adc*$
        -_next Adc*
        +getInstance() ADC_HandleTypeDef*
        +onConversionComplete()
        +isActive() bool
        +rawValue() uint16_t
        +start()
        +dispatchCallback(hadc)$
        #Adc(hadc, channel, doneFlag)
    }

    IAdcHAL <|.. Adc

    %% ── Capteurs concrets ───────────────────────────────────────────────────
    class InternalTemperature {
        +channelEnum PRIMARY_MOTOR / SECONDARY_MOTOR / POWER_SUPPLIES
        -_id / _name / _alarmThreshold
        -_lastValue / _periodWindowMs / _riseTime / _wasAbove
        -RESISTANCE_REFERENCE = 10000
        -B_REFERENCE = 3434.0f
        -TEMPERATURE_REFERENCE = 25
        -voltageToCelsius(adcVal) float
    }

    class MotorCurrentSense {
        +channelEnum PRIMARY_MOTOR_LEFT/RIGHT / SECONDARY_MOTOR_LEFT/RIGHT
        -_isPrimaryMotor bool
        -PRI_RESISTANCE_REFERENCE / PRI_GAIN_FACTOR / SEC_RESISTANCE_REFERENCE
        -voltageToCurrentPrimary(adcVal) float
        -voltageToCurrentSecondary(adcVal) float
    }

    class B5WLB2101 {
        +channelEnum CH_1 / CH_2 / CH_3 / CH_4
        -_id / _name / _alarmThreshold
        -_lastValue / _periodWindowMs / _riseTime / _wasAbove
    }

    Adc      <|-- InternalTemperature
    ISensor  <|.. InternalTemperature
    Adc      <|-- MotorCurrentSense
    ISensor  <|.. MotorCurrentSense
    Adc      <|-- B5WLB2101
    ISensor  <|.. B5WLB2101

    %% ── Orchestration ───────────────────────────────────────────────────────
    class SensorManager {
        +SensorGroup : sensors, count, periodMs, nextDueMs
        +task(param)$
        +pollDueGroups()
    }
    SensorManager o-- ISensor : groupes (ISensor*[])
```

## Routage des interruptions — registre intrusif statique

Contrairement à la v1, le callback de fin de conversion n'est plus lié à une instance unique. Chaque `Adc` s'auto-enregistre dans une liste chaînée statique (`s_head` / `_next`) au constructeur. Le callback HAL global parcourt la liste et route l'IT vers l'instance active du `hadc` concerné.

> Sur un même `hadc` les canaux sont séquentiels (un seul actif à la fois), mais **plusieurs `hadc` peuvent convertir en parallèle** ; la résolution se fait donc par la paire `(hadc, _isActive)`.

```mermaid
sequenceDiagram
    participant H as HAL / ISR
    participant Reg as Adc::dispatchCallback (static)
    participant A as Adc actif

    H-->>Reg: HAL_ADC_ConvCpltCallback(hadc)
    loop parcours s_head → _next
        Reg->>Reg: a->_hadc == hadc && a->_isActive ?
    end
    Reg->>A: onConversionComplete()
    A->>A: _rawValue = HAL_ADC_GetValue(hadc)
    A->>A: xTaskNotifyFromISR(_notifyThreadId, _doneFlag)
    A->>A: _isActive = false
```

## Flux acquisition → lecture (orchestré par SensorManager)

```mermaid
sequenceDiagram
    participant SM as SensorManager (task)
    participant S  as Capteur (ISensor)
    participant A  as Adc (base)
    participant H  as HAL / ISR

    Note over SM,S: au démarrage : bind() sur chaque capteur
    SM->>S: bind()
    S->>A: _notifyThreadId = xTaskGetCurrentTaskHandle()

    loop pollDueGroups() — groupe échu
        SM->>S: trigger()
        S->>A: start()
        A->>H: HAL_ADC_ConfigChannel() + HAL_ADC_Start_IT()

        SM->>SM: xTaskNotifyWait(doneFlag, portMAX_DELAY)
        H-->>A: ConvCpltCallback → onConversionComplete()
        A-->>SM: xTaskNotifyFromISR(_doneFlag) → réveille la task

        SM->>S: read()
        S->>A: rawValue()
        A-->>S: _rawValue (uint16_t)
        S->>S: conversion (°C / mA / V)
        S-->>SM: float
        SM->>S: isAlarm()
        SM->>SM: latestSnapshot[id] = {value, timestamp, alarmMask}
    end
```

`SensorManager` regroupe les capteurs par fréquence (`SensorGroup`) et planifie chaque groupe via `nextDueMs`. Pour chaque capteur échu : `trigger()` → attente bloquante sur `doneFlag` (`xTaskNotifyWait`) → `read()` + `isAlarm()` → écriture dans `latestSnapshot`.

## Alarme — deux modes

Logique portée par chaque capteur (et non plus par `Adc`). Mise à jour de l'état de seuil dans `read()`, décision dans `isAlarm()`.

| `periodWindowMs` | Comportement `isAlarm()` |
|---|---|
| `0` (défaut) | Instantané : `_lastValue > _alarmThreshold` |
| `> 0` | Temporel : alarme si `_wasAbove == true` **et** `HAL_GetTick() - _riseTime >= _periodWindowMs` |

## Conversions

### InternalTemperature — NTC (`voltageToCelsius`)

```
adcVal (12 bits, 0–4095)
  └─► voltage = adcVal / 4096.0 × 3.3 V
        └─► R_ntc = voltage × R_ref / (3.3 − voltage)      (pont diviseur)
              └─► T_K = 1 / (1/T_ref_K + ln(R_ntc/R_ref) / B)   (Steinhart–Hart simplifié)
                    └─► T_°C = T_K − 273.15
```

| Constante | Valeur |
|---|---|
| `RESISTANCE_REFERENCE` | 10 000 Ω |
| `B_REFERENCE` | 3434 K |
| `TEMPERATURE_REFERENCE` | 25 °C |

### MotorCurrentSense — courant

```
Primaire   : I = (adcVal/4096 × 3.3 × 1000 / PRI_GAIN_FACTOR) / PRI_RESISTANCE_REFERENCE
Secondaire : I = (adcVal/4096 × 3.3) / SEC_RESISTANCE_REFERENCE
```

| Constante | Valeur |
|---|---|
| `PRI_RESISTANCE_REFERENCE` | 3090 Ω |
| `PRI_GAIN_FACTOR` | 0.000212 |
| `SEC_RESISTANCE_REFERENCE` | 0.5 Ω |

### B5WLB2101 — tension brute

```
V = adcVal / 4096.0 × 3.3 V
```

> **Note architecture :** par rapport à la v1 (`AnalogSensor` + `IAnalogSource`), les capteurs ADC fusionnent désormais driver et logique capteur dans une seule classe via double héritage `Adc` + `ISensor`. Le routage d'interruption passe par un registre intrusif statique (`Adc::dispatchCallback`) au lieu d'un pointeur d'instance unique. `ProximeterPololu5472` (InputCapture sur `htim3`) reste à intégrer — le squelette est présent mais commenté.
