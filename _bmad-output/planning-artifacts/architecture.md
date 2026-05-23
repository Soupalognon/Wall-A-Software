---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8]
lastStep: 8
status: 'complete'
completedAt: '2026-05-09'
inputDocuments:
  - '_bmad-output/brainstorming/brainstorming-session-2026-05-09-1400.md'
workflowType: 'architecture'
project_name: 'bmad - robot CDR'
user_name: 'Gdurand'
date: '2026-05-09'
---

# Architecture Decision Document

_Ce document se construit collaborativement à travers une découverte étape par étape. Les sections sont ajoutées au fil de nos décisions architecturales communes._

## Project Context Analysis

### Requirements Overview

**Functional Requirements:**
- Contrôle moteur en boucle fermée (PID, 200Hz) avec odométrie encodeurs gauche/droite
- Gestion capteurs : proximité, température, courant (jusqu'à 15 capteurs via ISensor)
- Gestion actionneurs : pompes, servomoteurs, transducteurs linéaires (jusqu'à 10 via IActuator)
- Communication externe tri-canal : UART (commande terrain), USB (supervision), Ethernet (réseau)
- Planification de trajectoire réactive aux alarmes capteurs
- Monitoring et agrégation de télémétrie en temps quasi-réel

**Non-Functional Requirements:**
- Déterminisme temps-réel : OdoControl 200Hz non-interruptible, latence alarmes ≤1 tick FreeRTOS
- Zéro allocation dynamique : `new`/`delete` interdits, heap non utilisé, empreinte mémoire compile-time
- Publications IBus non-bloquantes (overwrite/drop) : aucune inversion de priorité possible par construction
- Testabilité : injection de dépendance systématique (IBus*, HAL*) pour mocks sans matériel
- Extensibilité : ajout capteur/actionneur = nouvelle classe concrète uniquement, zéro modification existant

**Scale & Complexity:**
- Domaine primaire : Embedded C++ / STM32 HAL + FreeRTOS CMSIS V2
- Niveau de complexité : **Haute** — contraintes temps-réel strictes + multi-tâches + hardware constraints
- Composants architecturaux estimés : 8 classes métier, 4 interfaces domaine, 9 interfaces HAL, 6 tâches FreeRTOS

### Technical Constraints & Dependencies

- **Plateforme** : STM32 (CubeMX généré), HAL Cube, FreeRTOS CMSIS V2
- **Langage** : C++ orienté objet, sans exceptions, sans RTTI
- **Mémoire** : Statique uniquement — tableaux de taille fixe (MAX_SENSORS=15, MAX_ACTUATORS=10)
- **Scheduling** : Préemptif FreeRTOS — priorités explicites requises pour chaque tâche
- **Point d'entrée système** : `SystemInit::boot()` câble tout — `main.cpp` = 3 lignes

### Cross-Cutting Concerns Identified

- **Temps-réel** : Toutes décisions doivent préserver le déterminisme de OdoControl (aucun blocking sur chemin critique)
- **Sécurité** : `xQueueReset()` comme primitif d'arrêt d'urgence universel — propager ce pattern
- **Observabilité** : IBus est le seul canal de sortie — chaque module doit publier son état de santé
- **Injection de dépendance** : Toutes les dépendances passent par le constructeur — `SystemInit` est l'unique point de câblage

## Starter Template Evaluation

### Primary Technology Domain

Embedded C++ / Bare-metal + RTOS — plateforme STM32 avec HAL CubeMX et FreeRTOS CMSIS V2.

### Starter Options Considered

Aucun starter template générique n'existe pour STM32 C++ OO. La fondation standard est la génération CubeMX, point de départ universel pour ce domaine.

### Selected Starter: STM32CubeMX Generated Project

**Rationale for Selection:**
- Génère la configuration HAL complète (horloges, périphériques, DMA, IRQ)
- Intègre FreeRTOS CMSIS V2 nativement
- Produit le `main.c` remplacé par `main.cpp` + `SystemInit::boot()`
- Seule approche maintenue officiellement par STMicroelectronics

**Initialisation du projet:**

```bash
# Via STM32CubeIDE ou STM32CubeMX :
# 1. Créer projet STM32 pour la cible exacte
# 2. Activer FreeRTOS (CMSIS V2)
# 3. Configurer périphériques : TIM (encodeurs), PWM (moteurs), UART, USB, ETH
# 4. Générer le code
# 5. Renommer main.c → main.cpp, ajouter SystemInit::boot()
```

**Architectural Decisions Provided by Starter:**

**Language & Runtime:**
C++17 sans exceptions, sans RTTI — flags compilateur `-fno-exceptions -fno-rtti`

**Build Tooling:**
STM32CubeIDE (Eclipse + ARM GCC) ou Makefile généré par CubeMX

**Testing Framework:**
Tests unitaires sur host via Google Test + mocks HAL injectés — tests d'intégration sur cible via UART/USB

**Structure du projet:**

```
Core/
  Inc/         ← interfaces HAL générées (CubeMX)
  Src/         ← main.cpp (3 lignes : SystemInit::boot() + vTaskStartScheduler())
App/
  Interfaces/  ← tous les contrats I*.h (IBus, ISensor, IActuator, IMotorHAL, etc.)
  Drivers/     ← pilotent directement le HW via registres STM32 / HAL CubeMX
               │   Encoder.h/.cpp, Drv8262.h/.cpp, Motor.h/.cpp
               └── UartChannel.h/.cpp, UsbCdcChannel.h/.cpp
  Services/    ← orchestrent des Drivers via interfaces, sans toucher le HW
               └── Odometry.h/.cpp
  Controllers/ ← algorithmes purs, zéro dépendance FreeRTOS ou HW
               └── Pid.h/.cpp (+ futurs filtres, régulateurs...)
  Tasks/       ← tâches FreeRTOS uniquement (boucle infinie ou osThreadNew)
               │   OdoControl.h/.cpp      (1 tâche 200Hz, vTaskDelayUntil)
               │   MotionPlanner.h/.cpp   (1 tâche event-driven, xTaskNotify)
               │   SensorManager.h/.cpp   (1 tâche polling, vTaskDelay)
               │   Monitoring.h/.cpp      (1 tâche queue-driven, IBus)
               └── ExternalComm.h/.cpp    (2 tâches : rxTask + txTask, impl IBus)
  SystemInit/  ← SystemInit.h/.cpp (câblage statique complet, zéro new)
Drivers/       ← HAL CubeMX généré (ne pas modifier manuellement)
Middlewares/   ← FreeRTOS CMSIS V2 (ne pas modifier manuellement)
```

**Convention Tasks/ :** `ExternalComm` a deux entry points statiques (`rxTask`, `txTask`) créés par `SystemInit` — une seule classe, deux `xTaskCreate`. Un fichier = une classe = N tâches FreeRTOS possibles si la classe les gère toutes.

**Note:** La première story d'implémentation = créer le projet CubeMX avec la configuration hardware complète et valider que FreeRTOS démarre avec une tâche vide.

## Core Architectural Decisions

### Decision Priority Analysis

**Décisions critiques (bloquent l'implémentation) :**
- Protocole de communication ASCII Option A
- Format messages : `TOPIC payload\n`
- Gestion des erreurs via IBus ALERT

**Décisions importantes (structurent l'architecture) :**
- Configuration système `constexpr` compile-time

**Décisions différées :**
- Migration protocole ASCII → binaire si débit insuffisant (post-validation)
- Watchdog hardware : non requis (supervision humaine permanente)

### Communication Protocol

**Protocole :** ASCII Option A — `TOPIC PAYLOAD\n`

Mapping topics IBus ↔ préfixes ASCII :

| IBus Topic   | Préfixe ASCII | Direction      |
|--------------|---------------|----------------|
| —            | `CMD`         | PC → Robot     |
| `TELEMETRY`  | `TEL`         | Robot → PC     |
| `ALERT`      | `ALT`         | Robot → PC     |
| `LOG`        | `LOG`         | Robot → PC     |
| `HEALTH`     | `HLT`         | Robot → PC     |

**Exemples de messages :**
```
# PC → Robot
CMD MOVE 0.5 0.3\n
CMD ACTUATOR PUMP_1 ON\n

# Robot → PC
TEL ODO 1.23 0.45 90.0\n
ALT PROXIMITY_LOW 0.12\n
LOG INFO SystemInit OK\n
HLT TEMP 36.5 CURRENT 1.2\n
```

**Parsing côté STM32 :** `sscanf` sur buffer UART, dispatch sur premier token (`CMD`).
**Parsing côté PC :** split sur espace, premier token = topic, reste = payload.

### System Configuration

**Stratégie :** `constexpr` compile-time dans un header dédié `App/Config.h`

```cpp
namespace Config {
    static constexpr uint32_t ODO_FREQ_HZ      = 200;
    static constexpr uint8_t  MAX_SENSORS      = 15;
    static constexpr uint8_t  MAX_ACTUATORS    = 10;
    static constexpr float    PID_KP_DEFAULT   = 1.0f;
    static constexpr float    PID_KI_DEFAULT   = 0.1f;
    static constexpr float    PID_KD_DEFAULT   = 0.05f;
}
```

Avantage : empreinte mémoire nulle, optimisation compile-time complète, valeurs visibles dans le code.

### Error Handling

**Stratégie :** Sans exceptions C++ (`-fno-exceptions`).

Toute erreur critique est publiée sur IBus topic `ALERT` :
```cpp
bus_->publish(Topic::ALERT, "ALT HAL_ERROR MOTOR_L\n");
```

Les erreurs non-critiques (timeout, donnée obsolète) sont publiées sur `LOG` :
```cpp
bus_->publish(Topic::LOG, "LOG WARN Sensor timeout ID=3\n");
```

`Monitoring` souscrit à `ALERT` et agrège — aucun module ne connaît `Monitoring` directement.

### Monitoring — Accès aux données (Pull model)

**Décision :** `Monitoring` utilise un modèle **pull avec shared memory horodatée**. Chaque module qui expose des données de santé déclare une struct statique publique avec timestamp. `Monitoring` poll ces structs périodiquement. IBus est inchangé.

**Pattern par module :**

```cpp
// Dans OdoControl.h
struct OdoSnapshot {
    float x, y, angle;
    float speedLeft, speedRight;
    uint32_t timestamp; // HAL_GetTick()
};
static OdoSnapshot latestSnapshot; // écrit par OdoControl, lu par Monitoring
```

`OdoControl` met à jour `latestSnapshot` à chaque tick 200Hz. Le writer (haute priorité) n'a pas besoin de section critique — il ne peut pas être préempté par `Monitoring`. Le reader (`Monitoring`, basse priorité) DOIT protéger la copie avec `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` pour éviter un torn read (une struct multi-champs n'est pas atomique sur ARM Cortex-M) :

```cpp
// Writer — OdoControl (TRÈS HAUTE priorité) : pas de critical section nécessaire
latestSnapshot = { _x, _y, _angle, _speedLeft, _speedRight, HAL_GetTick() };

// Reader — Monitoring (BASSE priorité) : critical section obligatoire
OdoControl::OdoSnapshot snap;
taskENTER_CRITICAL();
snap = OdoControl::latestSnapshot;
taskEXIT_CRITICAL();
// Utiliser snap hors critical section
```

La section critique côté lecteur dure ~50ns à 168MHz (copie de ~24 octets) — aucun impact sur le déterminisme d'OdoControl. `Monitoring` lit la struct et vérifie le timestamp pour détecter une donnée obsolète.

**Modules exposant une snapshot :**
- `OdoControl::OdoSnapshot` — vitesse, position, erreur PID
- `SensorManager::SensorSnapshot` — état de chaque capteur, dernière valeur lue
- `ExternalComm::CommSnapshot` — compteurs RX/TX, dernière commande reçue

**Règle :** Les snapshots sont `static` dans la classe, initialisées à zéro dans `SystemInit`. `Monitoring` ne connaît que les types concrets des snapshots, pas les instances des tâches.

**Détection de donnée obsolète :** Si `HAL_GetTick() - snapshot.timestamp > Config::MONITORING_STALE_MS`, `Monitoring` publie une alerte sur IBus.

### Watchdog

Non requis — le robot opère sous supervision humaine permanente. Simplifie l'architecture des tâches (pas de kick watchdog à distribuer).

### Decision Impact Analysis

**Séquence d'implémentation imposée par ces décisions :**
1. `App/Config.h` — toutes les constantes, compilé en premier
2. `App/Interfaces/` — contrats IBus, ISensor, IActuator
3. `ExternalComm` — implémente IBus, parseur ASCII CMD, formatter TEL/ALT/LOG/HLT
4. `SystemInit` — câble tout, instancie les queues BUS_CONFIG

**Dépendances croisées :**
- Toute classe qui publie sur IBus doit connaître le format ASCII de son topic
- `ExternalComm` est le seul producteur de bytes série — toutes les sorties passent par lui

## Implementation Patterns & Consistency Rules

### Naming Patterns

**Membres privés :** préfixe underscore
```cpp
float _x, _y, _angle;
IBus* _bus;
IMotorHAL* _motorLeft;
```

**Constantes :** ALL_CAPS dans `namespace Config`
```cpp
namespace Config {
    static constexpr uint32_t ODO_FREQ_HZ    = 200;
    static constexpr uint8_t  MAX_SENSORS    = 15;
    static constexpr uint16_t STACK_ODO      = 512;
    static constexpr UBaseType_t PRIO_ODO    = 5;
}
```

**Interfaces :** préfixe `I` — `IBus`, `ISensor`, `IActuator`
**Classes métier :** PascalCase — `OdoControl`, `MotionPlanner`
**Fichiers :** PascalCase identique à la classe — `OdoControl.h/.cpp`

### Header Guards

Convention : `APP_<DOSSIER>_<FICHIER>_H`

```cpp
// App/Tasks/
#ifndef APP_TASKS_ODOCONTROL_H

// App/Drivers/
#ifndef APP_DRIVERS_MOTOR_H

// App/Services/
#ifndef APP_SERVICES_ODOMETRY_H

// App/Controllers/
#ifndef APP_CONTROLLERS_PID_H

// App/Interfaces/
#ifndef APP_INTERFACES_IBUS_H
```

### IBus Message Formatting

Tous les modules utilisent `BusFormat` (`App/BusFormat.h`) — jamais de `snprintf` inline :

```cpp
// ✅ Correct
bus_->publish(Topic::TELEMETRY, BusFormat::telOdo(x, y, angle));
bus_->publish(Topic::ALERT,     BusFormat::altProximity(distance));
bus_->publish(Topic::LOG,       BusFormat::logInfo("SystemInit OK"));

// ❌ Interdit — snprintf inline
char buf[64];
snprintf(buf, sizeof(buf), "TEL ODO %.2f\n", x);
bus_->publish(Topic::TELEMETRY, buf);
```

`BusFormat` centralise le format ASCII — un seul endroit à modifier si le protocole évolue.

### FreeRTOS Configuration

Stacks et priorités dans `App/Config.h` uniquement — jamais définis localement dans les fichiers Tasks :

```cpp
namespace Config {
    // Stacks (en mots de 32 bits)
    static constexpr uint16_t STACK_ODO_CONTROL    = 512;
    static constexpr uint16_t STACK_MOTION_PLANNER = 256;
    static constexpr uint16_t STACK_SENSOR_MANAGER = 256;
    static constexpr uint16_t STACK_MONITORING     = 256;
    static constexpr uint16_t STACK_EXTCOMM_RX     = 256;
    static constexpr uint16_t STACK_EXTCOMM_TX     = 256;

    // Priorités FreeRTOS (plus haute = plus prioritaire)
    static constexpr UBaseType_t PRIO_ODO_CONTROL    = 5;
    static constexpr UBaseType_t PRIO_MOTION_PLANNER = 4;
    static constexpr UBaseType_t PRIO_EXTCOMM_RX     = 4;
    static constexpr UBaseType_t PRIO_EXTCOMM_TX     = 3;
    static constexpr UBaseType_t PRIO_SENSOR_MANAGER = 2;
    static constexpr UBaseType_t PRIO_MONITORING     = 1;
}
```

### Language Convention

**Tout identifiant est en anglais sans exception :** noms de classes, méthodes, variables, membres, constantes, fichiers, guards, topics IBus, messages ASCII.

```cpp
// ✅ Correct
float _leftWheelSpeed;
void computeOdometry();
static constexpr uint32_t ODO_FREQ_HZ = 200;

// ❌ Interdit
float _vitesseRoueGauche;
void calculerOdometrie();
```

**Commentaires :** en anglais uniquement, et utilisés avec parcimonie. Les noms doivent être suffisamment explicites pour se passer de commentaire. Un commentaire est justifié uniquement pour expliquer un *pourquoi* non-évident (contrainte hardware, invariant subtil, workaround).

```cpp
// ✅ Justifié — explique un pourquoi non-évident
xQueuePeek(_setpointMailbox, &sp, 0); // non-blocking: never suspend at 200Hz

// ❌ Inutile — le nom dit déjà tout
float _leftWheelSpeed; // left wheel speed
void stop();           // stops the motor
```

### Enforcement — Règles Obligatoires

**Tout développeur/agent DOIT :**
- Écrire tous les identifiants et commentaires en anglais
- Préfixer les membres privés avec `_`
- Utiliser `BusFormat::` pour toute publication IBus, jamais de `snprintf` inline
- Déclarer stacks et priorités FreeRTOS dans `Config.h` uniquement
- Utiliser les guards `#ifndef APP_<DOSSIER>_<FICHIER>_H`
- Nommer chaque fichier identiquement à la classe qu'il contient (PascalCase)
- Ne jamais utiliser `new` ou `delete` — allocation statique uniquement
- Choisir des noms suffisamment explicites pour éviter les commentaires descriptifs
- **Développer UNIQUEMENT dans `App/` et `Tests/`** — tout autre dossier (`Core/`, `Drivers/`, `Middlewares/`) est autogénéré par CubeMX et ne doit jamais être modifié manuellement
- **Respecter les critères de dossier** : `Drivers/` = pilote le HW directement · `Services/` = orchestre des Drivers via interfaces · `Controllers/` = algorithme pur sans HW ni FreeRTOS · `Tasks/` = tâche FreeRTOS avec boucle infinie · `Interfaces/` = contrat `I*.h` uniquement

## Project Structure & Boundaries

### Complete Project Directory Structure

```
Wall-A-Software/                   ← racine du dépôt git
│
├── Wall-A-STM/                    ← projet STM32CubeIDE (firmware embarqué)
│   │
│   ├── Core/                      ← généré CubeMX (ne pas modifier)
│   │   ├── Inc/
│   │   │   ├── main.h
│   │   │   ├── FreeRTOSConfig.h
│   │   │   ├── stm32f4xx_hal_conf.h
│   │   │   └── stm32f4xx_it.h
│   │   ├── Src/
│   │   │   ├── main.c             ← généré CubeMX, appelle cppMain()
│   │   │   ├── freertos.c
│   │   │   ├── stm32f4xx_hal_msp.c
│   │   │   ├── stm32f4xx_hal_timebase_tim.c
│   │   │   ├── stm32f4xx_it.c     ← ISR handlers
│   │   │   ├── sys_getentropy.c
│   │   │   ├── syscalls.c
│   │   │   └── sysmem.c
│   │   └── Startup/
│   │       └── startup_stm32f407igtx.s
│   │
│   ├── App/
│   │   ├── cppMain.cpp            ← point d'entrée C++ : boot() + vTaskStartScheduler()
│   │   ├── Config.h               ← toutes les constexpr (fréq, stacks, prios, PID)
│   │   │
│   │   ├── Interfaces/            ← contrats purs I*.h, aucune implémentation
│   │   │   ├── IActuator.h
│   │   │   ├── IActuatorHAL.h
│   │   │   ├── IActuatorManager.h
│   │   │   ├── IBus.h
│   │   │   ├── ICommChannel.h
│   │   │   ├── IEncoderHAL.h
│   │   │   ├── IMotorHAL.h
│   │   │   ├── IOdomHAL.h
│   │   │   ├── ISensor.h
│   │   │   └── ISensorHAL.h
│   │   │
│   │   ├── Drivers/               ← pilotent directement le HW (registres STM32 / HAL CubeMX)
│   │   │   ├── Drv8262.h/.cpp     ← pilote le circuit DRV8262 via GPIO/PWM
│   │   │   ├── Encoder.h/.cpp     ← lit les timers encodeurs via HAL
│   │   │   ├── InternalTemperature.h/.cpp  ← lecture ADC température interne STM32
│   │   │   ├── MotorCurrentSense.h/.cpp    ← lecture ADC courant moteur
│   │   │   ├── UartChannel.h/.cpp ← canal UART via HAL_UART
│   │   │   ├── UsbCdcChannel.h/.cpp        ← canal USB CDC via HAL USB
│   │   │   └── Stubs/             ← implémentations stub des interfaces actionneurs/capteurs
│   │   │       ├── CurrentSensor.h/.cpp
│   │   │       ├── LinearTransducer.h/.cpp
│   │   │       ├── ProximitySensor.h/.cpp
│   │   │       ├── Pump.h/.cpp
│   │   │       ├── Servo.h/.cpp
│   │   │       └── TemperatureSensor.h/.cpp
│   │   │
│   │   ├── Services/              ← orchestrent des Drivers via interfaces, sans toucher le HW
│   │   │   ├── BusFormat.h/.cpp   ← helpers formatage ASCII IBus
│   │   │   ├── Motor.h/.cpp       ← commande un moteur via IMotorHAL
│   │   │   └── Odometry.h/.cpp    ← calcul position/vitesse à partir de IEncoderHAL
│   │   │
│   │   ├── Controllers/           ← algorithmes purs, zéro dépendance FreeRTOS ou HW
│   │   │   └── Pid.h/.cpp         ← régulateur PID générique
│   │   │
│   │   ├── Tasks/                 ← tâches FreeRTOS uniquement (boucle infinie ou osThreadNew)
│   │   │   ├── ActuatorManager.h/.cpp      ← tâche gestion actionneurs, impl IActuatorManager
│   │   │   ├── ExternalComm.h/.cpp         ← 2 tâches rxTask+txTask, impl IBus
│   │   │   ├── Monitoring.h/.cpp           ← 1 tâche queue-driven, seuils/alertes
│   │   │   ├── MotionPlanner.h/.cpp        ← 1 tâche event-driven, xTaskNotify
│   │   │   ├── OdoControl.h/.cpp           ← 1 tâche 200Hz, vTaskDelayUntil
│   │   │   ├── SensorManager.h/.cpp        ← 1 tâche polling, ISensor[MAX_SENSORS]
│   │   │   └── StubActuatorManager.h       ← stub no-op de IActuatorManager pour tests
│   │   │
│   │   └── SystemInit/
│   │       ├── SystemInit.h
│   │       └── SystemInit.cpp     ← boot(), câblage statique complet, zéro new
│   │
│   ├── Drivers/                   ← généré CubeMX (ne pas modifier)
│   │   ├── BSP/
│   │   ├── CMSIS/
│   │   └── STM32F4xx_HAL_Driver/
│   │
│   ├── Middlewares/               ← généré CubeMX (ne pas modifier)
│   │   ├── ST/STM32_USB_Device_Library/
│   │   └── Third_Party/
│   │       ├── FreeRTOS/
│   │       └── LwIP/
│   │
│   ├── LWIP/                      ← généré CubeMX — configuration LwIP (ne pas modifier)
│   │   ├── App/
│   │   │   └── lwip.c/.h
│   │   └── Target/
│   │       ├── ethernetif.c/.h
│   │       └── lwipopts.h
│   │
│   ├── USB_DEVICE/                ← généré CubeMX — stack USB CDC (ne pas modifier)
│   │   ├── App/
│   │   │   ├── usb_device.c/.h
│   │   │   ├── usbd_cdc_if.c/.h   ← callbacks CDC (point de contact avec UsbCdcChannel)
│   │   │   └── usbd_desc.c/.h
│   │   └── Target/
│   │       └── usbd_conf.c/.h
│   │
│   └── Tests/                     ← tests unitaires sur host (Google Test / GMock)
│       ├── CMakeLists.txt
│       ├── run_tests.sh
│       ├── Mocks/                 ← utilisent MOCK_METHOD GMock, vérifient les appels
│       │   ├── MockActuator.h
│       │   ├── MockActuatorHAL.h
│       │   ├── MockBus.h
│       │   ├── MockCommChannel.h
│       │   ├── MockEncoderHAL.h
│       │   ├── MockMotorHAL.h
│       │   ├── MockOdomHAL.h
│       │   ├── MockSensor.h
│       │   └── MockSensorHAL.h
│       ├── Stubs/                 ← headers/implémentations minimales pour compiler sur host
│       │   ├── FreeRTOS.h / FreeRTOSStub.cpp
│       │   ├── HalStub.h
│       │   ├── queue.h / task.h
│       │   ├── stm32f4xx_hal.h
│       │   ├── StaticDefs.cpp
│       │   └── usbd_cdc_if.h
│       └── Unit/                  ← fichiers *Test.cpp, un par classe testée
│           ├── ActuatorDriversTest.cpp
│           ├── ActuatorManagerTest.cpp
│           ├── BusFormatTest.cpp
│           ├── ConcreteOdomHALTest.cpp
│           ├── ExternalCommTest.cpp
│           ├── MonitoringTest.cpp
│           ├── MotionPlannerTest.cpp
│           ├── OdoControlTest.cpp
│           ├── PidTest.cpp
│           ├── SensorDriversTest.cpp
│           └── SensorManagerTest.cpp
│
└── pythonTester/                  ← outil Python host pour tester le robot via USB/série
    ├── main.py                    ← point d'entrée
    ├── app.py
    ├── config.py
    ├── data_store.py
    ├── gamepad_tester.py
    ├── gamepad_worker.py
    ├── parser.py
    ├── serial_worker.py
    └── requirements.txt
```

### Folder Belonging Criteria

| Dossier | Critère d'appartenance |
|---------|----------------------|
| `App/Interfaces/` | Contrat pur `I*.h` — aucune implémentation, aucune dépendance HW |
| `App/Drivers/` | Parle directement au HW via registres STM32 ou HAL CubeMX — implémente une `I***HAL` ou `ICommChannel` |
| `App/Services/` | Orchestre un ou plusieurs Drivers **via leurs interfaces** — aucun appel HAL direct, pas une tâche FreeRTOS |
| `App/Controllers/` | Algorithme pur — zéro dépendance FreeRTOS, zéro dépendance HW, testable sans matériel |
| `App/Tasks/` | Hérite ou instancie une tâche FreeRTOS — contient obligatoirement une boucle infinie ou `osThreadNew` |
| `Tests/Mocks/` | Utilise `MOCK_METHOD` GMock — vérifie que les appels ont bien eu lieu |
| `Tests/Stubs/` | Implémentation minimale sans GMock — permet de compiler les tests sur host sans matériel |
| `Tests/Unit/` | Fichiers `*Test.cpp` — un fichier par classe testée |

### Architectural Boundaries

**1. `App/` ne peut pas accéder directement à `Drivers/` HAL CubeMX**

Les fonctions HAL générées (`HAL_GPIO_WritePin`, `HAL_TIM_ReadCapturedValue`, etc.) ne doivent jamais être appelées directement depuis une classe métier. On passe toujours par une interface injectée (`IMotorHAL*`, `IEncoderHAL*`). Si CubeMX regénère les fichiers HAL, le code métier reste intact. Les tests unitaires sur PC injectent un mock sans matériel.

**2. Une tâche ne peut pas appeler directement une autre tâche**

`OdoControl` ne peut pas appeler une méthode de `MotionPlanner`. `SensorManager` ne peut pas appeler une méthode d'`ExternalComm`. La seule communication inter-tâches autorisée est via IBus (`publish`) ou via les primitives FreeRTOS natives (`xTaskNotify`, `xQueueOverwrite`). Appeler directement une méthode d'une autre tâche crée du couplage fort, risque d'inversion de priorité, et rend le test unitaire impossible.

**3. Une tâche ne peut pas instancier ou utiliser un driver custom directement**

`SensorManager` ne crée pas un `VL53L0X` en interne — il reçoit un tableau d'`ISensor*` déjà instanciés par `SystemInit`. L'implémentation concrète est injectée. On peut ajouter un capteur en créant une nouvelle classe concrète sans toucher `SensorManager`.

**4. `SystemInit` est le seul endroit qui instancie et câble**

Aucune classe ne crée d'instance en dehors de `SystemInit::boot()`. Toutes les instances sont en mémoire statique, tous les pointeurs injectés via constructeurs. Zéro allocation dynamique, empreinte mémoire connue à la compilation, et un seul endroit pour comprendre le câblage complet du système.

### Requirements to Structure Mapping

| Fonctionnalité | Fichier(s) |
|----------------|------------|
| Locomotion PID + odométrie | `Tasks/OdoControl` + `Services/Odometry` + `Controllers/Pid` |
| Planification trajectoire + réactivité alarmes | `Tasks/MotionPlanner` |
| Pilotage moteurs HW | `Drivers/Motor` + `Drivers/Drv8262` |
| Lecture encodeurs HW | `Drivers/Encoder` |
| Acquisition capteurs | `Tasks/SensorManager` |
| Commande actionneurs | `Tasks/ExternalComm` → `IActuatorManager` |
| Supervision et agrégation télémétrie | `Tasks/Monitoring` |
| Communication PC UART | `Drivers/UartChannel` → `Tasks/ExternalComm` |
| Communication PC USB | `Drivers/UsbCdcChannel` → `Tasks/ExternalComm` |
| Contrats inter-modules | `App/Interfaces/` |
| Format messages ASCII | `App/Services/BusFormat.h/.cpp` |
| Toutes les constantes | `App/Config.h` |
| Câblage système | `App/SystemInit/SystemInit.cpp` |

### Data Flow

```
PC → UART/USB/ETH → ExternalComm::rxTask → MotionPlanner  (CMD MOVE)
                                          → ActuatorManager (CMD ACTUATOR)

Encodeurs → OdoControl → PID → Moteurs
OdoControl → IBus (TEL ODO) → ExternalComm::txTask → PC

SensorManager → xTaskNotify → MotionPlanner (alarme critique)
SensorManager → IBus (ALT/HLT) → ExternalComm::txTask → PC

Tous modules → IBus (LOG) → ExternalComm::txTask → PC
```

### Dependency Graph

> **Légende :** vert = communication PC ↔ ExternalComm · rouge = flux temps-réel FreeRTOS (`xQueueOverwrite`, `xTaskNotify`) · tirets gris = publications IBus · pointillés = snapshots pull (Monitoring) · gris clair = interfaces HAL injectées · *bleu italique* = fréquence de la tâche

```dot
digraph G {
    rankdir=TB
    nodesep=1.0
    ranksep=1.5
    fontname="Helvetica"
    node [fontname="Helvetica" fontsize=13 style=filled shape=box fillcolor="#dde8f5" color="#6688aa" penwidth=1.5]
    edge [fontname="Helvetica" fontsize=10 color="#444444"]

    PC         [shape=ellipse fillcolor="#f5f0dd" label=<<B><FONT POINT-SIZE="15">PC</FONT></B><BR/><I><FONT POINT-SIZE="9">usb / eth</FONT></I>>]
    DEBUG      [shape=ellipse fillcolor="#f5f0dd" label=<<B><FONT POINT-SIZE="15">DEBUG</FONT></B><BR/><I><FONT POINT-SIZE="9">uart</FONT></I>>]
    ExtComm    [label=<<B>ExternalComm</B><BR/><I><FONT POINT-SIZE="10" COLOR="#336699">Queue</FONT></I>> fillcolor="#c8daf5"]
    MoPlan     [label=<<B>MotionPlanner</B><BR/><I><FONT POINT-SIZE="10" COLOR="#336699">Queue or Notify</FONT></I>>]
    OdoCtrl    [label=<<B>OdoControl<BR/><FONT POINT-SIZE="10">TRÈS HAUTE PRIORITÉ</FONT><BR/></B><I><FONT POINT-SIZE="10" COLOR="#336699">200Hz</FONT></I>> fillcolor="#ffd9d9"]
    SenMgr     [label=<<B>SensorManager</B><BR/><I><FONT POINT-SIZE="10" COLOR="#336699">10Hz</FONT></I>>]
    Monitoring [label=<<B>Monitoring</B><BR/><I><FONT POINT-SIZE="10" COLOR="#336699">10Hz</FONT></I>>]

    { rank=same; PC; DEBUG }
    { rank=same; ExtComm }
    { rank=same; MoPlan }
    { rank=same; OdoCtrl; SenMgr }

    PC -> ExtComm    [label="CMD" color="#226622" fontcolor="#226622" style=bold]
    ExtComm -> PC    [label="TELEMETRY / HEALTH / ALERT" color="#226622" fontcolor="#226622" style=dashed]
    DEBUG -> ExtComm    [label="CMD" color="#226622" fontcolor="#226622" style=bold]
    ExtComm -> DEBUG    [label="All logs" color="#226622" fontcolor="#226622" style=dashed]

    ExtComm -> MoPlan  [label="xQueueOverwrite (CMD)" color="#cc4400" fontcolor="#cc4400" penwidth=2]
    MoPlan  -> OdoCtrl [label="xQueueOverwrite (mailbox)" color="#cc4400" fontcolor="#cc4400" penwidth=2]
    SenMgr  -> MoPlan  [label="xTaskNotify (alarm)" color="#cc4400" fontcolor="#cc4400" penwidth=2]
    Monitoring  -> MoPlan  [label="xTaskNotify (alarm)" color="#cc4400" fontcolor="#cc4400" penwidth=2]

    OdoCtrl    -> ExtComm [label="TELEMETRY" style=dashed color="#555555" fontcolor="#555555"]
    Monitoring -> ExtComm [label="TELEMETRY / HEALTH / ALERT" style=dashed color="#555555" fontcolor="#555555"]

    OdoCtrl -> Monitoring [label="snapshot" style=dotted color="#aaaaaa" fontcolor="#aaaaaa"]
    SenMgr  -> Monitoring [label="snapshot" style=dotted color="#aaaaaa" fontcolor="#aaaaaa"]
}
```

## Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility :** IBus + injection de dépendance + zéro `new` + `SystemInit` forment un système cohérent sans contradictions. Les priorités FreeRTOS respectent la hiérarchie temps-réel. Le format ASCII est cohérent avec `BusFormat` centralisé.

**Pattern Consistency :** Les conventions de nommage (`_prefix`, `ALL_CAPS`, `PascalCase`) sont uniformes. La règle "aucune tâche n'appelle directement une autre tâche" est respectée partout — `ExternalComm` → `ActuatorManager` via queue, pas via appel direct.

**Structure Alignment :** La structure `App/` supporte toutes les décisions. Les frontières sont claires et applicables. `SystemInit` est le seul point de câblage.

### Requirements Coverage Validation ✅

| Exigence | Couverture |
|----------|------------|
| Contrôle moteur 200Hz | `OdoControl` + mailbox |
| Odométrie encodeurs | `OdoControl` seul lecteur `IEncoderHAL` |
| Capteurs (jusqu'à 15) | `SensorManager` + `ISensor[MAX_SENSORS]` |
| Actionneurs (jusqu'à 10) | `ActuatorManager` + `IActuator[MAX_ACTUATORS]` |
| Communication PC tri-canal | `ExternalComm` UART/USB/ETH ASCII |
| Alarmes rapides ≤1 tick | `xTaskNotify` bitmask |
| Télémétrie | IBus → `ExternalComm::txTask` |
| Extensibilité capteurs/actionneurs | `ISensor`/`IActuator` — nouvelle classe uniquement |
| Tests unitaires sans hardware | Interfaces HAL + mocks injectés |

### Implementation Readiness Validation ✅

Tous les agents/développeurs disposent de :
- Contrats d'interface complets (`App/Interfaces/` + `App/Interfaces/HAL/`)
- Règles de nommage et de structure explicites
- Format de communication défini (`BusFormat`)
- Configuration centralisée (`Config.h`)
- Un seul point de câblage (`SystemInit`)

### Architecture Completeness Checklist

**Requirements Analysis**
- [x] Project context thoroughly analyzed
- [x] Scale and complexity assessed
- [x] Technical constraints identified
- [x] Cross-cutting concerns mapped

**Architectural Decisions**
- [x] Critical decisions documented
- [x] Technology stack fully specified
- [x] Integration patterns defined
- [x] Performance considerations addressed

**Implementation Patterns**
- [x] Naming conventions established
- [x] Structure patterns defined
- [x] Communication patterns specified
- [x] Process patterns documented

**Project Structure**
- [x] Complete directory structure defined
- [x] Component boundaries established
- [x] Integration points mapped
- [x] Requirements to structure mapping complete

### Architecture Readiness Assessment

**Overall Status :** READY FOR IMPLEMENTATION

**Confidence Level :** Haute — toutes les décisions critiques sont prises, les frontières sont claires, les patterns sont applicables immédiatement.

**Key Strengths :**
- Zéro allocation dynamique — empreinte mémoire prévisible
- IBus découple tous les modules — testabilité native
- `SystemInit` unique point de câblage — architecture compréhensible d'un seul fichier
- Frontières strictes — aucune tâche ne peut involontairement bloquer une autre

**Areas for Future Enhancement :**
- Migration protocole ASCII → binaire si débit insuffisant
- Ajout watchdog hardware si contexte de supervision change

### Implementation Handoff

**AI Agent Guidelines :**
- Suivre toutes les décisions architecturales exactement comme documentées
- Utiliser `BusFormat::` pour toute publication IBus — jamais de `snprintf` inline
- Déclarer stacks et priorités dans `Config.h` uniquement
- Respecter les frontières : aucune tâche n'appelle directement une autre tâche
- `SystemInit` est le seul endroit autorisé à instancier et câbler

**First Implementation Priority :**
1. Créer projet STM32CubeMX avec configuration hardware complète
2. Implémenter `App/Config.h` et `App/Interfaces/`
3. Implémenter `App/BusFormat.h/.cpp`
4. Implémenter `ExternalComm` + `IBus`
5. Implémenter `SystemInit::boot()` avec câblage statique complet

---

## Dependency Graph — OdoControl (zoom)

Fonctionnement interne de la tâche OdoControl (200 Hz, priorité 5) : pipeline de contrôle, dépendances HAL, et sorties système.

```dot
digraph OdoControl {
    rankdir=TB
    node [fontname="Helvetica" fontsize=10 shape=box]
    edge [fontname="Helvetica" fontsize=9]

    // ── Contexte système (entrées) ──────────────────────────────────────────
    subgraph cluster_ctx {
        label="Contexte système"
        style=dashed color="#aaaaaa" fontcolor="#aaaaaa"
        fontname="Helvetica" fontsize=10

        MoPlan   [label="MotionPlanner\n(Task)" style=filled fillcolor="#fff3cd"]
        ExtComm  [label="ExternalComm\n(Task)" style=filled fillcolor="#fff3cd"]
        Monitoring [label="Monitoring\n(Task)" style=filled fillcolor="#fff3cd"]
    }

    Mailbox [label="setpointMailbox\n(QueueHandle_t)\nSetpoint { v, w }"
             shape=cylinder style=filled fillcolor="#ffe0cc"]

    // ── Pipeline interne OdoControl ─────────────────────────────────────────
    subgraph cluster_odo {
        label="OdoControl  [200 Hz — priorité 5]"
        style=filled fillcolor="#fff5f5" color="#cc4400"
        fontname="Helvetica" fontsize=11

        OdomUpd  [label="1. _odom->update()\nlecture encodeurs"
                  style=filled fillcolor="#ffdddd"]
        EMAFilt  [label="2. Filtre EMA — lissage consigne\nspFilteredV  (α = 0.15)\nspFilteredW  (α = 0.05)"
                  style=filled fillcolor="#ffdddd"]
        ErrCalc  [label="3. Calcul erreur\ndv = spFilteredV − v_mes\ndw = spFilteredW − w_mes"
                  style=filled fillcolor="#ffdddd"]
        PIDComp  [label="4. Feedforward + PID\nv = spFilteredV · FF_V + PID_speed(dv)\nw = spFilteredW · FF_W + PID_angle(dw)"
                  style=filled fillcolor="#ffdddd"]
        Mixer    [label="5. Mélangeur différentiel\nleftDuty  = clamp(v − w, −1, 1)\nrightDuty = clamp(v + w, −1, 1)"
                  style=filled fillcolor="#ffdddd"]
        FaultDet [label="6. Détection pannes\n• Stall : duty↑ & |v| → 0\n• Enc. fault : duty↑ & vL/R = 0"
                  shape=diamond style=filled fillcolor="#ffcccc"]
        Snap     [label="7. Snapshot  (÷ 10 ticks)\nlatestSnapshot ← { x, y, θ,\n  vL, vR, v, w, timestamp }"
                  style=filled fillcolor="#ffdddd"]
    }

    // ── Couche HAL ──────────────────────────────────────────────────────────
    subgraph cluster_hal {
        label="Couche HAL"
        style=filled fillcolor="#eaf4ea" color="#336633"
        fontname="Helvetica" fontsize=10

        OdomHAL  [label="IOdomHAL\n→ Odometry\n→ IEncoderHAL\n→ Encodeurs (TIM)"
                  style=filled fillcolor="#c8e6c8"]
        MotorHAL [label="IMotorHAL\n→ Drv8262\n→ PWM (TIM)"
                  style=filled fillcolor="#c8e6c8"]
    }

    // ── Sorties bus ─────────────────────────────────────────────────────────
    BusTelem [label="IBus TELEMETRY\nBusFormat::telOdoVelocity\n(chaque tick)"
              shape=ellipse style=filled fillcolor="#d0e8ff"]
    BusAlert [label="IBus ALERT\naltStall / altEncoderFault\naltInitFailed"
              shape=ellipse style=filled fillcolor="#ffd0d0"]

    // ── Flux d'entrée ───────────────────────────────────────────────────────
    MoPlan  -> Mailbox  [label="xQueueOverwrite"
                         color="#cc4400" fontcolor="#cc4400" penwidth=2]
    Mailbox -> EMAFilt  [label="xQueuePeek  (200 Hz)"
                         color="#cc4400" fontcolor="#cc4400" penwidth=2]
    ExtComm -> PIDComp  [label="setPidGains()" style=dashed
                         color="#555555" fontcolor="#555555"]

    // ── Pipeline interne ────────────────────────────────────────────────────
    OdomHAL -> OdomUpd  [label="ticks encodeurs"
                         color="#336633" fontcolor="#336633"]
    OdomUpd -> ErrCalc  [label="getV(), getW()"]
    OdomUpd -> Snap     [label="getX(), getY(), getAngle()" style=dashed
                         color="#aaaaaa" fontcolor="#aaaaaa"]
    EMAFilt -> ErrCalc
    ErrCalc -> PIDComp
    PIDComp -> Mixer
    Mixer   -> FaultDet

    // ── Sortie moteurs ──────────────────────────────────────────────────────
    FaultDet -> MotorHAL [label="setMotors(L, R)  ✔"
                          color="#336633" fontcolor="#336633" penwidth=2]
    FaultDet -> BusAlert [label="panne détectée" style=dashed
                          color="#cc0000" fontcolor="#cc0000"]

    // ── Init failures ───────────────────────────────────────────────────────
    OdomHAL  -> BusAlert [label="begin() KO" style=dashed
                           color="#cc0000" fontcolor="#cc0000"]
    MotorHAL -> BusAlert [label="begin() KO" style=dashed
                           color="#cc0000" fontcolor="#cc0000"]

    // ── Télémétrie & snapshot ───────────────────────────────────────────────
    Mixer -> BusTelem   [label="telOdoVelocity" style=dashed
                         color="#555555" fontcolor="#555555"]
    Snap  -> Monitoring [label="read (taskENTER_CRITICAL)" style=dotted
                         color="#aaaaaa" fontcolor="#aaaaaa"]

    // ── Mise en page ────────────────────────────────────────────────────────
    { rank=same; MoPlan; ExtComm; Monitoring }
    { rank=same; OdomHAL; MotorHAL }
    { rank=same; BusTelem; BusAlert }
}
```

---

## Dependency Graph — RX path (PC → Robot)

Flux d'entrée : depuis le physique (UART byte-par-byte / USB CDC burst) jusqu'au dispatch applicatif. Les deux canaux convergent sur la même `_rxByteQueue`.

> **Légende :** rouge = chemin ISR → FreeRTOS · vert = périphérique HAL/CDC stack · bleu = tâche applicative · tirets gris = callbacks hardware

```dot
digraph RxPath {
    rankdir=TB
    nodesep=0.9
    ranksep=1.0
    fontname="Helvetica"
    node [fontname="Helvetica" fontsize=10 style=filled shape=box fillcolor="#dde8f5" color="#6688aa" penwidth=1.5]
    edge [fontname="Helvetica" fontsize=9 color="#444444"]

    // ── Acteurs externes ─────────────────────────────────────────────────────
    DEBUG  [shape=ellipse fillcolor="#f5f0dd" label=<<B>DEBUG</B><BR/><I><FONT POINT-SIZE="9">UART</FONT></I>>]
    PC     [shape=ellipse fillcolor="#f5f0dd" label=<<B>PC</B><BR/><I><FONT POINT-SIZE="9">USB CDC</FONT></I>>]

    // ── Couche HAL ───────────────────────────────────────────────────────────
    subgraph cluster_hw {
        label="STM32 Hardware / HAL"
        style=filled fillcolor="#eaf4ea" color="#336633"
        fontname="Helvetica" fontsize=10

        UART_HW [label="UART périphérique\nHUART_HandleTypeDef\nHAL_UART_Receive_IT(1 octet)"
                 style=filled fillcolor="#c8e6c8"]
        USB_HW  [label="USB FS périphérique\nUSBD_HandleTypeDef\n+ CDC stack"
                 style=filled fillcolor="#c8e6c8"]
    }

    // ── UartChannel RX ───────────────────────────────────────────────────────
    subgraph cluster_uart {
        label="UartChannel  (Driver)"
        style=filled fillcolor="#fff8ee" color="#cc8800"
        fontname="Helvetica" fontsize=10

        URxISR [label="onRxComplete(huart)\n─────────────────\nISR context\nxQueueSendFromISR(_rxQueue, byte)\nHAL_UART_Receive_IT()  ← ré-armement"
                fillcolor="#ffe8cc"]
    }

    // ── UsbCdcChannel RX ─────────────────────────────────────────────────────
    subgraph cluster_usb {
        label="UsbCdcChannel  (Driver)"
        style=filled fillcolor="#f0f0ff" color="#5555cc"
        fontname="Helvetica" fontsize=10

        USRxISR [label="onRxData(buf, len)\n─────────────────\nISR context  (buffer entier)\nfor each byte:\n  xQueueSendFromISR(_rxQueue, byte)\nportYIELD_FROM_ISR(woken)"
                 fillcolor="#d8d8ff"]
    }

    // ── Queue partagée ───────────────────────────────────────────────────────
    RxQueue [label="_rxByteQueue\nQueueHandle_t  (64 octets)\npartagée UART + USB"
             shape=cylinder fillcolor="#ffd0d0" color="#cc0000" penwidth=2]

    // ── ExternalComm rxTask ──────────────────────────────────────────────────
    subgraph cluster_ec {
        label="ExternalComm  (Task)"
        style=filled fillcolor="#e8f0ff" color="#334499"
        fontname="Helvetica" fontsize=10

        RxTask [label="rxTask\n─────────────────\nxQueueReceive(bloquant)\nassemble octets → ligne (\\n)\nsscanf / token parse\ndispatch sur préfixe CMD"
                fillcolor="#c0d0f0"]
    }

    // ── Destinations ─────────────────────────────────────────────────────────
    MoPlan [label="MotionPlanner\nxQueueOverwrite\n(CMD MOVE v w)"  fillcolor="#fff3cd"]
    ActMgr [label="ActuatorManager\ndirect call\n(CMD ACTUATOR id state)" fillcolor="#fff3cd"]

    // ── Arêtes ───────────────────────────────────────────────────────────────
    DEBUG  -> UART_HW [color="#336633" penwidth=2]
    PC     -> USB_HW  [color="#336633" penwidth=2]

    UART_HW -> URxISR  [label="HAL_UART_RxCpltCallback" style=dashed
                        color="#888888" fontcolor="#888888"]
    USB_HW  -> USRxISR [label="USB_CDC_RxHandler" style=dashed
                        color="#888888" fontcolor="#888888"]

    URxISR  -> RxQueue [label="xQueueSendFromISR  (1 octet)"
                        color="#cc0000" fontcolor="#cc0000" penwidth=2]
    USRxISR -> RxQueue [label="xQueueSendFromISR  (octet × len)"
                        color="#cc0000" fontcolor="#cc0000" penwidth=2]

    RxQueue -> RxTask  [label="xQueueReceive  (bloquant)"
                        color="#cc0000" fontcolor="#cc0000" penwidth=2]

    RxTask  -> MoPlan  [label="xQueueOverwrite" color="#334499" fontcolor="#334499" penwidth=2]
    RxTask  -> ActMgr  [label="direct call"     color="#334499" fontcolor="#334499"]

    // ── Mise en page ─────────────────────────────────────────────────────────
    { rank=same; DEBUG; PC }
    { rank=same; UART_HW; USB_HW }
    { rank=same; URxISR; USRxISR }
    { rank=same; MoPlan; ActMgr }
}
```

---

## Dependency Graph — TX path (Robot → PC)

Flux de sortie : depuis n'importe quelle tâche qui publie sur IBus jusqu'à l'émission physique sur UART et USB CDC. Chaque canal dispose de son propre ring buffer et d'un mécanisme de pompage interruptible.

> **Légende :** orange = chemin UART · violet = chemin USB CDC · bleu = tâche applicative · vert = périphérique HAL · tirets gris = callbacks ISR/CDC

```dot
digraph TxPath {
    rankdir=TB
    nodesep=0.9
    ranksep=1.0
    fontname="Helvetica"
    node [fontname="Helvetica" fontsize=10 style=filled shape=box fillcolor="#dde8f5" color="#6688aa" penwidth=1.5]
    edge [fontname="Helvetica" fontsize=9 color="#444444"]

    // ── Producteurs IBus ─────────────────────────────────────────────────────
    subgraph cluster_prod {
        label="Tâches productrices"
        style=filled fillcolor="#f8f8e8" color="#888833"
        fontname="Helvetica" fontsize=10

        OdoCtrl  [label="OdoControl\npublish(TELEMETRY)" fillcolor="#fff3cd"]
        SenMgr   [label="SensorManager\npublish(ALERT/LOG)" fillcolor="#fff3cd"]
        Monitor  [label="Monitoring\npublish(HEALTH/ALERT)" fillcolor="#fff3cd"]
        Others   [label="autres tâches\npublish(LOG)" fillcolor="#fff3cd"]
    }

    // ── ExternalComm txTask ──────────────────────────────────────────────────
    subgraph cluster_ec {
        label="ExternalComm  (Task)"
        style=filled fillcolor="#e8f0ff" color="#334499"
        fontname="Helvetica" fontsize=10

        TxQueues [label="TxQueueSet\n_telQueue(1) · _altQueue(1)\n_hltQueue(2) · _logQueue(4)"
                  shape=cylinder fillcolor="#b0c4e8"]
        TxTask   [label="txTask\n─────────────────\nxQueueSelectFromSet(bloquant)\nrécupère TxEntry\ntransmit() sur chaque canal\nselon policy (uart/usb/eth)"
                  fillcolor="#c0d0f0"]
    }

    // ── UartChannel TX ───────────────────────────────────────────────────────
    subgraph cluster_uart {
        label="UartChannel  (Driver)"
        style=filled fillcolor="#fff8ee" color="#cc8800"
        fontname="Helvetica" fontsize=10

        UTxRing [label="_txRingBuf[512]\n(ring circulaire)"
                 shape=cylinder fillcolor="#ffe0a0"]
        UTxPump [label="_pumpTx()\n─────────────────\ncopie ring → _txStagingBuf\nHAL_UART_Transmit_IT\n_txBusy = true"
                 fillcolor="#fff3cc"]
        UTxISR  [label="onTxComplete(huart)\n─────────────────\nISR context\n_txBusy = false\n_pumpTx()  ← vide le reste"
                 fillcolor="#ffe8cc"]
    }

    // ── UsbCdcChannel TX ─────────────────────────────────────────────────────
    subgraph cluster_usb {
        label="UsbCdcChannel  (Driver)"
        style=filled fillcolor="#f0f0ff" color="#5555cc"
        fontname="Helvetica" fontsize=10

        USTxRing [label="_txRingBuf[2048]\n(ring circulaire)"
                  shape=cylinder fillcolor="#c8c8ff"]
        USTxPump [label="_pumpTx()\n─────────────────\ncopie ring → _txStagingBuf\nCDC_Transmit_FS\n_txBusy = true"
                  fillcolor="#ddddff"]
        USTxISR  [label="onTxComplete()\n─────────────────\ncallback CDC\n_txBusy = false\n_pumpTx()  ← vide le reste"
                  fillcolor="#d8d8ff"]
    }

    // ── Couche HAL ───────────────────────────────────────────────────────────
    subgraph cluster_hw {
        label="STM32 Hardware / HAL"
        style=filled fillcolor="#eaf4ea" color="#336633"
        fontname="Helvetica" fontsize=10

        UART_HW [label="UART périphérique\nIT"    style=filled fillcolor="#c8e6c8"]
        USB_HW  [label="USB FS périphérique\nCDC stack" style=filled fillcolor="#c8e6c8"]
    }

    // ── Acteurs externes ─────────────────────────────────────────────────────
    DEBUG [shape=ellipse fillcolor="#f5f0dd" label=<<B>DEBUG</B><BR/><I><FONT POINT-SIZE="9">UART</FONT></I>>]
    PC    [shape=ellipse fillcolor="#f5f0dd" label=<<B>PC</B><BR/><I><FONT POINT-SIZE="9">USB CDC</FONT></I>>]

    // ── Arêtes ───────────────────────────────────────────────────────────────
    OdoCtrl -> TxQueues [label="publish(TELEMETRY)\nxQueueOverwrite" color="#334499" fontcolor="#334499"]
    SenMgr  -> TxQueues [label="publish(ALERT/LOG)\nxQueueOverwrite" color="#334499" fontcolor="#334499"]
    Monitor -> TxQueues [label="publish(HEALTH)\nxQueueSend"        color="#334499" fontcolor="#334499"]
    Others  -> TxQueues [label="publish(LOG)\nxQueueSend"           color="#334499" fontcolor="#334499"]

    TxQueues -> TxTask  [label="xQueueSelectFromSet" color="#334499" fontcolor="#334499" penwidth=2]

    // UART TX
    TxTask  -> UTxRing  [label="transmit()\ntaskENTER_CRITICAL"
                         color="#cc8800" fontcolor="#cc8800" penwidth=2]
    UTxRing -> UTxPump  [color="#cc8800"]
    UTxPump -> UART_HW  [label="HAL_UART_Transmit_IT"
                         color="#336633" fontcolor="#336633" penwidth=2]
    UART_HW -> UTxISR   [label="HAL_UART_TxCpltCallback" style=dashed
                         color="#888888" fontcolor="#888888"]
    UTxISR  -> UTxPump  [label="encore des octets ?" style=dashed
                         color="#cc8800" fontcolor="#cc8800"]
    UART_HW -> DEBUG    [color="#336633" penwidth=2]

    // USB TX
    TxTask   -> USTxRing [label="transmit()\ntaskENTER_CRITICAL"
                          color="#5555cc" fontcolor="#5555cc" penwidth=2]
    USTxRing -> USTxPump [color="#5555cc"]
    USTxPump -> USB_HW   [label="CDC_Transmit_FS"
                          color="#336633" fontcolor="#336633" penwidth=2]
    USB_HW   -> USTxISR  [label="UsbCdcChannel_onTxComplete" style=dashed
                          color="#888888" fontcolor="#888888"]
    USTxISR  -> USTxPump [label="encore des octets ?" style=dashed
                          color="#5555cc" fontcolor="#5555cc"]
    USB_HW   -> PC       [color="#336633" penwidth=2]

    // ── Mise en page ─────────────────────────────────────────────────────────
    { rank=same; OdoCtrl; SenMgr; Monitor; Others }
    { rank=same; UTxRing; USTxRing }
    { rank=same; UTxPump; USTxPump }
    { rank=same; UTxISR;  USTxISR  }
    { rank=same; UART_HW; USB_HW   }
    { rank=same; DEBUG;   PC       }
}
```
