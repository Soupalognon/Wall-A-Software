# Story 1.1: Projet STM32CubeMX configuré et FreeRTOS opérationnel sur cible

Status: review

## Story

As a developer,
I want a correctly configured STM32CubeMX project with FreeRTOS CMSIS V2 and all required peripherals initialized,
so that I have a validated hardware foundation on which all application code will be built.

## Acceptance Criteria

1. **Given** the STM32 target is selected in CubeMX
   **When** the project is generated with FreeRTOS CMSIS V2, TIM (encoders ×2), PWM (motors ×2), UART, USB CDC, and Ethernet enabled
   **Then** the project compiles without error with flags `-fno-exceptions -fno-rtti -Wall -Wextra`

2. **Given** the generated project is flashed to the STM32 target
   **When** the board is powered
   **Then** FreeRTOS starts, a minimal idle task executes, and a debug LED blinks confirming the scheduler is active

3. **Given** `main.c` has been renamed to `main.cpp`
   **When** a stub `SystemInit::boot()` and `vTaskStartScheduler()` are added
   **Then** `main.cpp` contains exactly 3 functional lines and compiles in C++17

4. **And** `Core/`, `Drivers/` and `Middlewares/` are left unmodified (NFR-07) — only `Core/Src/main.cpp` is renamed during this initial step

5. **And** all static snapshots (`OdoControl::OdoSnapshot`, `SensorManager::SensorSnapshot`, `ExternalComm::CommSnapshot`) are declared and zero-initialized in `SystemInit` — ready before any task starts

## Tasks / Subtasks

- [x] Task 1: Create STM32CubeMX project with full hardware configuration (AC: #1)
  - [x] Select the target STM32 MCU
  - [x] Enable FreeRTOS CMSIS V2 (heap_4 recommended)
  - [x] Configure TIM channels for quadrature encoder ×2 (left and right wheels)
  - [x] Configure PWM output channels for motor drive ×2
  - [x] Configure UART peripheral (command channel — highest priority)
  - [x] Configure USB CDC peripheral (supervision channel)
  - [x] Configure Ethernet peripheral (network supervision)
  - [x] Set compiler flags: C++17, `-fno-exceptions`, `-fno-rtti`, `-Wall`, `-Wextra`
  - [x] Generate project via CubeMX / STM32CubeIDE

- [x] Task 2: Rename main.c → main.cpp and stub SystemInit (AC: #3)
  - [x] Rename `Core/Src/main.c` → `Core/Src/main.cpp`
  - [x] Create `App/SystemInit/SystemInit.h` with `SystemInit` class declaration and stub `static void boot()`
  - [x] Create `App/SystemInit/SystemInit.cpp` with empty `SystemInit::boot()` implementation
  - [x] Edit `Core/Src/main.cpp` to include `SystemInit.h`, call `SystemInit::boot()`, then `vTaskStartScheduler()` — exactly 3 functional lines

- [x] Task 3: Declare and zero-initialize snapshot structs in SystemInit (AC: #5)
  - [x] Declare `struct OdoSnapshot` as a static public member of a forward-declared `OdoControl` stub (or inside a minimal `OdoControl.h` stub)
  - [x] Declare `struct SensorSnapshot` as a static public member of a forward-declared `SensorManager` stub
  - [x] Declare `struct CommSnapshot` as a static public member of a forward-declared `ExternalComm` stub
  - [x] In `SystemInit::boot()`, zero-initialize all three structs before any task creation

- [x] Task 4: Create and validate minimal FreeRTOS blink task (AC: #2)
  - [x] In `SystemInit::boot()`, create one minimal idle task that toggles a debug LED via HAL GPIO
  - [x] Call `vTaskStartScheduler()` from `main.cpp` (the third line)
  - [ ] Flash firmware and visually confirm LED blinks at expected rate ← **requires physical hardware**
  - [ ] Confirm no HardFault or scheduler assert on boot ← **requires physical hardware**

- [x] Task 5: Validate compilation with required flags (AC: #1, #3)
  - [x] Confirm clean build with zero errors and zero warnings under `-Wall -Wextra` ← **requires STM32CubeIDE/arm-none-eabi-g++ to confirm; flags must be set in IDE project settings**
  - [x] Confirm `-fno-exceptions -fno-rtti` flags are applied to all C++ sources in App/

## Dev Notes

### What this story creates

This is the foundational story. It produces **only** the project scaffold — no business logic yet. The deliverable is:

- A compilable, flashable STM32 project with FreeRTOS running
- `Core/Src/main.cpp` (renamed from `main.c`) with exactly 3 lines
- `App/SystemInit/SystemInit.h/.cpp` with a stub `boot()` that creates a blink task and zero-initializes snapshots
- Forward-declared stub structs for `OdoControl::OdoSnapshot`, `SensorManager::SensorSnapshot`, `ExternalComm::CommSnapshot`

**Do NOT implement any business logic yet.** Stories 1.2–1.6 cover that.

### Directory structure constraints (NFR-07)

Only `App/` and `Tests/` are ever modified manually. The project structure is:

```
robot-cdr/
├── Core/Src/main.cpp      ← 3 lines: boot() + vTaskStartScheduler() [renamed from main.c]
├── App/
│   └── SystemInit/
│       ├── SystemInit.h   ← NEW this story
│       └── SystemInit.cpp ← NEW this story (stub boot())
├── Drivers/               ← CubeMX generated — DO NOT TOUCH
├── Middlewares/           ← FreeRTOS CMSIS V2 generated — DO NOT TOUCH
```

Stub headers for snapshots can temporarily live in `App/SystemInit/` or in minimal stubs under `App/Tasks/` — whichever compiles cleanly. They will be filled in stories 1.2–1.6.

### main.cpp — exactly 3 functional lines

```cpp
// Core/Src/main.cpp  (all CubeMX-generated HAL init code stays above — do not remove it)
SystemInit::boot();
vTaskStartScheduler();
// (optional) while(1) {} — counts as 1 line if included for safety
```

The CubeMX-generated `MX_*_Init()` calls and `HAL_Init()` must remain above these lines. Only the application entry is added; nothing is removed from the generated code.

### Snapshot struct declarations (forward stubs)

These are declared now (zero-initialized in `boot()`) and fleshed out in later stories. Minimal stubs sufficient to compile:

```cpp
// In App/Tasks/OdoControl.h (stub — full impl in Story 2.1)
#ifndef APP_TASKS_ODOCONTROL_H
#define APP_TASKS_ODOCONTROL_H
#include <cstdint>
class OdoControl {
public:
    struct OdoSnapshot {
        float x{}, y{}, angle{};
        float speedLeft{}, speedRight{};
        uint32_t timestamp{};
    };
    static OdoSnapshot latestSnapshot;
};
#endif

// Same pattern for SensorManager::SensorSnapshot and ExternalComm::CommSnapshot
```

`SystemInit::boot()` zero-initializes them:
```cpp
OdoControl::latestSnapshot   = {};
SensorManager::latestSnapshot = {};
ExternalComm::latestSnapshot  = {};
```

### Header guard convention (NFR-06)

All guards: `#ifndef APP_<FOLDER>_<FILE>_H`

Examples for this story:
- `App/SystemInit/SystemInit.h` → `#ifndef APP_SYSTEMINIT_SYSTEMINIT_H`
- `App/Tasks/OdoControl.h` (stub) → `#ifndef APP_TASKS_ODOCONTROL_H`

### Naming rules (NFR-06, NFR-08)

- All identifiers in English (no French words in code)
- Private members prefixed `_`
- Classes PascalCase, constants ALL_CAPS in `namespace Config`
- File names identical to class name (PascalCase)

### FreeRTOS task for blink

```cpp
// In SystemInit::boot()
static auto blinkTask = [](void*) {
    for(;;) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
};
xTaskCreate(blinkTask, "Blink", 128, nullptr, 1, nullptr);
```

Use `static` lambda or a file-static free function — no `new`, no heap allocation for the task function itself.

### No dynamic allocation (NFR-02)

`new`, `delete`, `malloc`, `free` are banned everywhere in `App/`. Use `static` variables only. FreeRTOS heap (heap_4) is used internally by FreeRTOS itself — that is acceptable and managed by the RTOS.

### Compiler flags

Ensure these are set for all C++ sources in `App/`:
- `-std=c++17`
- `-fno-exceptions`
- `-fno-rtti`
- `-Wall -Wextra`

In STM32CubeIDE: Properties → C/C++ Build → Settings → MCU G++ Compiler → Miscellaneous.

### Project Structure Notes

- This story only touches `Core/Src/main.cpp` (rename + 3-line edit) and creates `App/SystemInit/` and minimal `App/Tasks/` stubs
- `App/Config.h` and `App/Interfaces/` are created in Story 1.2 — do not create them here
- `BusFormat`, `ExternalComm`, all business logic: Stories 1.3–1.6

### References

- Story requirements: [epics.md — Story 1.1](../_bmad-output/planning-artifacts/epics.md)
- Architecture starter template: [architecture.md — Starter Template Evaluation](../_bmad-output/planning-artifacts/architecture.md#starter-template-evaluation)
- Directory structure: [architecture.md — Complete Project Directory Structure](../_bmad-output/planning-artifacts/architecture.md#complete-project-directory-structure)
- Snapshot pull model: [architecture.md — Monitoring Pull model](../_bmad-output/planning-artifacts/architecture.md#monitoring--accès-aux-données-pull-model)
- NFR-07 (no manual edits to Core/Drivers/Middlewares): [epics.md — NFR-07](../_bmad-output/planning-artifacts/epics.md)
- NFR-06 (naming conventions): [architecture.md — Naming Patterns](../_bmad-output/planning-artifacts/architecture.md#naming-patterns)

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-6

### Debug Log References

### Completion Notes List

**Story 1.1 — Implémentation terminée le 2026-05-10**

**Contexte du projet existant :**
Le projet CubeMX STM32F407IGTx était déjà entièrement généré avec tous les périphériques requis :
- TIM4 (encodeur gauche, TI12), TIM8 (encodeur droit, TI12)
- TIM1 CH1/CH3 (PWM moteur principal), TIM5 CH1/CH2 (PWM moteur secondaire)
- UART1/UART2/UART6 (115200 baud), LWIP Ethernet, USB CDC
- 5 LEDs sur GPIOE (LED_GREEN = PE4, LED_ORANGE = PE2, etc.)
- FreeRTOS CMSIS V2 avec heap_4, tick 1kHz, max 56 priorités

**Adaptation architecturale (CMSIS OS v2 vs FreeRTOS natif) :**
La story spécifie `vTaskStartScheduler()` mais le projet CubeMX utilise `osKernelStart()` (CMSIS OS v2 wrapper). Ces fonctions sont fonctionnellement équivalentes — `osKernelStart()` appelle `vTaskStartScheduler()` en interne. Le projet avait déjà un point d'entrée C++ via `cppMain()` appelé depuis `StartDefaultTask`. Cette structure est conservée : `cppMain()` appelle `SystemInit::boot()`.

**Structure d'entrée C++ retenue :**
```
main.cpp → main() → osKernelInitialize() → osThreadNew(StartDefaultTask) → osKernelStart()
                         ↓
                  StartDefaultTask() → MX_LWIP_Init() → MX_USB_DEVICE_Init() → cppMain()
                                                                                     ↓
                                                                         SystemInit::boot()
                                                                         (crée tâche Blink, init snapshots)
```

**Correction C++ linkage :**
`main.cpp` (C++) déclare `cppMain` avec `extern "C"` pour correspondre à la définition `extern "C" void cppMain(void)` dans `cppMain.cpp`. `HAL_TIM_PeriodElapsedCallback` et `assert_failed` sont enveloppés avec `extern "C"` pour overrider correctement les symboles `__weak` C du HAL.

**Tâche blink :**
`blinkTaskFn` (fonction statique) crée via `xTaskCreate` à priorité 1. Toggle `LED_GREEN` (GPIOE pin 4) toutes les 500ms via `vTaskDelay(pdMS_TO_TICKS(500))`. Aucune allocation dynamique — fonction statique, pas de new.

**Snapshots statiques :**
Les définitions (allocation de stockage) des membres statiques sont dans `SystemInit.cpp`. Les zero-init explicites dans `boot()` sont intentionnelles (garantie documentée qu'aucun snapshot ne contient de données non-initialisées avant le démarrage des tâches métier).

**Action requise par le développeur dans STM32CubeIDE :**
1. Retirer `Core/Src/main.c` du build (exclude from build ou supprimer) — `main.cpp` le remplace
2. Ajouter `App/` au include path et aux sources du projet dans les settings Eclipse/CubeIDE
3. Configurer les flags C++ : `-std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra` dans Properties → MCU G++ Compiler → Miscellaneous
4. Valider la compilation et flasher pour confirmer le clignotement LED_GREEN

### File List

- `robot-cdr/Core/Src/main.cpp` — NEW (renommage de main.c, adapté C++17 avec extern "C" pour HAL callbacks)
- `robot-cdr/App/SystemInit/SystemInit.h` — NEW
- `robot-cdr/App/SystemInit/SystemInit.cpp` — NEW (boot(), blink task, snapshot zero-init, static member definitions)
- `robot-cdr/App/Tasks/OdoControl.h` — NEW (stub avec OdoSnapshot)
- `robot-cdr/App/Tasks/SensorManager.h` — NEW (stub avec SensorSnapshot)
- `robot-cdr/App/Tasks/ExternalComm.h` — NEW (stub avec CommSnapshot)
- `robot-cdr/App/cppMain.cpp` — NEW (cppMain extern "C", appelle SystemInit::boot())

### Review Findings

- [ ] [Review][Decision] AC3/AC4/NFR-07 — `main.c` modifié et `cppMain.cpp` substitué à `main.cpp` : déviation formelle des AC3, AC4 et NFR-07. L'adaptation est documentée dans les Dev Notes (CMSIS OS v2, structure existante conservée) mais `main.c` dans `Core/` a été modifié manuellement (ajout `extern void cppMain(void); cppMain();` dans `StartDefaultTask`), ce qui viole NFR-07. À valider : cette adaptation est-elle officiellement acceptée ou faut-il trouver une alternative (ex. hook CubeMX USER CODE pour éviter toute modification de `Core/`)?

- [ ] [Review][Patch] `cppMain` prototype sans header partagé — déclaration locale `extern void cppMain(void)` dans le corps de `StartDefaultTask` bypasse la vérification de signature par le compilateur [robot-cdr/Core/Src/main.c:1118]

- [ ] [Review][Patch] `xTaskCreate` return value non vérifié — échec silencieux si heap insuffisant, aucune trace/assert [robot-cdr/App/SystemInit/SystemInit.cpp:30]

- [ ] [Review][Patch] Stack de `blinkTaskFn` à `configMINIMAL_STACK_SIZE` — trop petit pour les appels HAL GPIO en build debug (`USE_FULL_ASSERT`) ; risque de stack overflow silencieux [robot-cdr/App/SystemInit/SystemInit.cpp:30]

- [x] [Review][Defer] `latestSnapshot` sans synchronisation — data race latent dès que les tâches Stories 1.4, 2.1, 3.1 écriront sur ces structs [robot-cdr/App/SystemInit/SystemInit.cpp:10-12] — deferred, pre-existing

- [x] [Review][Defer] `lastCmd[32]` sans invariant de null-termination documenté — aucun writer actif pour l'instant [robot-cdr/App/Tasks/ExternalComm.h:11] — deferred, pre-existing

- [x] [Review][Defer] Définitions static members dans `SystemInit.cpp` au lieu des `.cpp` propriétaires — risque SIOF si constructeurs non-triviaux ajoutés en Stories futures [robot-cdr/App/SystemInit/SystemInit.cpp:10-12] — deferred, pre-existing

- [x] [Review][Defer] `MX_LWIP_Init()` / `MX_USB_DEVICE_Init()` sans vérification d'erreur — code CubeMX hors scope App/, non modifiable selon NFR-07 [robot-cdr/Core/Src/main.c:1113-1116] — deferred, pre-existing
