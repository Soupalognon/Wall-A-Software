---
stepsCompleted: ['step-01-validate-prerequisites', 'step-02-design-epics', 'step-03-create-stories', 'step-04-final-validation']
inputDocuments:
  - '_bmad-output/planning-artifacts/prd.md'
  - '_bmad-output/planning-artifacts/architecture.md'
---

# bmad - robot CDR - Epic Breakdown

## Overview

Ce document fournit la décomposition complète en epics et stories pour le projet Robot CDR (STM32/FreeRTOS C++), en décomposant les exigences du PRD et de l'Architecture en stories implémentables.

## Requirements Inventory

### Functional Requirements

FR-01: OdoControl implémente une tâche FreeRTOS unique exécutant séquentiellement lecture encodeurs → calcul odométrie → boucle PID → commande moteurs à cadence fixe 200Hz via `vTaskDelayUntil`. OdoControl est le seul consommateur de IEncoderHAL et IMotorHAL.

FR-02: MotionPlanner écrit la consigne de vitesse dans une queue FreeRTOS depth-1 via `xQueueOverwrite`. OdoControl lit via `xQueuePeek` à chaque tick sans jamais bloquer. `xQueueReset()` vide la consigne (arrêt d'urgence) en O(1).

FR-03: ExternalComm implémente IBus. Quatre topics : TELEMETRY, ALERT, LOG, HEALTH. Politique par topic configurable dans BUS_CONFIG[] (overwrite ou drop-silent). Toute classe reçoit IBus* par injection de constructeur.

FR-04: ExternalComm gère UART (priorité commande terrain), USB (supervision haute fréquence), Ethernet (supervision réseau). Format ASCII : `TOPIC PAYLOAD\n`. En cas de commande simultanée, UART écrase. Parsing par `sscanf` sur premier token.

FR-05: SensorManager orchestre jusqu'à MAX_SENSORS=15 instances ISensor* injectées par SystemInit. Ajout d'un capteur = nouvelle classe concrète ISensor uniquement. SensorManager ne connaît pas les types concrets.

FR-06: ActuatorManager implémente IActuatorManager, orchestre jusqu'à MAX_ACTUATORS=10 instances IActuator*. ExternalComm commande via IActuatorManager::commandById(id, cmd). ActuatorManager ne connaît pas ExternalComm.

FR-07: SensorManager envoie `xTaskNotify(motionPlannerHandle, bitmask, eSetBits)` lors d'une alarme critique. Chaque bit du bitmask correspond à un type d'alarme. MotionPlanner se réveille sans polling.

FR-08: Monitoring accède aux données des modules via des structs statiques publiques horodatées (`OdoControl::OdoSnapshot`, `SensorManager::SensorSnapshot`, `ExternalComm::CommSnapshot`). Chaque module met à jour sa struct à chaque cycle. Monitoring poll périodiquement et publie une alerte sur IBus(ALERT) si `HAL_GetTick() - snapshot.timestamp > Config::MONITORING_STALE_MS`. IBus reste publish-only — pas de subscribe.

FR-09: Toutes les publications IBus utilisent BusFormat:: (BusFormat::telOdo(), BusFormat::altProximity(), etc.). `snprintf` inline dans les classes métier est interdit.

FR-10: Toutes les constantes système (fréquences, tailles de buffer, stacks FreeRTOS, priorités, PID defaults, MONITORING_STALE_MS) sont déclarées `constexpr` dans App/Config.h. Aucune magic number dans les fichiers de classes.

FR-11: SystemInit::boot() instancie tous les objets en mémoire statique et injecte toutes les dépendances. Aucune autre classe n'instancie un objet métier. Zéro `new`.

### NonFunctional Requirements

NFR-01: La tâche OdoControl s'exécute à 200Hz (5ms ± 50µs) sans interruption par des tâches de priorité inférieure. Les publications IBus depuis OdoControl sont non-bloquantes (overwrite) — aucun appel bloquant sur le chemin critique.

NFR-02: L'empreinte RAM totale du firmware est connue à la compilation. Aucune allocation dynamique. Les tableaux ont des tailles fixes (MAX_SENSORS=15, MAX_ACTUATORS=10). Les stacks FreeRTOS sont définies dans Config.h et validées par le linker.

NFR-03: 100% des classes dans App/Tasks/, App/Drivers/, App/SystemInit/ sont instanciables sur PC x86 avec mocks injectés. Les tests Google Test compilent et s'exécutent sans toolchain ARM ni hardware STM32.

NFR-04: Les mécanismes de communication inter-tâches autorisés sont : IBus::publish(), xTaskNotify(), xQueueOverwrite(), xQueuePeek() — communication générale. Exception : lecture de structs statiques publiques horodatées (OdoSnapshot, SensorSnapshot, CommSnapshot) exclusivement par Monitoring, protégée par taskENTER_CRITICAL()/taskEXIT_CRITICAL() côté lecteur uniquement (le writer haute priorité n'en a pas besoin). Toute autre forme d'accès mémoire inter-tâches est une erreur de conception détectable à la review.

NFR-05: L'ajout d'un capteur, actionneur ou module de communication ne modifie aucune classe existante. L'interface (ISensor, IActuator, IBus) est le seul contrat. SystemInit est le seul fichier à modifier pour câbler un nouveau composant.

NFR-06: Membres privés préfixés `_`. Constantes ALL_CAPS dans namespace Config. Interfaces préfixées I. Fichiers nommés identiquement à la classe (PascalCase). Guards `#ifndef APP_<DOSSIER>_<FICHIER>_H`. Violations détectées en code review.

NFR-07: Tout code développé manuellement réside exclusivement dans App/ et Tests/. Les dossiers Core/, Drivers/ et Middlewares/ sont autogénérés par CubeMX et ne doivent jamais être modifiés manuellement. Les commits ne contiennent aucune modification dans Core/, Drivers/ ou Middlewares/ (hors régénération CubeMX explicite).

NFR-08: Tout identifiant est en anglais sans exception : noms de classes, méthodes, variables, membres, constantes, fichiers, guards, topics IBus, messages ASCII.

### Additional Requirements

- **Starter Template :** Projet STM32CubeMX généré (STM32CubeIDE ou CubeMX). La première story d'implémentation = créer le projet CubeMX avec configuration hardware complète (TIM encodeurs ×2, PWM moteurs ×2, UART, USB CDC, Ethernet) et valider que FreeRTOS CMSIS V2 démarre avec une tâche vide. Renommer main.c → main.cpp.
- **Structure de répertoires :** robot-cdr/ avec Core/ (généré CubeMX), App/ (code métier), Drivers/ (HAL généré), Middlewares/ (FreeRTOS généré), Tests/ (Google Test host).
- **Interfaces HAL :** Toutes les classes métier accèdent au hardware via des interfaces injectées (IMotorHAL*, IEncoderHAL*, etc.) — jamais d'appel direct à HAL CubeMX depuis App/.
- **Tests unitaires :** Infrastructure Google Test sur host (CMakeLists.txt dans Tests/) avec Mocks/ (MockBus.h, MockMotorHAL.h, MockSensorHAL.h) et Tests/Unit/ pour chaque classe métier.
- **Protocole ASCII :** Format `TOPIC PAYLOAD\n` — cinq préfixes : CMD (PC→Robot), TEL, ALT, LOG, HLT (Robot→PC). BusFormat centralise tout le formatage.
- **Priorités FreeRTOS :** OdoControl=5, MotionPlanner=4, ExternalComm RX=4, ExternalComm TX=3, SensorManager=2, Monitoring=1. Stacks définies dans Config.h.
- **Séquence d'implémentation imposée :** (1) Config.h → (2) App/Interfaces/ → (3) ExternalComm + IBus → (4) SystemInit → puis modules métier.

### UX Design Requirements

Aucun document UX — projet embarqué sans interface graphique.

### FR Coverage Map

FR-01: Epic 2 — OdoControl tâche 200Hz PID encodeurs
FR-02: Epic 2 — Mailbox consigne xQueueOverwrite/Peek arrêt d'urgence
FR-03: Epic 1 — IBus interface + implémentation ExternalComm
FR-04: Epic 1 — ExternalComm UART/USB/ETH ASCII tri-canal
FR-05: Epic 3 — SensorManager ISensor[MAX_SENSORS=15]
FR-06: Epic 3 — ActuatorManager IActuator[MAX_ACTUATORS=10]
FR-07: Epic 3 — Alarmes xTaskNotify bitmask ≤1 tick FreeRTOS
FR-08: Epic 3 — Monitoring pull sur OdoSnapshot/SensorSnapshot/CommSnapshot horodatées
FR-09: Epic 1 — BusFormat centralisé, snprintf interdit
FR-10: Epic 1 — Config.h constexpr, zéro magic number (incl. MONITORING_STALE_MS)
FR-11: Epic 1 — SystemInit câblage statique complet, zéro new
NFR-01 à NFR-08: Transversaux — enforcement via conventions, tests, architecture boundaries

## Epic List

### Epic 1: Squelette Système
Un développeur peut démarrer FreeRTOS sur cible et valider l'architecture de communication — projet CubeMX opérationnel, interfaces IBus définies, ExternalComm fonctionnel (UART/USB/ETH ASCII), câblage statique complet, infrastructure Google Test sur host PC.
**FRs couverts :** FR-03, FR-04, FR-09, FR-10, FR-11

### Epic 2: Contrôle Locomotion
Un développeur peut piloter le robot en boucle fermée via commandes ASCII — `CMD MOVE` transite par MotionPlanner → OdoControl 200Hz PID, télémétrie odométrique sur IBus, arrêt d'urgence xQueueReset() opérationnel.
**FRs couverts :** FR-01, FR-02

### Epic 3: Écosystème Capteurs-Actionneurs
Un développeur peut intégrer jusqu'à 15 capteurs et 10 actionneurs avec alarmes et monitoring temps-réel — ajout capteur = nouvelle classe ISensor uniquement, alarmes ≤1 tick FreeRTOS, actionneurs commandés par ASCII, télémétrie surveillée par Monitoring.
**FRs couverts :** FR-05, FR-06, FR-07, FR-08

---

## Epic 1: Squelette Système

Un développeur peut démarrer FreeRTOS sur cible et valider l'architecture de communication — projet CubeMX opérationnel, interfaces IBus définies, ExternalComm fonctionnel (UART/USB/ETH ASCII), câblage statique complet, infrastructure Google Test sur host PC.

### Story 1.1: Projet STM32CubeMX configuré et FreeRTOS opérationnel sur cible

En tant que développeur,
Je veux un projet STM32CubeMX correctement configuré avec FreeRTOS CMSIS V2 et tous les périphériques requis initialisés,
Afin d'avoir une fondation hardware validée sur laquelle tout le code applicatif sera construit.

**Critères d'acceptation :**

**Given** la cible STM32 est sélectionnée dans CubeMX
**When** le projet est généré avec FreeRTOS CMSIS V2, TIM (encodeurs ×2), PWM (moteurs ×2), UART, USB CDC et Ethernet activés
**Then** le projet compile sans erreur avec les flags `-fno-exceptions -fno-rtti -Wall -Wextra`

**Given** le projet généré est flashé sur la cible STM32
**When** la carte est alimentée
**Then** FreeRTOS démarre, une tâche idle minimale s'exécute, et une LED de debug clignote confirmant que le scheduler est actif

**Given** `main.c` a été renommé en `main.cpp`
**When** un stub `SystemInit::boot()` et `vTaskStartScheduler()` sont ajoutés
**Then** `main.cpp` contient exactement 3 lignes fonctionnelles et compile en C++17

**And** `Core/`, `Drivers/` et `Middlewares/` sont laissés non-modifiés (NFR-07) — seul `Core/Src/main.cpp` est renommé lors de cette étape initiale uniquement

**And** toutes les snapshots statiques (`OdoControl::OdoSnapshot`, `SensorManager::SensorSnapshot`, `ExternalComm::CommSnapshot`) sont déclarées et initialisées à zéro dans `SystemInit` — prêtes avant tout démarrage de tâche

### Story 1.2: App/Config.h et App/Interfaces/ — constantes et contrats domaine

En tant que développeur,
Je veux toutes les constantes système déclarées `constexpr` dans `App/Config.h` et toutes les interfaces domaine définies dans `App/Interfaces/`,
Afin que chaque classe suivante dispose d'une source unique de vérité pour la configuration et de contrats clairs pour l'injection de dépendance.

**Critères d'acceptation :**

**Given** `App/Config.h` est créé avec `namespace Config`
**When** `grep` est lancé sur `App/Tasks/` à la recherche de valeurs numériques littérales pour stacks, priorités, fréquences ou bornes de tableaux
**Then** zéro occurrence trouvée — toutes les valeurs proviennent de `Config::*`

**Given** `App/Interfaces/IBus.h` est créé
**When** une classe reçoit `IBus*` dans son constructeur
**Then** elle peut appeler `publish(Topic topic, const char* payload)` sans connaître l'implémentation concrète, avec `Topic` enum couvrant TELEMETRY, ALERT, LOG, HEALTH

**Given** `App/Interfaces/ISensor.h` est créé
**When** un mock implémente `ISensor`
**Then** il doit fournir `uint8_t id()`, `const char* name()`, et `float read()`

**Given** `App/Interfaces/IActuator.h` et `IActuatorManager.h` sont créés
**When** `ExternalComm` tient un `IActuatorManager*`
**Then** il peut appeler `commandById(uint8_t id, const char* cmd)` sans connaître les types d'actionneurs concrets

**And** tous les guards suivent la convention `#ifndef APP_<DOSSIER>_<FICHIER>_H`

**And** `Config::MONITORING_STALE_MS` est déclaré `constexpr` dans `App/Config.h` — utilisé par `Monitoring` pour détecter les données obsolètes

**Given** `App/` est scanné pour la conformité NFR-08
**When** `grep -rn '[àâçèéêëîïôùûüÿœæ]' App/` est exécuté
**Then** zéro occurrence trouvée dans les identifiants — noms de classes, méthodes, variables, constantes, fichiers et messages ASCII sont en anglais exclusivement

**Given** les headers `App/` sont scannés pour la conformité NFR-06 (guards)
**When** `grep -rn "#ifndef" App/ | grep -v "APP_[A-Z0-9_]*_H"` est exécuté
**Then** zéro occurrence — tous les guards respectent la convention `APP_<DOSSIER>_<FICHIER>_H` (conventions _, PascalCase, ALL_CAPS vérifiées en code review)

**Given** `App/` est scanné pour la conformité NFR-06 (conventions de nommage)
**When** `grep -rn 'private:' App/ --include='*.h' -A5 | grep -v '^\s*_'` est exécuté sur les membres privés
**Then** zéro membre privé sans préfixe `_`

**When** `grep -rn 'class [^I][a-z]' App/ --include='*.h'` est exécuté
**Then** zéro classe métier en minuscule — toutes les classes sont PascalCase

**When** `grep -rn 'class I[^A-Z]' App/Interfaces/ --include='*.h'` est exécuté
**Then** zéro interface avec préfixe `I` suivi d'une minuscule — toutes les interfaces sont `I` + PascalCase

**When** `grep -rn 'constexpr.*[a-z][a-z]' App/Config.h` est exécuté
**Then** zéro constante `Config::` en minuscule — toutes les constantes sont ALL_CAPS

### Story 1.3: App/BusFormat.h/.cpp — formatage ASCII IBus centralisé

En tant que développeur,
Je veux tout le formatage des messages IBus centralisé dans `App/BusFormat`,
Afin que toute classe publie des messages ASCII sans écrire `snprintf` inline et que le protocole soit modifiable en un seul endroit.

**Critères d'acceptation :**

**Given** `BusFormat.h/.cpp` sont créés avec des méthodes factory statiques
**When** une classe publie de la télémétrie odométrique
**Then** elle appelle `BusFormat::telOdo(float x, float y, float angle)` qui retourne un `const char*` au format `TEL ODO %.2f %.2f %.2f\n`

**Given** BusFormat est disponible
**When** `BusFormat::altProximity(float dist)`, `BusFormat::logInfo(const char* msg)`, `BusFormat::hltTemp(float t)` sont appelés
**Then** chacun retourne une chaîne ASCII correctement préfixée (`ALT`, `LOG`, `HLT`) terminée par `\n`

**Given** le répertoire `App/` est scanné
**When** `grep -r "snprintf" App/` est exécuté en excluant `BusFormat.cpp`
**Then** zéro occurrence trouvée

**And** toutes les méthodes BusFormat utilisent des buffers internes fixes (statiques ou stack-locaux) — zéro allocation dynamique

### Story 1.4: ExternalComm — implémentation IBus et communication tri-canal ASCII

En tant que développeur,
Je veux qu'`ExternalComm` implémente `IBus` et gère la communication UART/USB/Ethernet avec le protocole ASCII,
Afin que tous les modules puissent publier sans connaître la couche transport et que le robot puisse recevoir des commandes du PC.

**Critères d'acceptation :**

**Given** `ExternalComm` implémente `IBus::publish(Topic, const char*)`
**When** un module appelle `bus_->publish(Topic::TELEMETRY, BusFormat::telOdo(...))`
**Then** `ExternalComm::txTask` transmet le message vers tous les canaux de sortie actifs (USB, ETH)

**Given** `ExternalComm::rxTask` est en cours et UART reçoit `CMD MOVE 0.5 0.3\n`
**When** le message est parsé avec `sscanf` sur le premier token
**Then** la consigne est écrite dans la mailbox mouvement via `xQueueOverwrite`

**Given** UART et USB reçoivent un `CMD MOVE` simultanément
**When** `rxTask` les traite
**Then** la commande UART prend la priorité et écrase la commande USB

**Given** `Tests/Unit/ExternalCommTest.cpp` existe avec `MockBus` et mock UART/USB
**When** les tests s'exécutent sur PC host
**Then** tous les variants CMD (MOVE, ACTUATOR) sont correctement dispatchés et les inputs invalides gérés — tests passent sans hardware STM32

**And** `ExternalComm` est construit avec `IBus*` injecté — il n'instancie aucune dépendance en interne

**And** `ExternalComm::CommSnapshot` (struct statique publique avec `rxCount`, `txCount`, `lastCmd[32]`, `timestamp`) est mise à jour par `rxTask` à chaque commande reçue — lisible par `Monitoring` sans synchronisation explicite (écriture atomique ARM Cortex-M)

### Story 1.5: SystemInit::boot() — câblage statique complet

En tant que développeur,
Je veux que `SystemInit::boot()` instancie tous les objets en mémoire statique et injecte toutes les dépendances,
Afin que `main.cpp` contienne seulement 3 lignes et que le câblage complet du système soit lisible en un seul fichier, sans allocation dynamique.

**Critères d'acceptation :**

**Given** `SystemInit::boot()` est implémenté
**When** il s'exécute
**Then** tous les objets (ExternalComm, stubs SensorManager/MotionPlanner/OdoControl) sont instanciés avec le mot-clé `static` et injectés via constructeurs

**Given** `SystemInit::boot()` est complet
**When** `grep -r "new\|delete\|malloc\|free" App/` est exécuté
**Then** zéro occurrence trouvée hors commentaires

**Given** `main.cpp` appelle `SystemInit::boot()` puis `vTaskStartScheduler()`
**When** le firmware démarre
**Then** les tâches FreeRTOS sont créées par SystemInit, le scheduler démarre, aucun crash ne se produit

**And** `SystemInit` est le seul fichier qui appelle `xTaskCreate` — aucune tâche n'en crée une autre

### Story 1.6: Infrastructure Google Test + mocks HAL + tests BusFormat/ExternalComm

En tant que développeur,
Je veux une infrastructure Google Test configurée pour PC host avec mocks HAL,
Afin que toutes les classes métier puissent être testées sans hardware STM32 ni toolchain ARM.

**Critères d'acceptation :**

**Given** `Tests/CMakeLists.txt` est créé
**When** `cmake .. && make` s'exécute sur PC host (x86)
**Then** tous les tests unitaires compilent et linkent sans toolchain ARM

**Given** `Tests/Mocks/MockBus.h` existe et implémente `IBus`
**When** un test instancie `MockBus` et le passe à une classe comme `IBus*`
**Then** les messages publiés sont capturés dans un buffer pour assertion

**Given** `Tests/Unit/BusFormatTest.cpp` existe
**When** toutes les méthodes factory BusFormat sont testées
**Then** les chaînes de sortie correspondent exactement au format ASCII attendu (terminateur `\n` inclus)

**Given** `Tests/Unit/ExternalCommTest.cpp` existe
**When** les tests de parsing CMD s'exécutent avec mock UART
**Then** tous les variants valides sont dispatchés correctement et les inputs invalides sont gérés

**And** `MockMotorHAL.h` et `MockSensorHAL.h` sont créés dans `Tests/Mocks/` — prêts pour Epic 2 et Epic 3

---

## Epic 2: Contrôle Locomotion

Un développeur peut piloter le robot en boucle fermée via commandes ASCII — `CMD MOVE` transite par MotionPlanner → OdoControl 200Hz PID, télémétrie odométrique sur IBus, arrêt d'urgence xQueueReset() opérationnel.

### Story 2.1: HAL Interfaces Locomotion — IEncoderHAL, IOdomHAL, IMotorHAL, ConcreteOdomHAL, MotorHAL + Config.h OdoControl

En tant que développeur,
Je veux les interfaces HAL locomotion et leurs implémentations concrètes, ainsi que toutes les constantes OdoControl dans `App/Config.h`,
Afin qu'`OdoControl` ne touche jamais le hardware directement et que tout calcul cinématique soit isolé dans `ConcreteOdomHAL`, testable via mocks.

**Critères d'acceptation :**

**Given** `App/Interfaces/IEncoderHAL.h` est créé
**When** `ConcreteOdomHAL` l'implémente
**Then** `IEncoderHAL` expose uniquement `int32_t getTicksLeft()` et `int32_t getTicksRight()` (position cumulée 32-bit) — le wrapping de l'overflow 16-bit est géré par cast `(int16_t)(current - old)` dans `ConcreteOdomHAL`, conformément au code Wall-A `Encoder.cpp`

**Given** `App/Interfaces/IOdomHAL.h` est créé
**When** `OdoControl` reçoit `IOdomHAL*` par injection de constructeur
**Then** `OdoControl` accède à `update()`, `getX()`, `getY()`, `getAngle()`, `getVLeft()`, `getVRight()`, `getV()`, `getW()`, `getDt()` — sans jamais connaître `IEncoderHAL` ni `Encoder` directement

**Given** `App/Drivers/ConcreteOdomHAL.h/.cpp` est créé
**When** `ConcreteOdomHAL::update()` s'exécute
**Then** il lit les ticks via `IEncoderHAL*`, calcule `dL`, `dR`, intègre position `(x, y, angle)` par Euler différentiel, calcule vitesses `(vL, vR, v, ω)`, et expose `dt` via timer hardware µs (implémentation `getDt()` branchée sur timer CubeMX configuré par l'utilisateur)

**And** `ConcreteOdomHAL` détient `Encoder _encoderL, _encoderR` par valeur (zéro indirection supplémentaire) — le virtuel `IOdomHAL` existe uniquement pour la testabilité de la couche supérieure

**And** la normalisation angle est appliquée après chaque intégration :
```cpp
if (_angle >  M_PI) _angle -= 2.0f * M_PI;
if (_angle < -M_PI) _angle += 2.0f * M_PI;
```

**Given** `App/Interfaces/IMotorHAL.h` est créé
**When** `MotorHAL` l'implémente en wrappant `Drv8262::setMotors(float, float)`
**Then** `IMotorHAL { virtual void setMotors(float left, float right) = 0; }` — adapter pattern pur, zéro réécriture du driver Wall-A

**Given** `App/Config.h` est mis à jour avec les constantes OdoControl
**When** `grep` est lancé sur `App/Tasks/OdoControl*` à la recherche de valeurs numériques littérales
**Then** zéro occurrence — toutes les valeurs proviennent de `Config::*`, incluant :
```cpp
constexpr float    ARRIVAL_THRESHOLD    = 0.02f;
constexpr float    PID_I_MAX_SPEED      = 0.5f;
constexpr float    PID_I_MAX_ANGLE      = 0.5f;
constexpr float    MAX_DUTY             = 0.8f;
constexpr float    STALL_DUTY_THRESHOLD = 0.5f;
constexpr uint32_t STALL_TIME_MS        = 500;
constexpr int8_t   ENCODER_L_SIGN       = +1;
constexpr int8_t   ENCODER_R_SIGN       = +1;
constexpr int8_t   MOTOR_L_SIGN         = +1;
constexpr int8_t   MOTOR_R_SIGN         = +1;
constexpr uint8_t  TELEM_DIVIDER        = 10;
```

**Given** `Tests/Mocks/MockOdomHAL.h` et `Tests/Mocks/MockMotorHAL.h` existent
**When** les mocks sont injectés dans un test `ConcreteOdomHALTest.cpp`
**Then** le calcul odométrique Euler et les vitesses calculées sont validés sur PC host avec des deltas de ticks connus — tests passent sans hardware

**And** les facteurs de signe `ENCODER_L/R_SIGN` sont appliqués dans `ConcreteOdomHAL::update()`, `MOTOR_L/R_SIGN` dans `MotorHAL::setMotors()` — corriger un câblage = changer une constante dans `Config.h`, pas chercher un bug dans les formules

**Given** les drivers Wall-A sont réutilisés
**When** la story est démarrée
**Then** `DriversCustom/Encoder/Encoder.cpp` et `Encoder.hpp` sont copiés depuis `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Encoder\` vers `robot-cdr/App/Drivers/Encoder/` sans modification — `ConcreteOdomHAL` les inclut directement et détient `Encoder _encoderL, _encoderR` par valeur

**And** `DriversCustom/Motor/Drv8262.cpp` et `Drv8262.hpp` sont copiés depuis `d:\_Programs\STM32\Wall-A\Wall-A-STM\DriversCustom\Motor\` vers `robot-cdr/App/Drivers/Motor/` sans modification — `MotorHAL` wraps `Drv8262` via adapter pattern pur, zéro réécriture du driver existant

### Story 2.2: OdoControl — tâche 200Hz, double PID orthogonal (v/ω), struct Setpoint union, condition d'arrivée

En tant que développeur,
Je veux qu'`OdoControl` s'exécute à 200Hz via `vTaskDelayUntil` avec deux PIDs orthogonaux sur v et ω, une struct `Setpoint` union polymorphe, et une condition d'arrivée explicite,
Afin d'avoir un contrôle locomotion déterministe en boucle fermée avec une convergence naturelle vers les waypoints sans machine d'état explicite.

**Critères d'acceptation :**

**Given** `OdoControl` est construit avec `OdoControl(IOdomHAL*, IMotorHAL*, IBus*)`
**When** la tâche s'exécute à 200Hz via `vTaskDelayUntil`
**Then** le temps d'exécution par cycle est mesuré < 4ms (80% du budget 5ms) via timer HAL

**Given** la séquence de tick 200Hz est implémentée
**When** un tick s'exécute
**Then** l'ordre est exactement :
1. `vTaskDelayUntil()` — réveil déterministe
2. `IOdomHAL::update()` — lit encodeurs + dt, intègre position
3. `xQueuePeek(mailbox, 0)` — setpoint non-bloquant
4. if `!_hasSetpoint` → `setMotors(0,0)`, retour — guard boot
5. Calcul erreurs distance et cap : `errDist = sqrt(Δx²+Δy²)`, `errAngle = atan2f(Δy,Δx) - angle`
6. `PID_speed.compute(errDist, dt)`
7. `PID_angle.compute(errAngle, dt)`
8. Clamp sortie : `v = clamp(v, -MAX_DUTY, MAX_DUTY)`, `ω = clamp(ω, -MAX_DUTY, MAX_DUTY)`
9. Décomposition : `leftDuty = v - ω`, `rightDuty = v + ω`
10. `IMotorHAL::setMotors(leftDuty, rightDuty)`
11. if `(tickCount % TELEM_DIVIDER == 0)` → mise à jour `OdoSnapshot`
12. if `(tickCount % TELEM_DIVIDER == 0)` → `bus_->publish(TELEMETRY, BusFormat::telOdo(x,y,angle))`

**Given** la struct `Setpoint` est définie
**When** `MotionPlanner` écrit dans la mailbox
**Then** `Setpoint` contient `Mode mode` + `union { PoseTarget{x,y,angle}; VelocityTarget{v,w} }` — mailbox unifiée, `OdoControl` dispache selon `mode`

**Given** deux PIDs orthogonaux `PID_speed` et `PID_angle` sont implémentés dans `App/Pid.h/.cpp`
**When** les gains sont modifiés
**Then** l'agressivité angulaire et linéaire sont indépendantes — tuning découplé, les anti-windups clampent à `Config::PID_I_MAX_SPEED` et `Config::PID_I_MAX_ANGLE` respectivement

**Given** `errDist < Config::ARRIVAL_THRESHOLD`
**When** la condition d'arrivée est détectée
**Then** `IMotorHAL::setMotors(0, 0)` est appelé ET `bus_->publish(Topic::ALERT, BusFormat::evtArrival())` notifie `MotionPlanner` — `OdoControl` reste stateless entre waypoints

**Given** `OdoControl` publie la télémétrie au rythme `TELEM_DIVIDER` (200Hz/10 = 20Hz)
**When** `bus_->publish(Topic::TELEMETRY, BusFormat::telOdo(x, y, angle))` est appelé
**Then** aucun `snprintf` inline n'est présent dans `OdoControl` — toute construction de message passe par `BusFormat::`

**Given** `Tests/Unit/OdoControlTest.cpp` existe avec `MockOdomHAL`, `MockMotorHAL`, `MockBus`
**When** les tests s'exécutent sur PC host
**Then** sont validés : séquence PID, anti-windup (intégrale ≤ I_MAX), condition d'arrivée, guard `_hasSetpoint` au boot, clamp MAX_DUTY — sans hardware STM32

**And** `OdoControl::OdoSnapshot latestSnapshot` (struct statique publique avec `x`, `y`, `angle`, `speedLeft`, `speedRight`, `timestamp`) est mise à jour à chaque bloc `tickCount % TELEM_DIVIDER` — lisible par `Monitoring` sans synchronisation explicite (écriture atomique ARM Cortex-M)

**And** `atan2f` (float) est utilisé exclusivement — jamais `atan2` (double) ; la FPU STM32F303 traite `atan2f` en ~0.3µs (0.003% du budget 5ms)

### Story 2.3: MotionPlanner — tâche event-driven, mailbox consigne Setpoint, arrêt d'urgence

En tant que développeur,
Je veux que `MotionPlanner` reçoive des consignes de mouvement depuis `ExternalComm` via mailbox, les transmette à `OdoControl` sous forme de `Setpoint`, et s'arrête immédiatement sur alarme via `xQueueReset()`,
Afin que le robot réponde aux commandes PC avec une latence minimale et s'arrête en O(1) sur alarme.

**Critères d'acceptation :**

**Given** `ExternalComm` reçoit `CMD MOVE 1.0 0.5 0.0\n` sur UART (x, y, angle cible)
**When** la consigne est parsée et écrite dans la mailbox via `xQueueOverwrite`
**Then** `MotionPlanner` construit un `Setpoint{Mode::POSE, PoseTarget{1.0f, 0.5f, 0.0f}}` et l'écrit dans la mailbox OdoControl dans le tick FreeRTOS suivant

**Given** `OdoControl` publie `BusFormat::evtArrival()` sur IBus
**When** `MotionPlanner` reçoit l'événement d'arrivée
**Then** il peut envoyer le prochain waypoint ou rester idle — la logique de séquence appartient exclusivement à `MotionPlanner`

**Given** `MotionPlanner` reçoit un `xTaskNotify` d'alarme
**When** le bitmask d'alarme est évalué
**Then** `xQueueReset()` est appelé sur la mailbox de consigne OdoControl — les moteurs s'arrêtent au prochain tick OdoControl (guard `_hasSetpoint`)

**Given** `MotionPlanner` déclenche un arrêt d'urgence
**When** l'alarme est traitée
**Then** `bus_->publish(Topic::ALERT, BusFormat::altAlarm(...))` est appelé → `ExternalComm` route vers PC

**Given** `Tests/Unit/MotionPlannerTest.cpp` existe
**When** les tests s'exécutent sur PC host avec `MockBus` et queues FreeRTOS simulées
**Then** le routage consigne `Setpoint`, la réception d'arrivée et l'arrêt d'urgence sont validés sans hardware

**And** `MotionPlanner` ne connaît pas `ExternalComm` directement — toute sortie passe par `IBus::publish()`

### Story 2.4: Robustesse OdoControl — détection blocage moteur, encodeur silencieux, guard boot

En tant que développeur,
Je veux qu'`OdoControl` détecte les défaillances moteur et encodeur et publie des alertes IBus, avec un comportement sûr au démarrage,
Afin que le système soit intrinsèquement défensif et que les modes de défaillance soient traçables sans intervention matérielle.

**Critères d'acceptation :**

**Given** `|duty| > Config::STALL_DUTY_THRESHOLD` et vitesse mesurée ≈ 0 pendant `Config::STALL_TIME_MS`
**When** `OdoControl` détecte la condition de blocage moteur
**Then** `bus_->publish(Topic::ALERT, BusFormat::altStall())` est publié + `IMotorHAL::setMotors(0, 0)` + reset intégral PID — le reset intégral évite les oscillations au redémarrage

**Given** `|duty| > Config::STALL_DUTY_THRESHOLD` et `|ΔticksL| == 0` (ou `|ΔticksR| == 0`) pendant N ticks consécutifs
**When** `OdoControl` détecte la condition encodeur silencieux
**Then** `bus_->publish(Topic::ALERT, BusFormat::altEncoderFault("LEFT"))` (ou `"RIGHT"`) est publié — commande élevée + zéro ticks est physiquement impossible sauf défaillance

**Given** `_hasSetpoint == false` au boot (avant que `MotionPlanner` n'ait écrit dans la mailbox)
**When** `xQueuePeek` retourne `pdFALSE`
**Then** `IMotorHAL::setMotors(0, 0)` est appelé et les PIDs ne sont pas calculés — comportement sûr garanti dès le premier tick

**Given** `OdoControl` clamp sa sortie PID à `[-Config::MAX_DUTY, +Config::MAX_DUTY]` avant `IMotorHAL::setMotors()`
**When** le clamping est actif (saturation PID)
**Then** la valeur non-clampée est loggée dans `OdoSnapshot` pour détecter des gains trop agressifs — le driver `Drv8262` clamp à `[-1, 1]` en filet de sécurité ultime

**Given** `Tests/Unit/OdoControlTest.cpp` couvre les scénarios de robustesse
**When** les tests injectent des ticks nuls avec duty élevé pendant `STALL_TIME_MS`
**Then** `MockBus` capture les alertes `ALT STALL` et `ALT ENCODER_FAULT` attendues — tests passent sur PC host sans hardware

---

## Epic 3: Écosystème Capteurs-Actionneurs

Un développeur peut intégrer jusqu'à 15 capteurs et 10 actionneurs avec alarmes et monitoring temps-réel — ajout capteur = nouvelle classe ISensor uniquement, alarmes ≤1 tick FreeRTOS, actionneurs commandés par ASCII, télémétrie surveillée par Monitoring.

### Story 3.1: SensorManager — polling ISensor[MAX_SENSORS], alarmes xTaskNotify

En tant que développeur,
Je veux que `SensorManager` orchestre jusqu'à 15 capteurs `ISensor*` injectés et envoie des alarmes critiques à `MotionPlanner` via `xTaskNotify`,
Afin que l'ajout d'un nouveau capteur ne modifie aucune classe existante et que les alarmes arrivent en ≤1 tick FreeRTOS.

**Critères d'acceptation :**

**Given** `SensorManager` reçoit un tableau de `ISensor*[MAX_SENSORS]` injecté par `SystemInit`
**When** la tâche poll tous les capteurs via `ISensor::read()`
**Then** `SensorManager` ne connaît pas les types concrets — aucun `static_cast` ou `dynamic_cast` dans son code

**Given** un capteur retourne une valeur dépassant un seuil critique
**When** `SensorManager` détecte l'alarme
**Then** `xTaskNotify(motionPlannerHandle, alarmBit, eSetBits)` est appelé — latence ≤1 tick FreeRTOS mesurée sur cible

**Given** `Tests/Unit/SensorManagerTest.cpp` existe
**When** 15 mocks `ISensor` sont instanciés et injectés
**Then** le polling et le déclenchement d'alarme sont validés — tests passent sur PC host

**And** `SensorManager` publie la télémétrie santé via `bus_->publish(Topic::HEALTH, BusFormat::hlt*(...))`

**And** `SensorManager::SensorSnapshot latestSnapshot` (struct statique publique avec état de chaque capteur, dernière valeur lue, `timestamp`) est mise à jour à chaque cycle de polling — lisible par `Monitoring` sans synchronisation explicite

### Story 3.2: ActuatorManager — IActuator[MAX_ACTUATORS], commandById

En tant que développeur,
Je veux qu'`ActuatorManager` orchestre jusqu'à 10 actionneurs `IActuator*` et expose `commandById(id, cmd)` à `ExternalComm`,
Afin que les actionneurs soient commandés via ASCII sans qu'`ExternalComm` connaisse les types d'actionneurs concrets.

**Critères d'acceptation :**

**Given** `ActuatorManager` implémente `IActuatorManager` avec `commandById(uint8_t id, const char* cmd)`
**When** `ExternalComm` reçoit `CMD ACTUATOR PUMP_1 ON\n`
**Then** `ActuatorManager::commandById` route la commande vers le bon `IActuator*` par son id

**Given** `ActuatorManager` reçoit un `id` inexistant
**When** `commandById` est appelé
**Then** `bus_->publish(Topic::LOG, BusFormat::logWarn("unknown actuator id"))` est appelé — aucun crash, aucun accès hors-tableau

**Given** `Tests/Unit/ActuatorManagerTest.cpp` existe avec 10 mocks `IActuator`
**When** les tests de routage par id s'exécutent
**Then** chaque commande atteint le bon mock — tests passent sur PC host

**And** `ActuatorManager` ne connaît pas `ExternalComm` — il reçoit `IBus*` par injection de constructeur

### Story 3.3: Monitoring — supervision par pull sur shared memory horodatée

En tant que développeur,
Je veux que `Monitoring` poll périodiquement les snapshots statiques de `OdoControl`, `SensorManager` et `ExternalComm` pour détecter les données obsolètes ou les seuils dépassés et publie des alertes sur IBus,
Afin d'avoir une supervision système centralisée sans subscribe IBus et sans que les modules ne connaissent `Monitoring`.

**Critères d'acceptation :**

**Given** `Monitoring` lit `OdoControl::latestSnapshot.timestamp` à chaque cycle de polling
**When** `HAL_GetTick() - latestSnapshot.timestamp > Config::MONITORING_STALE_MS`
**Then** `bus_->publish(Topic::ALERT, BusFormat::altStale("ODO"))` est appelé — donnée odométrique considérée obsolète

**Given** `Monitoring` lit `SensorManager::latestSnapshot` et `ExternalComm::latestSnapshot`
**When** le timestamp de l'une de ces structs dépasse `Config::MONITORING_STALE_MS`
**Then** une alerte correspondante est publiée sur `IBus(ALERT)` avec le module concerné

**Given** `MonitoringTest.cpp` injecte des snapshots avec timestamps artificiels
**When** `HAL_GetTick()` est mocké et le delta simulé dépasse le seuil
**Then** `MockBus` capture l'alerte attendue — test valide sans hardware STM32

**Given** `Monitoring` copie une struct de snapshot pour l'inspecter
**When** la copie est effectuée (lecture multi-champs susceptible d'être préemptée)
**Then** elle est encadrée par `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` — vérifié en code review et dans `MonitoringTest.cpp` via mock de la section critique

**And** `Monitoring` ne souscrit pas à IBus — IBus est publish-only; Monitoring lit uniquement les structs statiques des modules connus (`OdoControl`, `SensorManager`, `ExternalComm`)

### Story 3.4: Drivers concrets — ISensor et IActuator

> ⚠️ **Note charge :** cette story couvre 6 implémentations (`ProximitySensor`, `TemperatureSensor`, `CurrentSensor`, `Pump`, `Servo`, `LinearTransducer`). Si le sprint est serré, la découper en 3.4a (capteurs) + 3.4b (actionneurs).

En tant que développeur,
Je veux six drivers concrets implémentant `ISensor` ou `IActuator` dans `App/Drivers/`,
Afin de valider le pattern d'extensibilité : chaque driver est une nouvelle classe uniquement, zéro modification des classes existantes.

**Critères d'acceptation :**

**Given** `ProximitySensor`, `TemperatureSensor`, `CurrentSensor` implémentent `ISensor`
**When** chacun est instancié et injecté dans `SensorManager` par `SystemInit`
**Then** `SensorManager` les orchestre sans modification de son code

**Given** `Pump`, `Servo`, `LinearTransducer` implémentent `IActuator`
**When** chacun est instancié et injecté dans `ActuatorManager` par `SystemInit`
**Then** `ActuatorManager` les route sans modification de son code

**Given** un driver concret est ajouté
**When** `grep -r "new\|delete" App/Drivers/` est exécuté
**Then** zéro occurrence — les drivers n'allouent pas dynamiquement

**And** chaque driver accède au hardware via une interface HAL injectée (`ISensorHAL*`) — jamais d'appel direct à HAL CubeMX

### Story 3.5: Validation d'intégration système — suite de tests complète sur PC host

En tant que développeur,
Je veux une suite de tests Google Test globale couvrant tous les modules `App/` des Epics 1, 2 et 3,
Afin de valider que l'ensemble du système est testable en chaîne sur PC host sans hardware STM32.

**Critères d'acceptation :**

**Given** tous les modules des Epics 1, 2 et 3 sont implémentés
**When** `make test` s'exécute depuis `Tests/` sur PC host (toolchain x86, Google Test)
**Then** la commande se termine avec exit code 0 — zéro test en échec, zéro test ignoré

**Given** `Tests/Unit/` contient un fichier de test par classe métier
**When** la liste des fichiers est vérifiée
**Then** chaque classe dans `App/Tasks/`, `App/Drivers/`, `App/SystemInit/` possède un `*Test.cpp` correspondant

**Given** les tests s'exécutent avec mocks HAL injectés (`MockBus`, `MockMotorHAL`, `MockSensorHAL`, etc.)
**When** aucun hardware STM32 n'est connecté
**Then** 100% des classes `App/` sont instanciables et testées — aucune dépendance HAL CubeMX directe (NFR-03)

**And** `grep -r "HAL_\|osDelay\|vTaskDelay\|xTaskCreate" Tests/Unit/` retourne zéro occurrence — les tests ne dépendent d'aucune primitive FreeRTOS ou HAL réelle
