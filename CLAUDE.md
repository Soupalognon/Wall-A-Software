# Instructions projet — Wall-A-Software

## Règles absolues — NE JAMAIS enfreindre
1. **Coder uniquement dans `App/` et `Tests/`.** `Core/`, `Drivers/` (HAL CubeMX), `Middlewares/`, `LWIP/`, `USB_DEVICE/` = générés CubeMX → jamais édités à la main.
2. **Aucun `new`/`delete`/`malloc`/`free`.** Allocation statique uniquement, tableaux fixes.
3. **`App/` n'appelle jamais une fonction HAL CubeMX directement** → toujours via une interface injectée.
4. **Tout est instancié et câblé dans `cppMain.cpp` SEULEMENT**, par injection de dépendance via constructeur.
5. **Une tâche ne parle jamais directement à une autre tâche** → `IBus::publish` / `xTaskNotify` / `xQueueOverwrite`+`xQueuePeek`.
6. **Tous les identifiants ET commentaires en anglais.** Aucune exception (la doc/PRD peuvent rester en français).
7. **Compilation `-Wall -Wextra -fno-exceptions -fno-rtti` → 0 warning.**
8. **Ne jamais lancer les tests toi-même** → demander à l'utilisateur (cf. § Tests).

Firmware STM32F407 (CubeMX), C++17, FreeRTOS CMSIS V2, HAL Cube. Source de vérité :
`_bmad-output/planning-artifacts/architecture.md` + `prd.md` ; en cas de contradiction,
`architecture.md` fait foi. Point d'entrée : `main.c` (CubeMX) appelle `cppMain()`.

## Architecture en couches (`App/`) — critère d'appartenance strict
- `Interfaces/` : contrats purs `I*.h`, zéro impl, zéro dépendance HW. Domaine (`IBus`, `ISensor`, `IActuator`, `IActuatorManager`, `ICom`/`ICommChannel`) + HAL (`IMotorHAL`, `IEncoderHAL`, `IOdomHAL`, `IAdcHAL`, `IInputCaptureHAL`) au même niveau.
- `Drivers/` : parlent au HW directement (registres STM32 / HAL CubeMX). Implémentent une `I***HAL` ou `ICommChannel`. → `Adc`, `Drv8262`, `Encoder`, `InputCapture`, `UartChannel`, `UsbCdcChannel`.
- `Services/` : orchestrent des Drivers **via leurs interfaces**, aucun appel HAL direct, **pas** une tâche FreeRTOS. → `Odometry`, `ActuatorManager`, `BusFormat`.
- `Services/Sensors/` : capteur concret `ISensor` convertissant une source brute injectée (`IAdcHAL`/`IInputCaptureHAL`) en grandeur physique + alarme. → `B5WLB2101`, `InternalTemperature`, `MotorCurrentSense`, `Pololu5472`.
- `Controllers/` : algorithme pur, **zéro FreeRTOS, zéro HW**, testable sans matériel. → `Pid`.
- `Tasks/` : tâche FreeRTOS uniquement (boucle infinie ou `osThreadNew`). → `OdoControl`, `MotionPlanner`, `SensorManager`, `Monitoring`, `ExternalComm` (2 tâches rx+tx).
- `Config.h` : toutes les `constexpr`. `cppMain.cpp` : unique point de câblage.

## Règles dures — précisions
- Tableaux fixes : `MAX_SENSORS=15`, `MAX_ACTUATORS=10`.
- `cppMain.cpp` (objets `static` au scope fichier + `cppMain()`) résout les deps circulaires (ex. `actuatorMgr.setBus(&extComm)`), appelle `InputCapture::initAll()`, puis crée les tâches.
- Exception à la règle #5 : appel direct vers un **Service** (ex. `ActuatorManager`) autorisé — ce n'est pas une tâche.
- Une tâche n'instancie jamais un driver → reçoit des `ISensor*`/interfaces déjà câblés, injectés.

## Nommage / style
- Membres privés préfixés `_` : `float _x; IBus* _bus;`.
- Constantes `ALL_CAPS` dans `namespace Config`.
- Interfaces préfixées `I`. Classes métier PascalCase. Nom de fichier == nom de classe (PascalCase).
- Header guards : `APP_<DOSSIER>_<FICHIER>_H`.
- Commentaires **parcimonieux** : uniquement un *pourquoi* non-évident (contrainte HW, invariant subtil, workaround). Noms auto-explicites.
- Publication IBus **toujours** via `BusFormat::` — **jamais** de `snprintf` inline.
- Stacks & priorités FreeRTOS déclarées dans `Config.h` **uniquement**, jamais en local.

## Communication / protocole
- ASCII : `TOPIC PAYLOAD\n`. `enum class Topic` = sorties seulement : `TELEMETRY`(TEL), `ALERT`(ALT), `LOG`(LOG), `HEALTH`(HLT). Les `CMD` entrantes ne passent **pas** par IBus → parsées par `ExternalComm::rxTask` (`sscanf`, dispatch sur 1er token).
- Routage par canal via `Config::ComQueuePolicy {log,tel,alt,hlt}` : UART=DEBUG (log+alt), USB=PC (tel+alt+hlt), ETH réservé (`nullptr`). Flag `ENABLE_HIGH_SPEED_TUNING` → profil logs-only.
- Politique par topic dans `ExternalComm::BUS_CONFIG` : `OVERWRITE` pour TEL/ALT, `DROP_SILENT` pour HLT/LOG.
- `ExternalComm` = seul producteur d'octets série ; toutes les sorties passent par lui. Helpers statiques `log_info/log_warn/log_error`.

## Patterns temps-réel — invariants (pipeline détaillé : `architecture.md`)
- **`Monitoring` = modèle pull** sur snapshots statiques horodatés. Writer haute prio : pas de section critique. **Reader basse prio : la copie DOIT être entourée de `taskENTER_CRITICAL()`/`taskEXIT_CRITICAL()`** (anti torn-read). Stale si `HAL_GetTick()-timestamp > MONITORING_STALE_MS` → ALERT.
- **Acquisition non-bloquante** : `ISensor::trigger()` lance ADC/InputCapture → ISR `xTaskNotifyFromISR(doneFlag)` → `SensorManager`. Drivers auto-enregistrés (liste chaînée statique intrusive `s_head`/`_next`), dispatch ISR résolu par `(périphérique, isActive)`.
- **`OdoControl`** : seule tâche à 200 Hz (`vTaskDelayUntil`), seul consommateur de `IEncoderHAL`+`IMotorHAL`. Watchdog : consigne→0 si pas de commande sous `CMD_WATCHDOG_TIMEOUT_MS`.
- **Alarmes capteurs** : `SensorManager` → `xTaskNotify(motionPlanner, bitmask)`, latence ≤1 tick. Groupes multi-cadence (`SensorGroup`), chacun à sa fréquence.

## Table tâches FreeRTOS (prio / stack mots-32b)
OdoControl 6/512 · MotionPlanner 5/256 · ExtComm RX 5/512 · ExtComm TX 4/256 · SensorManager 3/512 · Monitoring 2/1024.

## Règle d'extensibilité (NFR-05)
Ajouter capteur/actionneur/canal = **nouvelle classe concrète** implémentant l'interface + câblage dans `cppMain.cpp`. **Zéro** modification d'une classe existante. (Si on touche `MAX_SENSORS` : aussi mettre à jour l'id `SensorType` + la table `AlarmBits` SENSOR de `MotionPlanner.h` — cf. warning dans `Config.h`.)

## Tests
- Host PC x86 via Google Test + GMock — pas de toolchain ARM, pas de STM32. 100% des classes `App/` testables avec mocks injectés.
- `Tests/Mocks` (`MockBus`, `Mock*HAL`, `Fake*HAL`…), `Tests/Stubs` (stubs FreeRTOS/HAL pour compiler host), `Tests/Unit` (`*Test.cpp`, un par classe).
- **Ne jamais lancer les tests via Bash/PowerShell** (MSYS2/MinGW64, sortie mal capturée). Demander à l'utilisateur d'exécuter, puis coller le résultat :

```powershell
& "D:\msys64\usr\bin\bash.exe" -l "D:\_Programs\STM32\Wall-A-Software\Wall-A-STM\Tests\run_tests.sh"
```
