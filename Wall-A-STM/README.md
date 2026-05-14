# Robot CDR — STM32F407

Firmware embarqué STM32F407IGT6 avec FreeRTOS, généré par STM32CubeMX.

## Périphériques initialisés (Core/Src/main.c)

### ADC
| Périphérique | Canal | Résolution | Déclenchement |
|---|---|---|---|
| ADC1 | CH3 | 12-bit | Software |
| ADC2 | CH9 | 12-bit | Software |
| ADC3 | CH4 | 12-bit | Software |

### CAN
| Périphérique | Débit | Mode |
|---|---|---|
| CAN1 | 875 kbps | Normal |

### I2C
| Périphérique | Fréquence |
|---|---|
| I2C1 | 100 kHz |

### SPI
| Périphérique | Mode | Taille donnée | Polarité |
|---|---|---|---|
| SPI2 | Maître | 8-bit | Mode 0 (CPOL=0, CPHA=0) |

### Timers
| Timer | Mode | Canaux |
|---|---|---|
| TIM1 | PWM | CH1, CH3 |
| TIM2 | PWM 32-bit | CH1, CH2, CH3, CH4 |
| TIM3 | Input Capture | CH1, CH2, CH3, CH4 |
| TIM4 | Encodeur quadrature (TI12) | CH1, CH2 |
| TIM5 | PWM 32-bit | CH1, CH2 |
| TIM8 | Encodeur quadrature (TI12) | CH1, CH2 |

### UART
| Périphérique | Baudrate | Mode |
|---|---|---|
| USART1 | 115200 | TX/RX |
| USART2 | 115200 | TX/RX |
| USART6 | 115200 | TX/RX |

### GPIO
| Signal | Direction | Rôle |
|---|---|---|
| LED_ORANGE/YELLOW/GREEN/BLUE/RED | Sortie | LEDs de statut ×5 |
| WHEEL_DIR1/2 | Sortie | Direction roues ×2 |
| SECONDARY_MOTOR_IN1–4 | Sortie | Direction moteur secondaire ×4 |
| PRI_MOTOR_SLEEP | Sortie | Mise en veille moteur primaire |
| PRI_MOTOR_NFAULT | Entrée | Détection défaut moteur primaire |
| SENSOR_PULSE_1–4 | Sortie | Impulsions capteurs ×4 |
| ENABLE_POWER_SUPPLIES | Sortie | Activation alimentation |
| PGOOD_24V | Entrée | Surveillance alimentation 24V |
| USER_SWITCH_1–4 | Entrée | Boutons utilisateur ×4 |
| SWITCH_1–4 | Entrée | Boutons de configuration ×4 |
| SERVO_ROBOTIS_TX_ENABLE | Sortie | Enable TX half-duplex Robotis |
| ETH_OSC_OUT | Sortie (MCO) | Horloge 25 MHz vers PHY Ethernet |

### FreeRTOS / Middlewares
| Élément | Détail |
|---|---|
| Tâche `defaultTask` | Priorité Normal, stack 4 KB |
| LWIP | Stack Ethernet, IP statique 192.168.0.123 |
| USB Device (CDC) | Port série virtuel |
| `cppMain()` | Point d'entrée application C++ |

---

## Tests unitaires — PC host (x86, sans STM32)

Les tests tournent entièrement sur PC via Google Test. Aucune carte STM32, aucun toolchain ARM requis.

### Prérequis

**MSYS2 MinGW64** doit être installé sur `D:\msys64\` (installeur officiel : [msys2.org](https://www.msys2.org)).

Vérifier que le dossier `D:\msys64\mingw64\bin\` existe, puis ouvrir un terminal (PowerShell, CMD, ou Git Bash) et installer les outils de build :

```powershell
# Dans PowerShell ou CMD
"D:\msys64\mingw64.exe" pacman -S --noconfirm mingw-w64-x86_64-cmake
```

> Si MSYS2 est installé ailleurs (ex. `C:\msys64\`), adapter le chemin dans toutes les commandes ci-dessous.

Les outils nécessaires après installation :

| Outil | Chemin |
|---|---|
| `g++` (compilateur C++) | `D:\msys64\mingw64\bin\g++.exe` |
| `cmake` | `D:\msys64\mingw64\bin\cmake.exe` |
| `ninja` | `D:\msys64\mingw64\bin\ninja.exe` |

---

### Structure des tests

```
robot-cdr/Tests/
├── CMakeLists.txt          ← point d'entrée CMake
├── Stubs/                  ← headers qui remplacent FreeRTOS/HAL sur host
│   ├── FreeRTOS.h          ← types FreeRTOS (QueueHandle_t, pdTRUE, …)
│   ├── queue.h             ← déclarations xQueueCreate, xQueueReceive, …
│   ├── task.h              ← déclarations vTaskDelay, xTaskCreate, …
│   ├── FreeRTOSStub.cpp    ← implémentation des queues (std::deque)
│   ├── MockHAL.h           ← HAL_GetTick() contrôlable par les tests
│   ├── stm32f4xx_hal.h     ← shadow du vrai header STM32 → inclut MockHAL.h
│   └── StaticDefs.cpp      ← définition de ExternalComm::latestSnapshot
├── Mocks/                  ← mocks des interfaces métier
│   ├── MockBus.h           ← implémente IBus, capture les publish()
│   ├── MockCommChannel.h   ← implémente ICommChannel, queues rx/tx
│   ├── MockMotorHAL.h      ← implémente IMotorHAL (Epic 2)
│   ├── MockEncoderHAL.h    ← implémente IEncoderHAL (Epic 2)
│   └── MockSensorHAL.h     ← implémente ISensorHAL (Epic 3)
└── Unit/
    ├── BusFormatTest.cpp   ← 9 tests sur les formats ASCII (TEL, ALT, LOG, HLT)
    └── ExternalCommTest.cpp ← 6 tests CMD parsing, publish, snapshot
```

---

### Compilation et exécution

#### Étape 1 — Configurer le projet CMake

Ouvrir **PowerShell** (ou CMD) et exécuter :

```powershell
# Ajouter MinGW64 au PATH pour cette session
$env:PATH = "D:\msys64\mingw64\bin;D:\msys64\usr\bin;" + $env:PATH

# Se placer dans le dossier Tests et créer le répertoire de build
cd "C:\<chemin_vers_le_projet>\robot-cdr\Tests"
mkdir build            # si le dossier n'existe pas encore
cd build

# Configurer CMake (génération des fichiers de build Ninja)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

Sortie attendue (extrait) :
```
-- The CXX compiler identification is GNU 15.x.x
-- Configuring done
-- Generating done
-- Build files have been written to: .../Tests/build
```

> La première exécution télécharge Google Test v1.14.0 depuis GitHub (~5 MB). Une connexion internet est requise la première fois. Les runs suivants utilisent le cache local dans `Tests/build/_deps/`.

#### Étape 2 — Compiler

Toujours depuis `Tests/build/` :

```powershell
ninja
```

Sortie attendue :
```
[16/16] Linking CXX executable ExternalCommTest.exe
```

Les deux exécutables sont créés dans `Tests/build/` :
- `BusFormatTest.exe`
- `ExternalCommTest.exe`

#### Étape 3 — Lancer les tests

```powershell
# Tests BusFormat (9 tests)
.\BusFormatTest.exe

# Tests ExternalComm (6 tests)
.\ExternalCommTest.exe
```

Résultat attendu pour chaque exécutable :
```
[==========] Running N tests from 1 test suite.
[  PASSED  ] N tests.
```

---

### Commandes en une seule ligne (build + run complet)

```powershell
$env:PATH = "D:\msys64\mingw64\bin;D:\msys64\usr\bin;" + $env:PATH
cd "C:\<chemin>\robot-cdr\Tests\build"
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug; ninja; .\BusFormatTest.exe; .\ExternalCommTest.exe
```

---

### Rebuild après modification de code

Après avoir modifié un fichier source (`.cpp` ou `.h`), relancer simplement :

```powershell
cd "C:\<chemin>\robot-cdr\Tests\build"
ninja
```

CMake détecte automatiquement les fichiers modifiés — pas besoin de reconfigurer sauf si `CMakeLists.txt` change.

---

### Ajouter de nouveaux tests

1. Créer un nouveau fichier `Tests/Unit/MonTest.cpp` avec les tests GTest.
2. Dans `Tests/CMakeLists.txt`, ajouter un exécutable :
   ```cmake
   add_executable(MonTest
       Unit/MonTest.cpp
       ${APP_DIR}/MonModule.cpp
   )
   target_include_directories(MonTest PRIVATE ${COMMON_INCLUDES})
   target_link_libraries(MonTest PRIVATE GTest::gtest_main)
   gtest_discover_tests(MonTest)
   ```
3. Relancer `cmake .. -G Ninja` puis `ninja`.

---

### Dépannage

| Symptôme | Cause probable | Solution |
|---|---|---|
| `cmake: command not found` | cmake non installé | `"D:\msys64\mingw64.exe" pacman -S --noconfirm mingw-w64-x86_64-cmake` |
| `fatal error: gtest/gtest.h: No such file` | Première config sans internet | Vérifier la connexion, supprimer `build/_deps/` et relancer `cmake ..` |
| Erreur de compilation `undefined reference` | Nouveau membre statique non défini | Ajouter la définition dans `Tests/Stubs/StaticDefs.cpp` |
| Test qui ne se termine pas | Appel direct à une tâche FreeRTOS sans `try/catch` | Entourer l'appel de `try { ... } catch (const TaskDelayEscape&) {}` |
| Avertissement `DOWNLOAD_EXTRACT_TIMESTAMP` | CMake >= 3.24 | Ignorable (warning dev uniquement), n'affecte pas le build |



Lancer le script
`& "D:\msys64\usr\bin\bash.exe" -l "C:\Users\gdurand\Downloads\bmad - robot CDR\robot-cdr\Tests\run_tests.sh"`