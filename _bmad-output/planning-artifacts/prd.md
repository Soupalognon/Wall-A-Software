---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-02b-vision', 'step-02c-executive-summary', 'step-03-success-criteria', 'step-04-scope', 'step-05-user-journeys', 'step-06-domain-requirements', 'step-07-project-type-requirements', 'step-08-functional-requirements', 'step-09-nonfunctional-requirements', 'step-12-complete']
inputDocuments:
  - '_bmad-output/brainstorming/brainstorming-session-2026-05-09-1400.md'
  - '_bmad-output/planning-artifacts/architecture.md'
briefCount: 0
researchCount: 0
brainstormingCount: 1
projectDocsCount: 1
classification:
  projectType: 'iot_embedded'
  domain: 'process_control'
  complexity: 'high'
  projectContext: 'brownfield'
  prdFocus: 'architecture_skeleton_dev_rules'
  targetUsers: 'developers'
workflowType: 'prd'
project_name: 'bmad - robot CDR'
user_name: 'Gdurand'
date: '2026-05-10'
---

# Product Requirements Document — Robot CDR (STM32/FreeRTOS C++)

**Auteur :** Gdurand
**Date :** 2026-05-10
**Version :** 1.0

---

## Executive Summary

Le projet Robot CDR est un système embarqué autonome sur microcontrôleur STM32 avec FreeRTOS CMSIS V2 et HAL CubeMX. Ce PRD définit le **squelette architectural C++ orienté objet** et les **règles de développement non-négociables** qui gouvernent toute implémentation du système.

**Vision :** Fournir aux développeurs (humains et agents IA) un contrat architectural clair, complet et immédiatement implémentable — zéro ambiguïté sur la structure, les dépendances, les patterns de communication inter-modules et les conventions de code.

**Différenciateur :** L'architecture repose sur trois invariants qui éliminent des classes entières de bugs embarqués : (1) bus pub/sub IBus avec injection de dépendance — zéro couplage direct entre modules, (2) zéro allocation dynamique — empreinte mémoire compile-time connue, (3) tâche OdoControl isolée à 200Hz — déterminisme temps-réel garanti par construction.

**Utilisateurs cibles :** Développeurs implémentant les classes du système Robot CDR. Toute décision de ce PRD doit permettre à un développeur de créer une nouvelle classe conforme sans modifier l'architecture existante.

---

## Success Criteria

| Critère | Mesure | Cible |
|---------|--------|-------|
| Déterminisme temps-réel OdoControl | Gigue de la boucle 200Hz mesurée par oscilloscope ou timer HAL | ≤ 50µs de déviation par cycle |
| Zéro allocation dynamique | Utilisation de `new`/`delete`/`malloc` détectée à la compilation | 0 occurrence (erreur de build) |
| Publications IBus non-bloquantes | Durée d'appel `IBus::publish()` depuis une tâche haute priorité | < 1µs (overwrite/drop atomique) |
| Latence alarme capteur | Délai `SensorManager::xTaskNotify` → `MotionPlanner` actif | ≤ 1 tick FreeRTOS (5ms max) |
| Testabilité sans hardware | Couverture des tests unitaires sur host (Google Test + mocks HAL) | Toutes les classes métier testables sans STM32 connecté |
| Extensibilité capteurs/actionneurs | Ajout d'un nouveau capteur ou actionneur | 0 modification de classe existante requise |
| Compilation propre | Warnings `-Wall -Wextra` avec `-fno-exceptions -fno-rtti` | 0 warning, 0 erreur |

---

## Product Scope

### MVP — Squelette fonctionnel (Priorité 1)

Implémentation minimale permettant le démarrage FreeRTOS et la validation de l'architecture sur cible :

- `App/Config.h` — toutes les constantes `constexpr` (fréquences, stacks, priorités, PID defaults)
- `App/Interfaces/` — contrats `IBus`, `ISensor`, `IActuator`, `IActuatorManager`
- `App/BusFormat.h/.cpp` — formatage ASCII centralisé pour tous les topics IBus
- `App/Tasks/ExternalComm` — implémentation IBus + parsing CMD + routage UART/USB/ETH
- `App/SystemInit/` — câblage statique complet, zéro `new`, `main.cpp` = 3 lignes
- `App/Tasks/OdoControl` — tâche 200Hz, PID, encodeurs gauche/droite
- Projet STM32CubeMX configuré pour la cible avec FreeRTOS CMSIS V2

### Growth — Modules métier complets (Priorité 2)

- `App/Tasks/MotionPlanner` — planification trajectoire, réaction alarmes via `xTaskNotify`
- `App/Tasks/SensorManager` — polling configurable, gestion jusqu'à 15 capteurs `ISensor`
- `App/Tasks/Monitoring` — agrégation télémétrie, seuils, alertes via IBus
- `App/Tasks/ActuatorManager` — gestion jusqu'à 10 actionneurs `IActuator`
- Drivers concrets : `ProximitySensor`, `TemperatureSensor`, `CurrentSensor`, `Pump`, `Servo`, `LinearTransducer`
- Suite de tests unitaires Google Test couvrant toutes les classes métier

### Vision — Validation système complète (Priorité 3)

- Tests d'intégration sur cible via UART/USB
- Validation de la boucle fermée odométrie + PID sur hardware réel
- Migration protocole ASCII → binaire si débit insuffisant (décision post-validation)

---

## User Journeys

### Journey 1 — Ajouter un nouveau capteur

**Acteur :** Développeur
**Précondition :** `ISensor` et `SensorManager` implémentés

1. Créer `App/Drivers/MonNouveauCapteur.h/.cpp` implémentant `ISensor` (id, name, read())
2. Ajouter l'instanciation statique dans `SystemInit::boot()` et l'injecter dans `SensorManager`
3. Écrire `Tests/Unit/MonNouveauCapteurTest.cpp` avec mock `ISensorHAL` injecté
4. Compiler et tester sur host sans hardware

**Résultat attendu :** Zéro modification de `SensorManager`, `MotionPlanner`, `ExternalComm` ou tout autre module existant.

### Journey 2 — Envoyer de la télémétrie depuis un module

**Acteur :** Développeur
**Précondition :** `IBus` injecté dans la classe via constructeur

1. Appeler `bus_->publish(Topic::TELEMETRY, BusFormat::telXxx(...))` — jamais de `snprintf` inline
2. `ExternalComm::txTask` consomme et route vers USB/ETH automatiquement

**Résultat attendu :** Le module ne connaît pas `ExternalComm`. Testable via `MockBus` sans HAL.

### Journey 3 — Déclencher un arrêt d'urgence

**Acteur :** SensorManager (alarme capteur critique)
**Précondition :** `MotionPlanner` tâche active

1. `SensorManager` appelle `xTaskNotify(motionPlannerHandle, ALARM_BIT, eSetBits)`
2. `MotionPlanner` se réveille en ≤ 1 tick, évalue le bitmask d'alarme
3. `MotionPlanner` appelle `xQueueReset()` sur la mailbox de consigne — arrêt moteurs immédiat
4. `MotionPlanner` publie sur `IBus(ALERT)` → `ExternalComm` → PC

**Résultat attendu :** Latence ≤ 1 tick FreeRTOS, zéro copie, aucun module bloqué.

### Journey 4 — Recevoir une commande mouvement depuis le PC

**Acteur :** Opérateur terrain (via UART) ou supervision PC (via USB/ETH)
**Précondition :** `ExternalComm` opérationnel

1. PC envoie `CMD MOVE 0.5 0.3\n` sur UART (prioritaire) ou USB/ETH
2. `ExternalComm::rxTask` parse avec `sscanf`, dispatch sur token `CMD`
3. `ExternalComm` écrit dans la mailbox de consigne via `xQueueOverwrite`
4. `MotionPlanner` lit la consigne et la transfère à `OdoControl`

**Résultat attendu :** UART écrase toute consigne simultanée USB/ETH.

---

## Project-Type Requirements (iot_embedded)

### Contraintes hardware et plateforme

- **Cible :** STM32 (référence exacte définie dans le projet CubeMX)
- **RTOS :** FreeRTOS CMSIS V2 — API exclusivement via CMSIS wrappers
- **HAL :** STM32 CubeMX généré — ne jamais modifier les fichiers `Core/` et `Drivers/` manuellement
- **Langage :** C++17, `-fno-exceptions`, `-fno-rtti`, `-Wall`, `-Wextra`
- **Mémoire :** Statique uniquement. `new`, `delete`, `malloc`, `free` interdits. Violation = erreur de build.
- **Périphériques configurés dans CubeMX :** TIM (encodeurs × 2), PWM (moteurs × 2), UART, USB CDC, Ethernet

### Point d'entrée système

`main.cpp` contient exactement 3 lignes : `SystemInit::boot()` + `vTaskStartScheduler()` + boucle vide. Toute initialisation se fait dans `SystemInit`.

### Tests sans hardware (obligatoire)

Toutes les classes métier (`App/`) sont testables sur host PC via Google Test avec mocks HAL injectés par constructeur. Aucune classe métier ne peut appeler directement une fonction HAL CubeMX.

---

## Functional Requirements

### FR-01 — Contrôle moteur en boucle fermée

Les développeurs peuvent implémenter `OdoControl` comme unique tâche FreeRTOS exécutant séquentiellement : lecture encodeurs → calcul odométrie → boucle PID → commande moteurs, à cadence fixe 200Hz via `vTaskDelayUntil`. `OdoControl` est le seul consommateur de `IEncoderHAL` et `IMotorHAL`.

**Critère de test :** La tâche s'exécute en moins de 4ms (80% du budget 5ms) mesurée par timer HAL.

### FR-02 — Consigne de mouvement via mailbox FreeRTOS

`MotionPlanner` écrit la consigne de vitesse dans une queue FreeRTOS depth-1 via `xQueueOverwrite`. `OdoControl` lit via `xQueuePeek` à chaque tick sans jamais bloquer. `xQueueReset()` vide la consigne (arrêt d'urgence) en O(1).

**Critère de test :** `xQueuePeek` retourne en < 1µs mesurée par DWT cycle counter.

### FR-03 — Bus pub/sub IBus multi-topic

`ExternalComm` implémente `IBus`. Quatre topics : `TELEMETRY`, `ALERT`, `LOG`, `HEALTH`. Politique par topic configurable dans `BUS_CONFIG[]` (overwrite ou drop-silent). Toute classe reçoit `IBus*` par injection de constructeur.

**Critère de test :** `MockBus` capture les publications dans les tests unitaires sans hardware.

### FR-04 — Communication externe tri-canal ASCII

`ExternalComm` gère UART (priorité commande terrain), USB (supervision haute fréquence), Ethernet (supervision réseau). Format ASCII : `TOPIC PAYLOAD\n`. En cas de commande simultanée, UART écrase. Parsing par `sscanf` sur premier token.

**Critère de test :** `ExternalCommTest.cpp` valide le parsing de tous les messages CMD avec mock UART.

### FR-05 — Gestion capteurs extensible

`SensorManager` orchestre jusqu'à `MAX_SENSORS=15` instances `ISensor*` injectées par `SystemInit`. Ajout d'un capteur = nouvelle classe concrète `ISensor` uniquement. `SensorManager` ne connaît pas les types concrets.

**Critère de test :** `SensorManagerTest.cpp` instancie 15 mocks `ISensor` et valide le polling.

### FR-06 — Gestion actionneurs extensible

`ActuatorManager` implémente `IActuatorManager`, orchestre jusqu'à `MAX_ACTUATORS=10` instances `IActuator*`. `ExternalComm` commande via `IActuatorManager::commandById(id, cmd)`. `ActuatorManager` ne connaît pas `ExternalComm`.

**Critère de test :** Commande `CMD ACTUATOR PUMP_1 ON\n` route vers le bon `IActuator` via mock.

### FR-07 — Alarmes capteurs à faible latence

`SensorManager` envoie `xTaskNotify(motionPlannerHandle, bitmask, eSetBits)` lors d'une alarme critique. Chaque bit du bitmask correspond à un type d'alarme. `MotionPlanner` se réveille sans polling.

**Critère de test :** Latence notify → réveil ≤ 1 tick FreeRTOS mesurée sur cible.

### FR-08 — Monitoring par pull sur shared memory horodatée

`Monitoring` accède aux données des autres modules via des structs statiques publiques horodatées (`OdoSnapshot`, `SensorSnapshot`, `CommSnapshot`). Chaque module qui expose des données de santé déclare une telle struct mise à jour à chaque cycle. `Monitoring` poll périodiquement et vérifie le timestamp : si `HAL_GetTick() - snapshot.timestamp > Config::MONITORING_STALE_MS`, une alerte est publiée sur `IBus(ALERT)`. IBus reste un canal publish-only — pas de subscribe.

**Critère de test :** `MonitoringTest.cpp` injecte des snapshots avec timestamps artificiels et valide que l'alerte est publiée sur `MockBus` après dépassement du seuil.

### FR-09 — Formatage IBus centralisé

Toutes les publications IBus utilisent `BusFormat::` (`BusFormat::telOdo()`, `BusFormat::altProximity()`, etc.). `snprintf` inline dans les classes métier est interdit.

**Critère de test :** Grep sur `App/` ne retourne aucun `snprintf` en dehors de `BusFormat.cpp`.

### FR-10 — Configuration compile-time centralisée

Toutes les constantes système (fréquences, tailles de buffer, stacks FreeRTOS, priorités, PID defaults) sont déclarées `constexpr` dans `App/Config.h`. Aucune magic number dans les fichiers de classes.

**Critère de test :** Grep sur `App/Tasks/` ne retourne aucune valeur numérique littérale pour stacks ou priorités.

### FR-11 — Câblage statique unique dans SystemInit

`SystemInit::boot()` instancie tous les objets en mémoire statique et injecte toutes les dépendances. Aucune autre classe n'instancie un objet métier. Zéro `new`.

**Critère de test :** Grep sur `App/` ne retourne aucun `new` ou `delete` en dehors de `SystemInit.cpp`.

---

## Non-Functional Requirements

### NFR-01 — Déterminisme temps-réel

La tâche `OdoControl` s'exécute à 200Hz (5ms ± 50µs) sans interruption par des tâches de priorité inférieure. Les publications IBus depuis `OdoControl` sont non-bloquantes (overwrite) — aucun appel bloquant sur le chemin critique.

### NFR-02 — Empreinte mémoire prévisible

L'empreinte RAM totale du firmware est connue à la compilation. Aucune allocation dynamique. Les tableaux ont des tailles fixes (`MAX_SENSORS=15`, `MAX_ACTUATORS=10`). Les stacks FreeRTOS sont définies dans `Config.h` et validées par le linker.

### NFR-03 — Testabilité sur host

100% des classes dans `App/Tasks/`, `App/Drivers/`, `App/SystemInit/` sont instanciables sur PC x86 avec mocks injectés. Les tests Google Test compilent et s'exécutent sans toolchain ARM ni hardware STM32.

### NFR-04 — Sécurité inter-tâches par construction

Les mécanismes de communication inter-tâches autorisés sont :

- `IBus::publish()`, `xTaskNotify()`, `xQueueOverwrite()`, `xQueuePeek()` — communication générale
- Lecture de structs statiques publiques horodatées (`OdoSnapshot`, `SensorSnapshot`, `CommSnapshot`) — **exclusivement pour `Monitoring`**, protégée par `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` côté lecteur

Le writer (tâche haute priorité) n'a pas besoin de section critique : il ne peut pas être préempté par `Monitoring` (priorité inférieure). Le reader (`Monitoring`) doit impérativement protéger la copie de struct pour éviter un torn read lors d'une préemption en pleine lecture.

Toute autre forme d'accès mémoire inter-tâches est une erreur de conception détectable à la review.

### NFR-05 — Extensibilité sans régression

L'ajout d'un capteur, actionneur ou module de communication ne modifie aucune classe existante. L'interface (`ISensor`, `IActuator`, `IBus`) est le seul contrat. `SystemInit` est le seul fichier à modifier pour câbler un nouveau composant.

### NFR-06 — Conformité conventions de code

Membres privés préfixés `_`. Constantes `ALL_CAPS` dans `namespace Config`. Interfaces préfixées `I`. Fichiers nommés identiquement à la classe (PascalCase). Guards `#ifndef APP_<DOSSIER>_<FICHIER>_H`. Violations détectées en code review.

**Critère de test :** Grep sur `App/` ne retourne aucune violation des patterns ci-dessus.

### NFR-07 — Périmètre de développement : `App/` et `Tests/` uniquement

Tout code développé manuellement réside exclusivement dans `App/` et `Tests/`. Les dossiers `Core/`, `Drivers/` et `Middlewares/` sont autogénérés par CubeMX et ne doivent jamais être modifiés manuellement. Toute modification hors `App/` est une erreur de conception.

**Critère de test :** Les commits ne contiennent aucune modification dans `Core/`, `Drivers/` ou `Middlewares/` (hors régénération CubeMX explicite).

### NFR-08 — Convention de langue : anglais exclusif

Tout identifiant est en anglais sans exception : noms de classes, méthodes, variables, membres, constantes, fichiers, guards, topics IBus, messages ASCII. La langue de documentation (commentaires, PRD, architecture) reste indépendante de cette règle.

---

## Architecture Reference

Ce PRD est dérivé de [`_bmad-output/planning-artifacts/architecture.md`](_bmad-output/planning-artifacts/architecture.md) qui contient :
- Le graphe de dépendances complet entre toutes les classes
- La structure de répertoires complète (`robot-cdr/`)
- Le protocole de communication ASCII avec exemples de messages
- Les règles d'implémentation obligatoires avec exemples de code
- La séquence d'implémentation recommandée
- La validation de cohérence architecturale

Toute implémentation doit être conforme à ce document d'architecture. En cas de contradiction entre ce PRD et `architecture.md`, `architecture.md` fait foi pour les détails d'implémentation.
