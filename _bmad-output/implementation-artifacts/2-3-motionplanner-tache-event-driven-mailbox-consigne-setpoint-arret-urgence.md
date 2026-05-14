# Story 2.3: MotionPlanner — tâche event-driven, mailbox consigne Setpoint, arrêt d'urgence

Status: review

## Story

En tant que développeur,
Je veux que `MotionPlanner` reçoive des consignes de mouvement depuis `ExternalComm` via mailbox, les transmette à `OdoControl` sous forme de `Setpoint`, et s'arrête immédiatement sur alarme via `xQueueReset()`,
Afin que le robot réponde aux commandes PC avec une latence minimale et s'arrête en O(1) sur alarme.

## Acceptance Criteria

1. **Given** `ExternalComm` reçoit `CMD MOVE 1.0 0.5 0.0\n` sur UART (x, y, angle cible)
   **When** la consigne est parsée et écrite dans la mailbox via `xQueueOverwrite`
   **Then** `MotionPlanner` construit un `Setpoint{SetpointMode::POSE, PoseTarget{1.0f, 0.5f, 0.0f}}` et l'écrit dans la mailbox OdoControl dans le tick FreeRTOS suivant

2. **Given** `OdoControl` publie `BusFormat::evtArrival()` sur IBus
   **When** `MotionPlanner` n'a plus de consigne active (OdoControl a effacé `_hasSetpoint`)
   **Then** `MotionPlanner` peut rester idle ou attendre le prochain CMD MOVE — la logique de séquence appartient exclusivement à `MotionPlanner`

3. **Given** `MotionPlanner` reçoit un `xTaskNotify` d'alarme de `SensorManager`
   **When** le bitmask d'alarme est évalué
   **Then** `xQueueReset()` est appelé sur la mailbox de consigne OdoControl — les moteurs s'arrêtent au prochain tick OdoControl (guard `_hasSetpoint`)

4. **Given** `MotionPlanner` déclenche un arrêt d'urgence
   **When** l'alarme est traitée
   **Then** `_bus->publish(Topic::ALERT, BusFormat::altAlarm(bitmask))` est appelé → `ExternalComm` route vers PC

5. **Given** `Tests/Unit/MotionPlannerTest.cpp` existe
   **When** les tests s'exécutent sur PC host avec `MockBus` et queues FreeRTOS simulées
   **Then** le routage consigne `Setpoint`, le cas sans consigne (idle) et l'arrêt d'urgence sont validés sans hardware

6. **And** `MotionPlanner` ne connaît pas `ExternalComm` directement — toute sortie passe par `IBus::publish()`

## Tasks / Subtasks

- [x] Task 1: Ajouter `BusFormat::altAlarm(uint32_t bitmask)` (AC: #4)
  - [x] Déclaration dans `App/BusFormat.h` : `static const char* altAlarm(uint32_t bitmask);`
  - [x] Implémentation dans `App/BusFormat.cpp` : buffer statique, format `"ALT ALARM 0x%08X\n"` via `snprintf`
  - [x] Guard: cette fonction est la SEULE à utiliser `snprintf` dans BusFormat — les autres méthodes existantes utilisent des littéraux statiques

- [x] Task 2: Créer `App/Tasks/MotionPlanner.h` (AC: #1, #2, #3, #4)
  - [x] Guard: `#ifndef APP_TASKS_MOTIONPLANNER_H`
  - [x] Includes: `"Interfaces/IBus.h"`, `"Tasks/OdoControl.h"` (pour `Setpoint`, `SetpointMode`, `PoseTarget`), `<FreeRTOS.h>`, `<task.h>`, `<queue.h>`, `<cstdint>`
  - [x] Définir `struct MoveCmd { float x; float y; float angle; };` — struct utilisée par ExternalComm → MotionPlanner via `_cmdMailbox`
  - [x] Classe MotionPlanner :
    ```cpp
    class MotionPlanner {
    public:
        MotionPlanner(IBus* bus, QueueHandle_t cmdMailbox, QueueHandle_t setpointMailbox);
        static void task(void* param);
        // Exposed for unit testing
        void processCmd(const MoveCmd& cmd);
        void handleAlarm(uint32_t bitmask);
    private:
        IBus*         _bus;
        QueueHandle_t _cmdMailbox;
        QueueHandle_t _setpointMailbox;
    };
    ```

- [x] Task 3: Créer `App/Tasks/MotionPlanner.cpp` (AC: #1, #3, #4)
  - [x] Includes: `"Tasks/MotionPlanner.h"`, `"BusFormat.h"`
  - [x] Constructeur: initialise `_bus`, `_cmdMailbox`, `_setpointMailbox`
  - [x] `task()` — boucle event-driven (voir Dev Notes pour l'implémentation complète)
  - [x] `processCmd()` — construit `Setpoint{SetpointMode::POSE, {cmd.x, cmd.y, cmd.angle}}`, appelle `xQueueOverwrite(_setpointMailbox, &sp)`
  - [x] `handleAlarm()` — appelle `xQueueReset(_setpointMailbox)` puis `_bus->publish(Topic::ALERT, BusFormat::altAlarm(bitmask))`

- [x] Task 4: Ajouter `xTaskNotifyWait` aux stubs de test (AC: #5)
  - [x] Ajouter dans `Tests/Stubs/task.h` : déclaration de `xTaskNotifyWait`
  - [x] Ajouter dans `Tests/Stubs/FreeRTOSStub.cpp` : stub retournant `pdFALSE` (aucune notification en attente)

- [x] Task 5: Mettre à jour `App/Tasks/ExternalComm.cpp` — parser CMD MOVE avec 3 floats (AC: #1)
  - [x] Ajouter `#include "Tasks/MotionPlanner.h"` au debut de `ExternalComm.cpp` pour accéder à `MoveCmd`
  - [x] Dans `_processRxLine()`, modifier le bloc `CMD MOVE` :
    - Avant (BUGUÉ pour Story 2.3) : `sscanf(..., "%f %f", &left, &right)` — 2 floats = duty cycles
    - Après (CORRECT) : `sscanf(..., "%f %f %f", &x, &y, &angle)` — 3 floats = pose cible
    - Écrire `MoveCmd{x, y, angle}` via `xQueueOverwrite(_motionMailbox, &cmd)`

- [x] Task 6: Mettre à jour `App/SystemInit/SystemInit.cpp` — câbler MotionPlanner (AC: #1, #3)
  - [x] Ajouter `#include "Tasks/MotionPlanner.h"` — fournit aussi `MoveCmd`
  - [x] Supprimer la définition de `struct RawMoveCmd { float left; float right; }` — remplacée par `MoveCmd` depuis `MotionPlanner.h`
  - [x] Changer la création de `cmdMailbox` : `xQueueCreate(1, sizeof(MoveCmd))` au lieu de `sizeof(RawMoveCmd)` — **CRITIQUE : la taille change de 8 à 12 bytes**
  - [x] Instancier `MotionPlanner` : `MotionPlanner motionPlanner { &extComm, cmdMailbox, setpointMailbox };`
  - [x] Capturer le handle de la tâche : `TaskHandle_t motionPlannerHandle = nullptr;` (extern dans SystemInit.h)
  - [x] Dans `boot()`, créer la tâche : `xTaskCreate(MotionPlanner::task, "MoPlan", Config::STACK_MOTION_PLANNER, &motionPlanner, Config::PRIO_MOTION_PLANNER, &motionPlannerHandle);`
  - [x] Le handle `motionPlannerHandle` sera transmis à `SensorManager` dans Story 3.1 — exposé via `extern` dans SystemInit.h

- [x] Task 7: Créer `Tests/Unit/MotionPlannerTest.cpp` (AC: #5)
  - [x] Voir Dev Notes pour les 5 cas de test complets

- [x] Task 8: Mettre à jour `Tests/CMakeLists.txt` — ajouter MotionPlannerTest (AC: #5)
  - [x] Ajouter executable `MotionPlannerTest` avec sources : `Unit/MotionPlannerTest.cpp`, `${APP_DIR}/Tasks/MotionPlanner.cpp`, `${APP_DIR}/BusFormat.cpp`, `Stubs/FreeRTOSStub.cpp`
  - [x] `target_include_directories` avec `${COMMON_INCLUDES}`
  - [x] `target_link_libraries` avec `GTest::gtest_main`
  - [x] `gtest_discover_tests(MotionPlannerTest)`

- [x] Task 9: Vérifier conformité NFR
  - [x] `grep -r "snprintf" App/Tasks/MotionPlanner.cpp` → zéro résultat (snprintf uniquement dans BusFormat.cpp)
  - [x] `grep -r "new\|delete\|malloc\|free" App/Tasks/MotionPlanner.h App/Tasks/MotionPlanner.cpp` → zéro résultat
  - [x] `grep -rn "[àâçèéêëîïôùûüÿœæ]" App/Tasks/MotionPlanner.h App/Tasks/MotionPlanner.cpp` → zéro résultat

## Dev Notes

### Scope — Fichiers créés/modifiés par cette story

```
App/
├── BusFormat.h                        ← UPDATE (add altAlarm declaration)
├── BusFormat.cpp                      ← UPDATE (add altAlarm implementation)
├── Tasks/
│   ├── MotionPlanner.h                ← NEW (MoveCmd struct + MotionPlanner class)
│   └── MotionPlanner.cpp              ← NEW (task loop, processCmd, handleAlarm)
└── SystemInit/
    └── SystemInit.cpp                 ← UPDATE (wire MotionPlanner, fix cmdMailbox size)

Tests/
├── CMakeLists.txt                     ← UPDATE (add MotionPlannerTest target)
├── Stubs/
│   ├── task.h                         ← UPDATE (add xTaskNotifyWait declaration)
│   └── FreeRTOSStub.cpp               ← UPDATE (add xTaskNotifyWait stub)
└── Unit/
    └── MotionPlannerTest.cpp          ← NEW (5 test cases)
```

**Ne pas toucher :** `App/Interfaces/`, `App/Drivers/`, `App/Tasks/OdoControl.*`, `App/Tasks/Pid.*`, `App/Tasks/ExternalComm.h`, `Core/`, `Middlewares/`

---

### BREAKING CHANGE CRITIQUE — cmdMailbox item size

**ExternalComm.cpp actuel (Story 2.2, INCORRECT pour Story 2.3) :**
```cpp
// _processRxLine — bloc CMD MOVE actuel
float left = 0.0f, right = 0.0f;
sscanf(line, "%*s %*s %f %f", &left, &right);
struct { float left; float right; } sp = { left, right };
xQueueOverwrite(_motionMailbox, &sp);
```
Ce code écrit 8 bytes (2 floats). Le protocole câble des **duty cycles** directement.

**Après (CORRECT pour Story 2.3) :**
```cpp
// _processRxLine — bloc CMD MOVE corrigé
MoveCmd cmd{};
sscanf(line, "%*s %*s %f %f %f", &cmd.x, &cmd.y, &cmd.angle);
xQueueOverwrite(_motionMailbox, &cmd);
```
Ce code écrit 12 bytes (3 floats). Le protocole câble des **coordonnées pose cible (x, y, angle)**.

**SystemInit.cpp — correction obligatoire :**
```cpp
// Avant (8 bytes — INCORRECT après Story 2.3)
struct RawMoveCmd { float left; float right; };
static QueueHandle_t cmdMailbox = xQueueCreate(1, sizeof(RawMoveCmd));

// Après (12 bytes — CORRECT)
// RawMoveCmd supprimé — MoveCmd vient de MotionPlanner.h
static QueueHandle_t cmdMailbox = xQueueCreate(1, sizeof(MoveCmd));
```

**Si la taille de queue ne correspond pas à la struct écrite → UB sur cible ARM, crash silencieux en test.**

---

### MotionPlanner::task() — Implémentation complète

```cpp
void MotionPlanner::task(void* param) {
    auto* self = static_cast<MotionPlanner*>(param);
    for (;;) {
        uint32_t notifyVal = 0;
        // Block until alarm notification or timeout (allows periodic CMD check)
        xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, pdMS_TO_TICKS(50));

        if (notifyVal != 0) {
            self->handleAlarm(notifyVal);
        }

        // Check cmdMailbox non-blocking (written by ExternalComm via xQueueOverwrite)
        MoveCmd cmd{};
        if (xQueueReceive(self->_cmdMailbox, &cmd, 0) == pdTRUE) {
            self->processCmd(cmd);
        }
    }
}
```

**Pourquoi ce design :**
- `xTaskNotifyWait(portMAX_DELAY)` seul bloquerait jusqu'à une alarme — le CMD MOVE ne réveillerait pas la tâche
- Timeout 50ms = MotionPlanner vérifie `cmdMailbox` au moins toutes les 50ms + immédiatement sur alarme
- `xQueueOverwrite` côté ExternalComm garantit que la dernière consigne est toujours disponible
- `pdMS_TO_TICKS(50)` est indépendant de la valeur de `ODO_FREQ_HZ`

---

### MotionPlanner::processCmd() — Implémentation

```cpp
void MotionPlanner::processCmd(const MoveCmd& cmd) {
    Setpoint sp{};
    sp.mode   = SetpointMode::POSE;
    sp.pose   = { cmd.x, cmd.y, cmd.angle };
    xQueueOverwrite(_setpointMailbox, &sp);
}
```

**Note:** `xQueueOverwrite` sur `_setpointMailbox` — même primitive que Story 2.2. `OdoControl` lit via `xQueuePeek` (non-destructif), donc la consigne persiste jusqu'à remplacement ou `xQueueReset`.

---

### MotionPlanner::handleAlarm() — Implémentation

```cpp
void MotionPlanner::handleAlarm(uint32_t bitmask) {
    xQueueReset(_setpointMailbox);                              // O(1) stop
    _bus->publish(Topic::ALERT, BusFormat::altAlarm(bitmask)); // notify PC
}
```

**Séquence d'arrêt d'urgence :**
1. `SensorManager::task()` (Story 3.1) appelle `xTaskNotify(motionPlannerHandle, bit, eSetBits)`
2. `MotionPlanner::task()` se réveille, `notifyVal != 0`, appelle `handleAlarm(notifyVal)`
3. `xQueueReset(_setpointMailbox)` vide la mailbox OdoControl
4. Au prochain tick OdoControl (≤5ms), `xQueuePeek` retourne `pdFALSE` → `_hasSetpoint = false` → `setMotors(0, 0)` — moteurs stoppés

---

### BusFormat::altAlarm() — Implémentation

```cpp
// BusFormat.h
static const char* altAlarm(uint32_t bitmask);

// BusFormat.cpp
const char* BusFormat::altAlarm(uint32_t bitmask) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "ALT ALARM 0x%08lX\n",
             static_cast<unsigned long>(bitmask));
    return buf;
}
```

**Note :** `%08lX` pour compatibilité `uint32_t` sur ARM (32-bit) et x86-64 (test host). `static` buffer — ne jamais retourner un pointeur vers une variable locale.

---

### xTaskNotifyWait — Stub de test à ajouter

**Tests/Stubs/task.h — ajouter la déclaration :**
```cpp
BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry,
                           uint32_t ulBitsToClearOnExit,
                           uint32_t* pulNotificationValue,
                           TickType_t xTicksToWait);
```

**Tests/Stubs/FreeRTOSStub.cpp — ajouter le stub :**
```cpp
BaseType_t xTaskNotifyWait(uint32_t, uint32_t, uint32_t* pulNotificationValue,
                           TickType_t xTicksToWait) {
    if (pulNotificationValue) *pulNotificationValue = 0;
    if (xTicksToWait == portMAX_DELAY) throw TaskDelayEscape{};
    return pdFALSE; // No notification pending
}
```

**Comportement du stub :** retourne `pdFALSE` (aucune notification) sauf si `portMAX_DELAY` → throw. Le test appelle `processCmd()` et `handleAlarm()` directement — le stub n'interfère pas.

---

### Tests/Unit/MotionPlannerTest.cpp — 5 cas de test

```cpp
#include "Stubs/MockHAL.h"
#include <gtest/gtest.h>
#include "Mocks/MockBus.h"
#include "Tasks/MotionPlanner.h"

class MotionPlannerTest : public ::testing::Test {
protected:
    MockBus       bus;
    QueueHandle_t cmdMailbox;
    QueueHandle_t setpointMailbox;
    MotionPlanner* mp;

    void SetUp() override {
        cmdMailbox       = xQueueCreate(1, sizeof(MoveCmd));
        setpointMailbox  = xQueueCreate(1, sizeof(Setpoint));
        mp = new MotionPlanner{ &bus, cmdMailbox, setpointMailbox };
    }
    void TearDown() override { delete mp; }
};

// CMD MOVE → Setpoint POSE écrit dans setpointMailbox
TEST_F(MotionPlannerTest, ProcessCmdWritesPoseSetpoint) {
    MoveCmd cmd{ 1.0f, 0.5f, 0.0f };
    mp->processCmd(cmd);

    Setpoint sp{};
    ASSERT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdTRUE);
    EXPECT_EQ(sp.mode, SetpointMode::POSE);
    EXPECT_FLOAT_EQ(sp.pose.x,     1.0f);
    EXPECT_FLOAT_EQ(sp.pose.y,     0.5f);
    EXPECT_FLOAT_EQ(sp.pose.angle, 0.0f);
}

// Setpoint précédent écrasé par nouveau CMD MOVE (xQueueOverwrite)
TEST_F(MotionPlannerTest, ProcessCmdOverwritesPreviousSetpoint) {
    mp->processCmd({ 1.0f, 0.0f, 0.0f });
    mp->processCmd({ 2.0f, 3.0f, 1.57f });

    Setpoint sp{};
    ASSERT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdTRUE);
    EXPECT_FLOAT_EQ(sp.pose.x, 2.0f);
}

// Alarme → xQueueReset sur setpointMailbox + ALT ALARM publié
TEST_F(MotionPlannerTest, HandleAlarmResetsMailboxAndPublishesAlert) {
    // Pre-fill setpointMailbox to verify reset
    mp->processCmd({ 1.0f, 0.0f, 0.0f });

    mp->handleAlarm(0x01);

    // setpointMailbox must be empty after reset
    Setpoint sp{};
    EXPECT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdFALSE);

    // ALT ALARM published
    ASSERT_TRUE(bus.hasPublished(Topic::ALERT));
    EXPECT_NE(bus.last(Topic::ALERT)->payload.find("ALARM"), std::string::npos);
}

// Alarme sans consigne préalable — pas de crash
TEST_F(MotionPlannerTest, HandleAlarmWithEmptyMailboxDoesNotCrash) {
    EXPECT_NO_THROW(mp->handleAlarm(0xFF));
    EXPECT_TRUE(bus.hasPublished(Topic::ALERT));
}

// ExternalComm → cmdMailbox → processCmd via queue (intégration queue)
TEST_F(MotionPlannerTest, QueueIntegration_CmdMailboxDeliveredToSetpoint) {
    MoveCmd cmd{ 5.0f, 2.5f, 0.785f };
    xQueueOverwrite(cmdMailbox, &cmd);

    MoveCmd received{};
    ASSERT_EQ(xQueueReceive(cmdMailbox, &received, 0), pdTRUE);
    mp->processCmd(received);

    Setpoint sp{};
    ASSERT_EQ(xQueuePeek(setpointMailbox, &sp, 0), pdTRUE);
    EXPECT_FLOAT_EQ(sp.pose.x, 5.0f);
    EXPECT_FLOAT_EQ(sp.pose.y, 2.5f);
}
```

---

### SystemInit.cpp — Câblage complet après story 2.3

```cpp
#include "Tasks/MotionPlanner.h"   // ← ADD (provides MoveCmd + MotionPlanner)
// ... autres includes existants

// Supprimer: struct RawMoveCmd { float left; float right; };
// cmdMailbox passe de 8 à 12 bytes item size
static QueueHandle_t cmdMailbox      = xQueueCreate(1, sizeof(MoveCmd));      // ← CHANGED
static QueueHandle_t setpointMailbox = xQueueCreate(1, sizeof(Setpoint));

static MotionPlanner motionPlanner { &extComm, cmdMailbox, setpointMailbox };  // ← ADD

// Capturer le handle pour SensorManager (Story 3.1)
static TaskHandle_t motionPlannerHandle = nullptr;  // ← ADD

void SystemInit::boot() {
    // ... initialisations existantes ...
    xTaskCreate(MotionPlanner::task, "MoPlan",
                Config::STACK_MOTION_PLANNER, &motionPlanner,
                Config::PRIO_MOTION_PLANNER, &motionPlannerHandle);  // ← ADD
    // ... reste des xTaskCreate existants ...
}
```

**Note :** `motionPlannerHandle` est utilisé par `SensorManager` (Story 3.1) pour `xTaskNotify`. Le rendre accessible : déclarer `extern TaskHandle_t motionPlannerHandle;` dans `SystemInit.h`, ou passer le handle au constructeur de `SensorManager` depuis `boot()`.

---

### ExternalComm.cpp — Diff minimal du bloc CMD MOVE

```cpp
// AVANT (supprimer ces lignes)
float left = 0.0f, right = 0.0f;
sscanf(line, "%*s %*s %f %f", &left, &right);
struct {
    float left;
    float right;
} sp = { left, right };
xQueueOverwrite(_motionMailbox, &sp);

// APRÈS (remplacer par)
MoveCmd cmd{};
sscanf(line, "%*s %*s %f %f %f", &cmd.x, &cmd.y, &cmd.angle);
xQueueOverwrite(_motionMailbox, &cmd);
```

**Ajouter en tête de ExternalComm.cpp :**
```cpp
#include "Tasks/MotionPlanner.h"   // for MoveCmd
```

---

### Architecture Constraints

| Règle | Source | Application |
|-------|--------|-------------|
| `MotionPlanner` ne connaît pas `ExternalComm` | Architecture boundary | Toute sortie via `IBus::publish()` |
| `xQueueOverwrite` pour écriture consigne | FR-02 | Non-bloquant, depth-1 mailbox |
| `xQueueReset()` pour arrêt d'urgence | FR-02 | O(1), primitif atomique FreeRTOS |
| Zéro `snprintf` dans `MotionPlanner.cpp` | FR-09 | Formatage via `BusFormat::altAlarm()` |
| Zéro `new/delete` | NFR-02 | Instances statiques dans SystemInit |
| Identifiants en anglais | NFR-08 | `MoveCmd`, `processCmd`, `handleAlarm` |
| Guard convention | NFR-06 | `#ifndef APP_TASKS_MOTIONPLANNER_H` |
| `xTaskCreate` uniquement dans SystemInit | FR-11 | `MotionPlanner::task` créé dans `boot()` |
| Développer uniquement dans `App/` et `Tests/` | NFR-07 | Aucune modification dans `Core/`, `Middlewares/` |

---

### Dépendances — Ce qui doit exister avant l'implémentation

**Depuis Story 2.2 (doit être complet) :**
- `App/Tasks/OdoControl.h` — `Setpoint`, `SetpointMode`, `PoseTarget` définis à la portée de fichier
- `App/SystemInit/SystemInit.cpp` — `setpointMailbox` (QueueHandle_t, depth-1, sizeof(Setpoint)) existe

**Depuis Stories 1.x :**
- `App/Interfaces/IBus.h`, `App/BusFormat.h/.cpp` (ajouter `altAlarm` dans cette story)
- `Tests/Mocks/MockBus.h`, stubs FreeRTOS, `Tests/Stubs/MockHAL.h`

---

### Travaux différés

- **Velocity mode :** `SetpointMode::VELOCITY` déclaré dans `OdoControl.h` (Story 2.2) mais non implémenté. `processCmd` construit uniquement `Mode::POSE` — prévu conforme à l'epic.
- **Robustesse OdoControl :** détection blocage moteur et encodeur silencieux → Story 2.4
- **SensorManager handle :** `motionPlannerHandle` capturé mais non consommé avant Story 3.1
- **Séquençage waypoints :** AC #2 dit "MotionPlanner peut envoyer le prochain waypoint ou rester idle" — logique de séquence multi-waypoints = hors scope de cette story

---

### Références

- Exigences : [epics.md — Story 2.3](_bmad-output/planning-artifacts/epics.md)
- Story précédente (OdoControl, Setpoint, mailbox) : [2-2-odocontrol-tache-200hz-double-pid-orthogonal-setpoint-union-condition-arrivee.md](_bmad-output/implementation-artifacts/2-2-odocontrol-tache-200hz-double-pid-orthogonal-setpoint-union-condition-arrivee.md)
- Architecture : [architecture.md](_bmad-output/planning-artifacts/architecture.md)
- ExternalComm (parseur CMD, cmdMailbox) : [1-4-externalcomm-implementation-ibus-et-communication-tri-canal-ascii.md](_bmad-output/implementation-artifacts/1-4-externalcomm-implementation-ibus-et-communication-tri-canal-ascii.md)
- SystemInit câblage : [1-5-systeminit-boot-cablage-statique-complet.md](_bmad-output/implementation-artifacts/1-5-systeminit-boot-cablage-statique-complet.md)
- Stubs FreeRTOS (pattern xTaskNotifyWait à ajouter) : [1-6-infrastructure-google-test-mocks-hal-tests-busformat-externalcomm.md](_bmad-output/implementation-artifacts/1-6-infrastructure-google-test-mocks-hal-tests-busformat-externalcomm.md)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

(aucun blocage)

### Completion Notes List

- Implémenté `BusFormat::altAlarm(uint32_t)` avec buffer statique 32 octets, format `ALT ALARM 0x%08lX\n`
- Créé `MotionPlanner` avec boucle event-driven : `xTaskNotifyWait(50ms timeout)` + poll `cmdMailbox` non-bloquant
- `processCmd()` construit `Setpoint{POSE, {x,y,angle}}` et écrit via `xQueueOverwrite` dans la mailbox OdoControl
- `handleAlarm()` appelle `xQueueReset(_setpointMailbox)` (arrêt O(1)) puis publie `ALT ALARM` sur `IBus`
- Corrigé `ExternalComm.cpp` : CMD MOVE passe de 2 floats (duty) à 3 floats (pose x,y,angle)
- `SystemInit.cpp` : supprimé `RawMoveCmd`, `cmdMailbox` corrigé à `sizeof(MoveCmd)` (12 bytes au lieu de 8)
- `motionPlannerHandle` exposé via `extern` dans `SystemInit.h` pour Story 3.1 (SensorManager)
- Stub `xTaskNotifyWait` ajouté : retourne `pdFALSE` sauf `portMAX_DELAY` → throw `TaskDelayEscape`
- 5 tests unitaires : routage Setpoint, écrasement, alarme+reset, alarme sans consigne, intégration queue
- **34/34 tests passent** (BusFormat:9, ExternalComm:6, ConcreteOdomHAL:5, OdoControl:9, MotionPlanner:5)
- Build ARM STM32 corrigé : `#include <cstdint>` dans BusFormat.h, `extern`/`static` résolu sur `motionPlannerHandle`

### File List

- `App/BusFormat.h` — ajout `#include <cstdint>` + déclaration `altAlarm`
- `App/BusFormat.cpp` — implémentation `altAlarm`
- `App/Tasks/MotionPlanner.h` — nouveau fichier (MoveCmd + classe MotionPlanner)
- `App/Tasks/MotionPlanner.cpp` — nouveau fichier (task loop, processCmd, handleAlarm)
- `App/Tasks/ExternalComm.cpp` — CMD MOVE corrigé : 3 floats, MoveCmd, include MotionPlanner.h
- `App/SystemInit/SystemInit.h` — ajout `extern TaskHandle_t motionPlannerHandle`
- `App/SystemInit/SystemInit.cpp` — câblage MotionPlanner, fix cmdMailbox size, motionPlannerHandle
- `Tests/Stubs/task.h` — déclaration `xTaskNotifyWait`
- `Tests/Stubs/FreeRTOSStub.cpp` — stub `xTaskNotifyWait`
- `Tests/Unit/MotionPlannerTest.cpp` — nouveau fichier (5 cas de test)
- `Tests/CMakeLists.txt` — target `MotionPlannerTest` ajouté

### Change Log

- 2026-05-14 : Story 2.3 implémentée — MotionPlanner event-driven, mailbox consigne, arrêt d'urgence. Breaking change cmdMailbox 8→12 bytes. 34/34 tests passent.
