---
stepsCompleted: [1]
session_topic: "Organisation en couches des dossiers App/* et Tests/*"
session_goals: "Définir les noms de dossiers possibles et leurs critères d'appartenance pour documenter dans architecture.md"
selected_approach: "AI-Recommended"
techniques_used: []
ideas_generated: []
---

## Session Overview

**Topic:** Organisation en couches des dossiers App/* et Tests/*
**Goals:** Trouver les noms de dossiers et critères d'appartenance pour une architecture en couches — à documenter dans architecture.md

### Contexte projet

Projet embarqué STM32F407 (robot Wall-A), C++17, FreeRTOS. Structure actuelle à réorganiser :

- `App/Drivers/` contient : drivers HW réels (Encoder, Drv8262, Motor), canaux de comm (UartChannel, UsbCdcChannel), et calcul odométrique (Odometry)
- `App/Tasks/` contient : vraies tâches FreeRTOS + PID (algorithme) + StubActuatorManager (stub de test)
- `Tests/Stubs/` contient : MockHAL.h (qui est un Mock, pas un Stub)

### Session Setup

Architecture en couches. Brainstorming des noms de dossiers et critères d'appartenance.

---

## Architecture consolidée

### App/*

| Couche | Dossier | Critère d'appartenance | Fichiers actuels |
|--------|---------|----------------------|-----------------|
| 1 | `App/Interfaces/` | Toutes les interfaces `I*.h` — contrats purs, aucune implémentation | `IActuator`, `IActuatorManager`, `IBus`, `ICommChannel`, `IEncoderHAL`, `IMotorHAL`, `IOdomHAL`, `ISensor`, `ISensorHAL` |
| 2 | `App/HAL/` | Implémente une `I***HAL` ou `ICommChannel`, dépend directement du HW (registres STM32 / HAL ST) | `Encoder`, `Drv8262`, `UartChannel`, `UsbCdcChannel` |
| 3 | `App/Services/` | Orchestre un ou plusieurs HAL, fournit un comportement de haut niveau, pas de tâche FreeRTOS | `Motor`, `Odometry` |
| 4 | `App/Controllers/` | Logique de calcul pur, sans dépendance FreeRTOS ni HW, testable isolément | `Pid` (+ futurs filtres, planificateurs...) |
| 5 | `App/Tasks/` | Hérite/instancie une tâche FreeRTOS — contient une boucle infinie ou `osThreadNew` | `ExternalComm`, `MotionPlanner`, `OdoControl` |

### Tests/*

| Dossier | Critère d'appartenance | Fichiers |
|---------|----------------------|---------|
| `Tests/Mocks/` | Utilise GMock (`MOCK_METHOD`) — remplace une dépendance avec vérification d'appels | `MockBus`, `MockCommChannel`, `MockEncoderHAL`, `MockMotorHAL`, `MockOdomHAL`, `MockSensorHAL`, `MockHAL` ← à déplacer depuis Stubs |
| `Tests/Stubs/` | Implémentation minimale sans GMock — simule l'environnement embarqué pour compiler | `FreeRTOS.h`, `FreeRTOSStub.cpp`, `queue.h`, `task.h`, `stm32f4xx_hal.h`, `StaticDefs.cpp` |
| `Tests/Unit/` | Fichiers `*Test.cpp` — un fichier de test par classe/module | `BusFormatTest`, `ConcreteOdomHALTest`, `ExternalCommTest`, `MotionPlannerTest`, `OdoControlTest` |

### Fichiers à déplacer

| Fichier | Depuis | Vers | Raison |
|---------|--------|------|--------|
| `Pid.h/.cpp` | `App/Tasks/` | `App/Controllers/` | Algorithme pur, pas une tâche |
| `StubActuatorManager.h` | `App/Tasks/` | `Tests/Stubs/` | Stub de test dans le code applicatif |
| `MockHAL.h` | `Tests/Stubs/` | `Tests/Mocks/` | Utilise GMock — c'est un Mock |

