---
stepsCompleted: [1, 2, 3, 4]
session_topic: Architecture logicielle C++ orientée objet pour robot autonome STM32/FreeRTOS
session_goals: Cartographier les dépendances entre classes, définir une architecture cohérente et évolutive, respecter les contraintes temps-réel FreeRTOS CMSIS V2 + HAL
selected_approach: ai-recommended
techniques_used: [First Principles Thinking, Morphological Analysis, Constraint Mapping]
ideas_generated: 16
session_active: false
workflow_completed: true
---

# Session de Brainstorming — Architecture Robot Autonome STM32

**Date :** 2026-05-09
**Projet :** bmad - robot CDR
**Participant :** Gdurand

---

## Session Overview

**Topic :** Architecture logicielle C++ orientée objet pour un robot autonome sur STM32 avec HAL et FreeRTOS CMSIS V2

**Composants matériels :**
- 2 moteurs principaux (gauche/droite) avec encodeurs
- Capteurs de proximité (nombre/type à définir)
- Capteurs de température et de courant (santé PCB)
- Pompes, servomoteurs, transducteurs linéaires
- Communication USB + UART + Ethernet vers PC

**Objectifs :**
1. Plan de dépendances entre classes C++
2. Architecture cohérente et évolutive (non bloquée dans le temps)
3. Bonnes pratiques pour microcontrôleur STM32 + HAL + FreeRTOS CMSIS V2

---

## Technique Selection

**Approche :** Techniques recommandées par l'IA
**Séquence :** First Principles Thinking → Morphological Analysis → Constraint Mapping

---

## Décisions Architecturales — 16 Insights

### Thème 1 : Infrastructure de Communication

**[Architecture #1] — Bus pub/sub unifié via `IBus`**
_Concept_ : Une seule infrastructure de communication pour monitoring, alertes, logs et telemetry PC. Les modules publient via une interface `IBus*`, le monitoring souscrit. Zéro couplage direct entre modules.
_Nouveauté_ : Élimine la dépendance monitoring→modules et unifie trois problèmes distincts en une seule solution. Testabilité native via `MockBus`.

**[Architecture #13] — Interface `IBus` par injection de dépendance**
_Concept_ : Tous les modules reçoivent un `IBus*` à la construction. `ExternalComm` implémente `IBus`. Zéro module ne connaît `ExternalComm` directement — la dépendance circulaire est brisée.
_Nouveauté_ : Testabilité native — on peut injecter un `MockBus` en test unitaire sans HAL.

**[Architecture #12] — Bus pub/sub intégré à ExternalComm, une queue par topic**
_Concept_ : `ExternalComm` héberge le bus. Chaque topic (`TELEMETRY`, `ALERT`, `LOG`, `HEALTH`) est une queue FreeRTOS dédiée. Les modules publient via `IBus`, `ExternalComm` consomme et route vers USB/UART/Ethernet.
_Nouveauté_ : Le routage bus→canal physique est centralisé en un seul endroit.

**[Architecture #16] — Politique de queue configurable par topic dans `SystemInit`**
_Concept_ : Un tableau de config `BusQueueConfig[]` dans `SystemInit` définit politique et profondeur par topic. `ExternalComm` instancie ses queues depuis ce tableau.
_Nouveauté_ : Comportement du bus modifiable sans toucher au code des modules.

```cpp
static constexpr BusQueueConfig BUS_CONFIG[] = {
    { Topic::TELEMETRY, QueuePolicy::OVERWRITE,    1  },
    { Topic::LOG,       QueuePolicy::DROP_SILENT,  20 },
    { Topic::ALERT,     QueuePolicy::DROP_SILENT,  5  },
    { Topic::HEALTH,    QueuePolicy::OVERWRITE,    1  },
};
```

---

### Thème 2 : Boucle de Contrôle Temps-Réel

**[Architecture #2] — Asservissement en îlot isolé**
_Concept_ : La boucle de contrôle moteur est un système fermé. Elle publie ses données (vitesse, position, erreur PID) vers le bus en best-effort, mais reçoit ses consignes et pilote les moteurs via un canal dédié, hors bus.
_Nouveauté_ : Garantit le déterminisme temps-réel en supprimant toute dépendance vers l'infrastructure commune.

**[Architecture #7] — Task unique `OdoControl` à 200Hz**
_Concept_ : Une seule tâche FreeRTOS exécute séquentiellement : 1) calcul odométrie depuis encodeur, 2) boucle PID asservissement, 3) écriture commande moteur. Zéro partage mémoire inter-tâches, zéro protection nécessaire, déterminisme parfait.
_Nouveauté_ : Simplifie radicalement l'architecture en éliminant un problème de synchronisation à la source.

**[Architecture #9 — révisée] — Mailbox FreeRTOS pour la consigne**
_Concept_ : Queue depth-1 avec `xQueueOverwrite`/`xQueuePeek`. Sémantique "dernière valeur valide", thread-safe natif, flush intégré via `xQueueReset()`. OdoControl peek à chaque tick 200Hz sans jamais bloquer.
_Nouveauté_ : Primitif FreeRTOS idiomatique prévu pour ce cas exact — zéro code de synchronisation custom.

**[Architecture #6] — Shared memory horodatée**
_Concept_ : Un timestamp valide la fraîcheur de la donnée odométrique. Si trop vieux, l'asservissement peut déclencher une alarme ou geler la consigne.
_Nouveauté_ : Le timestamp transforme la donnée en valeur *qualifiée*, pas juste lue.

---

### Thème 3 : Planification et Réactivité

**[Architecture #4] — Chaîne de commande linéaire**
_Concept_ : `ExternalComm → MotionPlanner → [Mailbox] → OdoControl → Moteur`. MotionPlanner est le seul décideur de consigne, il intègre la réaction aux capteurs.
_Nouveauté_ : Un seul point de décision de mouvement — simplifie la cohérence et la sécurité.

**[Architecture #3] — Queue de consigne dédiée avec flush**
_Concept_ : Canal unidirectionnel exclusif vers l'asservissement, avec capacité d'invalidation immédiate (flush) de la consigne courante — mécanisme d'arrêt d'urgence natif via `xQueueReset()`.
_Nouveauté_ : Le flush devient le primitif de sécurité universel.

**Alarmes capteurs → `xTaskNotify`**
_Concept_ : `SensorManager` envoie un `xTaskNotify()` avec bitmask vers la tâche `MotionPlanner` lors d'une alarme. Chaque bit = un type d'alarme. MotionPlanner se réveille et interroge l'état.
_Nouveauté_ : Latence ~1 tick, zéro copie, natif FreeRTOS. Pas de pub/sub pour les chemins critiques.

---

### Thème 4 : Abstraction Hardware

**[Architecture #5] — Odometry possède l'encodeur**
_Concept_ : `OdoControl` est le seul propriétaire des abstractions encodeur HAL. Elle calcule position et vitesse mesurée — l'asservissement ne touche jamais le HAL directement.
_Nouveauté_ : Séparation claire hardware/logique, un seul lecteur HAL par périphérique.

**[Architecture #14] — Hiérarchie générique `ISensor` / `IActuator`**
_Concept_ : Interfaces génériques avec id/nom comme invariants. Implémentations concrètes : `ProximitySensor`, `TemperatureSensor`, `CurrentSensor`, `Pump`, `Servo`, `LinearTransducer`. `SensorManager` et `ActuatorManager` orchestrent des collections de taille fixe (MAX_SENSORS=15, MAX_ACTUATORS=10).
_Nouveauté_ : Le système s'étend sans modifier le code existant — ajout d'un capteur = nouvelle classe concrète uniquement.

---

### Thème 5 : Communication Externe

**[Architecture #10] — ExternalComm dual-canal avec priorité UART**
_Concept_ : UART = canal de commande prioritaire (opérateur terrain), USB = canal supervision/télémétrie haut débit, Ethernet = supervision réseau. En cas de consigne simultanée, UART écrase.
_Nouveauté_ : La topologie physique reflète la hiérarchie d'autorité.

**[Architecture #11] — Logger utilitaire d'ExternalComm**
_Concept_ : `Logger` est une classe interne à `ExternalComm`, avec une queue non-bloquante. Destination configurable par constante de compilation (`LOG_TARGET = USB | UART | ETH | ALL`).
_Nouveauté_ : La destination de log est une décision de build — simplifie le code embarqué.

---

### Thème 6 : Assemblage Système

**[Architecture #8] — Pompes sous contrôle direct ExternalComm**
_Concept_ : `ExternalComm` commande `ActuatorManager` via `IActuatorManager`. Les actionneurs ne passent pas par le `MotionPlanner` — séparation locomotion/actionnement.
_Nouveauté_ : Modifications des actionneurs indépendantes de la logique de mouvement.

**[Architecture #15] — `SystemInit` — câblage statique unique**
_Concept_ : `SystemInit::boot()` instancie tous les objets en mémoire statique et injecte les dépendances. Zéro allocation dynamique (`new` interdit), empreinte mémoire connue à la compilation. `main.cpp` = 3 lignes.
_Nouveauté_ : Sur STM32, l'absence de heap élimine une classe entière de bugs (fragmentation, allocation failed silencieuse).

---

## Graphe de Dépendances Final

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
COUCHE HAL (STM32 CubeMX)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
IEncoderHAL  → impl: EncoderHAL (×2 L/R)
IMotorHAL    → impl: MotorHAL   (×2 L/R)
ISensorHAL   → impl: ProximitySensorHAL, TemperatureHAL, CurrentHAL, ...
IPumpHAL     → impl: PumpHAL
IServoHAL    → impl: ServoHAL
ILinearHAL   → impl: LinearHAL
IUartHAL     → impl: UartHAL
IUsbHAL      → impl: UsbHAL
IEthernetHAL → impl: EthernetHAL

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
INTERFACES DOMAINE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
IBus              → publish(Topic, data, len)
ISensor           → id, name, read()
IActuator         → id, name, command()
IActuatorManager  → commandById(id, cmd)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
CLASSES MÉTIER
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

OdoControl  [Task 200Hz — PRIORITÉ TRÈS HAUTE]
  ↳ IEncoderHAL* [2]     (left, right)
  ↳ IMotorHAL*   [2]     (left, right)
  ↳ Mailbox consigne     (xQueuePeek, depth=1)
  ↳ IBus*                (publie TELEMETRY)
  Exécution séquentielle : Odométrie → PID → Commande moteur

MotionPlanner  [Task — PRIORITÉ HAUTE, event-driven]
  ↳ IBus*                (publie TELEMETRY, ALERT)
  ↳ Mailbox consigne*    (xQueueOverwrite → OdoControl)
  ↳ reçoit xTaskNotify ← SensorManager (bitmask alarmes)

SensorManager  [Task — PRIORITÉ BASSE, polling]
  ↳ ISensor* [MAX_SENSORS=15]
  ↳ IBus*                (publie HEALTH, ALERT)
  ↳ envoie xTaskNotify → MotionPlanner (alarmes)

Capteurs concrets (impl ISensor) :
  ProximitySensor    ↳ ISensorHAL
  TemperatureSensor  ↳ ISensorHAL
  CurrentSensor      ↳ ISensorHAL
  [FutureSensor]     ↳ ISensorHAL

ActuatorManager  [on-demand] (impl IActuatorManager)
  ↳ IActuator* [MAX_ACTUATORS=10]
  ↳ IBus*                (publie TELEMETRY)

Actionneurs concrets (impl IActuator) :
  Pump              ↳ IPumpHAL
  Servo             ↳ IServoHAL
  LinearTransducer  ↳ ILinearHAL

ExternalComm  [Tasks RX haute / TX moyenne]
  ↳ IUartHAL*        (PRIORITAIRE — opérateur terrain)
  ↳ IUsbHAL*         (supervision)
  ↳ IEthernetHAL*    (supervision réseau)
  ↳ Logger           (queue non-bloquante, LOG_TARGET compile-time)
  ↳ impl IBus        (queues par topic selon BUS_CONFIG[])
  → commande MotionPlanner    (consignes mouvement)
  → commande IActuatorManager (commandes actionneurs)

Monitoring  [Task — PRIORITÉ BASSE, queue-driven]
  ↳ IBus*            (souscrit TELEMETRY, ALERT, HEALTH)
  ↳ logique seuils, agrégation, alertes

SystemInit  [boot uniquement]
  → instancie TOUT en mémoire statique
  → injecte tous les pointeurs (IBus*, HAL*, etc.)
  → configure BUS_CONFIG[]

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
FLUX DE DONNÉES PRINCIPAUX
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Commande mouvement :
  ExternalComm → MotionPlanner → [Mailbox xQueueOverwrite] → OdoControl → Moteurs

Boucle fermée :
  Encodeurs → OdoControl (odométrie + PID) → Moteurs

Alarme capteur :
  SensorManager → xTaskNotify → MotionPlanner → flush mailbox + nouvelle consigne

Télémétrie :
  Tous modules → IBus (TELEMETRY/HEALTH) → ExternalComm → USB/ETH → PC

Logs :
  Tous modules → IBus (LOG) → ExternalComm → Logger queue → UART/USB/ETH

Commande actionneur :
  ExternalComm → IActuatorManager → IActuator → HAL
```

---

## Tâches FreeRTOS

| Tâche | Priorité | Cadence | Mécanisme réveil |
|-------|----------|---------|-----------------|
| `OdoControl` | Très haute | 200Hz / 5ms | `vTaskDelayUntil` |
| `MotionPlanner` | Haute | Event-driven | `xTaskNotify` |
| `ExternalComm RX` | Haute | Event-driven | Queue RX |
| `ExternalComm TX` | Moyenne | Queue-driven | `IBus` queues |
| `SensorManager` | Basse | Polling configurable | `vTaskDelay` |
| `Monitoring` | Basse | Queue-driven | `IBus` queues |

---

## Contraintes Validées

| Contrainte | Statut | Solution |
|------------|--------|----------|
| Asservissement non-bloquant | ✅ | Task isolée, mailbox non-bloquante |
| Zéro allocation dynamique | ✅ | `SystemInit` statique, tableaux taille fixe |
| Alarmes capteurs rapides | ✅ | `xTaskNotify` ~1 tick, pas de pub/sub |
| Inversion de priorité IBus | ✅ | Publications non-bloquantes (overwrite/drop) |
| Stack prévisible | ✅ | PID simple, pas de récursion |
| Extensibilité capteurs/actionneurs | ✅ | `ISensor`/`IActuator` + managers |

---

## Prochaines Étapes Recommandées

1. **Implémenter les interfaces** : `IBus`, `ISensor`, `IActuator`, `IActuatorManager` — elles définissent tous les contrats
2. **Implémenter `SystemInit`** : câblage statique complet, valide la compilation de l'ensemble
3. **Implémenter `OdoControl`** : cœur temps-réel, à valider en premier sur hardware
4. **Implémenter `ExternalComm` + `IBus`** : infrastructure de communication, permet les tests système
5. **Implémenter `MotionPlanner`** : planification et réaction aux capteurs
6. **Implémenter `SensorManager`** + capteurs concrets : un par un selon besoin
7. **Implémenter `ActuatorManager`** + actionneurs concrets : un par un selon besoin
8. **Implémenter `Monitoring`** : seuils et alertes une fois les données disponibles

---

## Insights Clés de Session

- **Décision pivot :** Unifier odométrie et asservissement dans une seule task 200Hz élimine toute synchronisation inter-tâches sur le chemin critique
- **Pattern central :** `IBus` + injection de dépendance découple complètement tous les modules — chaque classe peut être testée isolément
- **Principe de sécurité :** Publications non-bloquantes sur `IBus` (overwrite/drop) préviennent toute inversion de priorité par construction
- **Extensibilité :** `ISensor`/`IActuator` permettent d'ajouter n'importe quel capteur ou actionneur futur sans modifier l'architecture existante
