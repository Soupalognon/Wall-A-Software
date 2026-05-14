---
stepsCompleted: [1, 2, 3, 4]
inputDocuments:
  - '_bmad-output/planning-artifacts/architecture.md'
  - 'robot-cdr/App/Tasks/OdoControl.h'
session_topic: 'Conception et implémentation de la boucle odométrie/asservissement moteur (OdoControl 200Hz)'
session_goals: 'Conception complète OdoControl — odométrie, PID, interfaces HAL, architecture tâche 200Hz'
selected_approach: 'ai-recommended'
techniques_used: ['Morphological Analysis', 'First Principles Thinking', 'Constraint Mapping', 'Reverse Brainstorming']
ideas_generated: [25]
implementation_order: ['A-HAL Interfaces', 'D-Config', 'B-Control Logic', 'C-Odometry Math', 'E-Robustness']
workflow_completed: true
context_file: '_bmad-output/planning-artifacts/architecture.md'
---

# Brainstorming Session Results

**Facilitateur:** Gdurand
**Date:** 2026-05-13

## Session Overview

**Sujet :** Conception et implémentation de la boucle odométrie/asservissement moteur — `OdoControl` (tâche FreeRTOS 200Hz, haute priorité)

**Contexte architectural chargé :**
- Tâche temps-réel stricte, 200Hz, `vTaskDelayUntil`, non-interruptible
- Lecture encodeurs via `IEncoderHAL*` injecté
- Commande moteurs via `IMotorHAL*` injecté
- Setpoint reçu par mailbox `xQueueOverwrite` depuis `MotionPlanner`
- Publication télémétrie `IBus` (TEL ODO) via `BusFormat`
- Snapshot statique `OdoSnapshot` pour Monitoring (pull model)
- Zéro allocation dynamique — statique uniquement
- Stub actuel : `OdoControl.h` ne contient que `OdoSnapshot`

### Session Setup

_Contexte architecture complet chargé depuis `architecture.md`. Session focalisée sur la conception interne de la tâche OdoControl : algorithme PID, calcul odométrie, interfaces, gestion du setpoint._

---

## Technique Selection

**Approche :** Techniques recommandées par l'IA
**Contexte d'analyse :** Conception embarquée C++ temps-réel, STM32 FreeRTOS, PID différentiel

**Séquence recommandée :**
1. **Morphological Analysis** — cartographie exhaustive de toutes les dimensions de design
2. **First Principles Thinking** — validation des formules odométrie et PID depuis la physique
3. **Constraint Mapping** — vérification de chaque décision contre les contraintes temps-réel
4. **Reverse Brainstorming** — identification des modes de défaillance → exigences défensives

---

## Idées Générées (25 idées)

### Thème A — Interfaces HAL

**[Interfaces #1]** : Séparation `IEncoderHAL` / `IOdomHAL`
_Concept_ : `IEncoderHAL` expose les ticks bruts cumulés 32-bit. `IOdomHAL` calcule et expose position `(x, y, angle)` + vitesses `(vL, vR, v, ω)`. `OdoControl` ne touche que `IOdomHAL`.
_Novelty_ : Changement de modèle cinématique sans toucher `OdoControl` — isolation totale du calcul odométrique.

**[Encoder #13]** : `IEncoderHAL` wraps `Encoder::refresh()` — position 32-bit par accumulation de deltas `int16_t`
_Concept_ : Réutilisation directe du code Wall-A (`DriversCustom/Encoder/Encoder.cpp`). Le cast `(int16_t)(current - old)` gère l'overflow du compteur timer 16-bit via l'arithmétique modulaire.
_Novelty_ : Zéro condition spéciale pour le rollover — la soustraction signée 16-bit est la solution, pas un workaround.

**[HAL #3]** : `IMotorHAL` wraps `Drv8262::setMotors(float, float)`
_Concept_ : `IMotorHAL { virtual void setMotors(float left, float right) = 0; }`. Driver Wall-A réutilisable tel quel. `isError()` peut exposer l'état NFAULT.
_Novelty_ : Zéro réécriture — adapter pattern pur, le test unitaire injecte un mock sans matériel.

**[Arch #18]** : `ConcreteOdomHAL` détient `Encoder` par valeur — zéro indirection à ce niveau
_Concept_ : `OdoControl` → `IOdomHAL*` (un niveau virtuel pour les tests). `ConcreteOdomHAL` → `Encoder _encoderL, _encoderR` par valeur (direct, pas de virtuel).
_Novelty_ : Meilleur compromis testabilité/performance — le virtuel existe là où il est utile, pas en cascade.

**[Timing #16]** : `getDt()` placeholder dans `IOdomHAL`
_Concept_ : `ConcreteOdomHAL` implémente `getDt()` en lisant un timer CubeMX configuré par l'utilisateur (µs ou dizaine de µs). Pas d'interface `ITimerHAL` supplémentaire.
_Novelty_ : Zéro abstraction inutile — l'utilisateur branche son timer au bon endroit sans couche intermédiaire.

---

### Thème D — Configuration `Config.h`

**[Config #14]** : Facteurs de signe `ENCODER_L_SIGN`, `ENCODER_R_SIGN`, `MOTOR_L_SIGN`, `MOTOR_R_SIGN`
_Concept_ : Quatre `constexpr int8_t` valant +1 ou -1 dans `Config.h`. Appliqués au plus tôt dans la chaîne : signe encodeur dans `IEncoderHAL`, signe moteur dans `IMotorHAL`.
_Novelty_ : Correction de câblage = changer une constante compile-time, pas chercher un bug dans les formules.

**[HAL #9]** : Clamping délégué au driver — `dutyToCCR` dans `Drv8262` clamp `[-1, 1]`
_Concept_ : Le driver Wall-A existant clamp dans `dutyToCCR` avec `std::max(-1.0f, std::min(1.0f, duty))`. Filet de sécurité ultime déjà implémenté.
_Novelty_ : `OdoControl` peut logger la valeur non-clampée dans `OdoSnapshot` pour détecter une saturation persistante (signe de gains PID trop agressifs).

**[Robustness #25]** : `Config::MAX_DUTY` — cap consigne moteur au niveau `OdoControl`
_Concept_ : `constexpr float MAX_DUTY = 0.8f`. OdoControl clamp sa sortie PID à `[-MAX_DUTY, +MAX_DUTY]` avant `IMotorHAL::setMotors()`. Paramètre tunable sans recompiler le driver.
_Novelty_ : Permet de commencer prudemment à 50% lors de la mise en route avant de monter progressivement.

**[Timing #10]** : Diviseur de fréquence 200Hz → 20Hz pour les publications
_Concept_ : `_tickCount % 10 == 0` déclenche la mise à jour `OdoSnapshot` et la publication `TEL ODO`. Fréquence configurable via une constante dans `Config.h`.
_Novelty_ : Découplage propre — changer la fréquence de télémétrie = modifier une seule constante.

**Constantes `Config.h` à ajouter (synthèse) :**
```cpp
constexpr float    ARRIVAL_THRESHOLD    = 0.02f;   // m
constexpr float    PID_I_MAX_SPEED      = 0.5f;
constexpr float    PID_I_MAX_ANGLE      = 0.5f;
constexpr float    MAX_DUTY             = 0.8f;
constexpr float    STALL_DUTY_THRESHOLD = 0.5f;
constexpr uint32_t STALL_TIME_MS        = 500;
constexpr int8_t   ENCODER_L_SIGN       = +1;
constexpr int8_t   ENCODER_R_SIGN       = +1;
constexpr int8_t   MOTOR_L_SIGN         = +1;
constexpr int8_t   MOTOR_R_SIGN         = +1;
constexpr uint8_t  TELEM_DIVIDER        = 10;      // 200Hz / 10 = 20Hz
```

---

### Thème B — Logique de contrôle OdoControl

**[Control #2]** : Double mode asservissement — position ou vitesse via `constexpr`
_Concept_ : Une constante compile-time détermine si la boucle fermée porte sur la position `(x, y, angle)` ou la vitesse `(v, ω)`. Le compilateur élimine le code du mode non-utilisé.
_Novelty_ : Zéro overhead runtime — deux chemins de code propres sans `if` dans la boucle critique.

**[Interfaces #6]** : Setpoint struct union — `PoseTarget` ou `VelocityTarget`
_Concept_ : La mailbox transporte `struct Setpoint { Mode mode; union { PoseTarget{x,y,angle}; VelocityTarget{v,w} }; }`. Un seul type de message, `MotionPlanner` n'a pas besoin de connaître le mode de contrôle.
_Novelty_ : Mailbox unifiée — extensible si un troisième mode est ajouté plus tard.

**[Control #5]** : Deux PIDs orthogonaux — `PID_speed` sur `v`, `PID_angle` sur `ω`
_Concept_ : Gains complètement indépendants pour la vitesse linéaire et la rotation. Tuning découplé — ajuster l'agressivité angulaire sans perturber le comportement en ligne droite.
_Novelty_ : Deux espaces de contrôle physiquement distincts (m/s vs rad/s) — gains naturellement différents.

**[Control #4]** : Décomposition linéaire/angulaire `leftDuty = v - ω`, `rightDuty = v + ω`
_Concept_ : Un seul espace de contrôle (v, ω) indépendant de la géométrie roue-à-roue. Changer l'entraxe ne touche pas les PIDs, juste le facteur de conversion.
_Novelty_ : Post-traitement minimal après les PIDs — séparation nette entre contrôle et géométrie.

**[Control #7]** : Erreur angulaire = cap vers la cible `atan2(Δy, Δx) - angle`
_Concept_ : En mode position, `PID_angle` reçoit l'erreur de heading vers le waypoint. Trajectoire convergente naturelle en courbe sans machine d'état explicite.
_Novelty_ : La convergence émerge de la dynamique des deux PIDs — pas de séquençage "tourner puis avancer".

**[Control #8]** : Condition d'arrivée — moteurs zéro + publication IBus `ARRIVAL`
_Concept_ : Quand `distance < Config::ARRIVAL_THRESHOLD`, `setMotors(0, 0)` + `BusFormat::evtArrival()` sur IBus. `MotionPlanner` reçoit l'événement et envoie le prochain waypoint.
_Novelty_ : `OdoControl` reste stateless entre waypoints — toute la logique de séquence appartient à `MotionPlanner`.

**[PID #11]** : Anti-windup — clamp intégral `[-I_MAX, +I_MAX]`
_Concept_ : Après `integral += error * dt`, clamp avec `Config::PID_I_MAX_SPEED` et `Config::PID_I_MAX_ANGLE`. Reset intégral au moment de la détection de blocage.
_Novelty_ : Comportement entièrement prévisible et testable — le test unitaire vérifie que l'intégrale ne dépasse jamais les bornes.

**[Arch #17]** : `OdoControl` injecte `IOdomHAL*` — testabilité via injection
_Concept_ : `OdoControl(IOdomHAL*, IMotorHAL*, IBus*)` — toutes les dépendances via constructeur. `SystemInit` instancie les objets concrets statiquement.
_Novelty_ : Tests unitaires sur PC avec mocks injectés sans matériel — garantie de comportement avant le premier flash.

**[Constraint #21]** : Flag `_hasSetpoint` — comportement sûr au boot
_Concept_ : `bool _hasSetpoint = false`. Au premier tick, si `xQueuePeek` retourne `pdFALSE`, moteurs à zéro, PIDs non calculés.
_Novelty_ : Sans ce guard, le PID démarre avec un setpoint indéfini — comportement imprévisible au boot.

**Séquence d'un tick 200Hz (confirmée) :**
```
1.  vTaskDelayUntil()            ← réveil déterministe
2.  IOdomHAL::update()           ← lit encodeurs + dt, intègre position
3.  xQueuePeek(mailbox, 0)       ← setpoint non-bloquant
4.  if (!_hasSetpoint) → stop    ← guard boot
5.  calcul erreurs distance/cap  ← mode position uniquement
6.  PID_speed.compute(err, dt)
7.  PID_angle.compute(err, dt)
8.  clamp(v, ω, MAX_DUTY)
9.  décomposition v/ω → L/R
10. IMotorHAL::setMotors(L, R)
11. if (tickCount % TELEM_DIVIDER) → OdoSnapshot update
12. if (tickCount % TELEM_DIVIDER) → IBus TEL ODO
```

---

### Thème C — Odométrie et mathématiques

**[Odom #12]** : Intégration Euler différentielle à 200Hz
_Concept_ :
```cpp
// Constante compile-time
constexpr float D_PER_TICK = (2.0f * M_PI * WHEEL_RADIUS) / TICKS_PER_REV;

// Chaque tick
float dL = deltaTicksL * D_PER_TICK;
float dR = deltaTicksR * D_PER_TICK;
float dd = (dL + dR) * 0.5f;
float dTheta = (dR - dL) / WHEELBASE;
_angle += dTheta;
_x += dd * cosf(_angle);
_y += dd * sinf(_angle);
_vL = dL / dt;  _vR = dR / dt;
_v  = (_vL + _vR) * 0.5f;
_w  = (_vR - _vL) / WHEELBASE;
```
_Novelty_ : À 200Hz les erreurs Euler sont négligeables (~`Δθ²/6` par pas) — Runge-Kutta inutile.

**[Timing #15]** : `dt` via timer hardware µs — précision 10-100× supérieure à `HAL_GetTick`
_Concept_ : Timer libre CubeMX (ex: TIM6, 1µs/tick). `dt = (getMicros() - _lastMicros) * 1e-6f`. Implémentation laissée à l'utilisateur via `getDt()` dans `ConcreteOdomHAL`.
_Novelty_ : Absorbe le jitter FreeRTOS avec précision — `dt` réel ~4.9-5.1ms mesuré à 1µs près.

**[Constraint #19]** : `atan2f` float sur Cortex-M4 FPU — ~0.3µs, négligeable
_Concept_ : STM32F303 FPU — `atan2f` prend ~20-50 cycles à 72MHz. Sur 5ms de budget : 0.003% du temps. Toujours utiliser `atan2f` (float), jamais `atan2` (double).
_Novelty_ : La FPU rend les calculs trigonométriques gratuits — pas besoin d'approximations.

**[Control #20]** : Normalisation angle `[-π, +π]` après chaque intégration
_Concept_ : Après `_angle += dTheta` :
```cpp
if (_angle > M_PI)  _angle -= 2.0f * M_PI;
if (_angle < -M_PI) _angle += 2.0f * M_PI;
```
_Novelty_ : Sans normalisation, après ~1800° de rotation le float perd de la précision et l'erreur PID angulaire peut sauter de +π à -π au lieu de passer par 0.

---

### Thème E — Robustesse et détection de défaillances

**[Robustness #22]** : Détection blocage moteur → `ALT STALL` + moteurs zéro
_Concept_ : Si `|duty| > STALL_DUTY_THRESHOLD` ET vitesse mesurée ≈ 0 pendant `STALL_TIME_MS` → `bus_->publish(ALERT, BusFormat::altStall())` + `setMotors(0,0)` + reset intégral PID.
_Novelty_ : Reset intégral au moment du déblocage évite les oscillations au redémarrage.

**[Robustness #23]** : Détection encodeur silencieux → `ALT ENCODER_FAULT`
_Concept_ : Si `|duty| > STALL_DUTY_THRESHOLD` ET `|ΔticksL| == 0` (ou `|ΔticksR| == 0`) pendant N ticks → `ALT ENCODER_FAULT LEFT` (ou `RIGHT`).
_Novelty_ : Combinaison commande élevée + zéro ticks est physiquement impossible sauf défaillance — discriminant fiable.

**[Robustness #24]** : Latence `MotionPlanner` absorbée par `xQueueOverwrite`
_Concept_ : Mailbox taille 1 + `xQueueOverwrite` garantit que `OdoControl` lit toujours le setpoint le plus récent. Si `MotionPlanner` est lent, `OdoControl` continue sur le dernier setpoint connu.
_Novelty_ : Aucune modification nécessaire — c'est précisément pourquoi l'architecture a choisi `xQueueOverwrite`.

---

## Organisation et priorités d'implémentation

**Ordre retenu : A → D → B → C → E**

| Étape | Livrable | Dépendances |
|-------|----------|-------------|
| **1 — Interfaces HAL** | `IEncoderHAL.h`, `IOdomHAL.h`, `IMotorHAL.h`, `ConcreteOdomHAL.h/.cpp`, `MotorHAL.h/.cpp` | Wall-A Encoder + Drv8262 |
| **2 — Config** | `App/Config.h` — toutes les constantes OdoControl | Interfaces HAL |
| **3 — Logique contrôle** | `OdoControl.h/.cpp` complet, struct `Setpoint`, classe `Pid` | Config + Interfaces |
| **4 — Maths odométrie** | Implémentation `ConcreteOdomHAL::update()` + timer µs | IEncoderHAL concret |
| **5 — Robustesse** | Détection stall, encodeur fault, guard boot | OdoControl fonctionnel |

---

## Résumé de session

**25 idées générées** à travers 4 techniques sur la conception complète d'`OdoControl`.

**Décisions clés prises :**
- Architecture à deux niveaux d'abstraction HAL — `IEncoderHAL` (ticks) + `IOdomHAL` (cinématique)
- Deux PIDs orthogonaux (v, ω) + décomposition différentielle en post-traitement
- Setpoint union polymorphe sur mailbox unique `xQueueOverwrite`
- Erreur angulaire = cap vers cible (`atan2f`) avec condition d'arrivée explicite
- `dt` via timer hardware µs, `getDt()` placeholder dans `ConcreteOdomHAL`
- 10 constantes `Config.h` défensives identifiées

**Réutilisations du projet Wall-A :**
- `Encoder.cpp/.hpp` → wrappé derrière `IEncoderHAL` (adaptation minimale)
- `Drv8262.cpp/.hpp` → wrappé derrière `IMotorHAL` (adapter pattern pur)
