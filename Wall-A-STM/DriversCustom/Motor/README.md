# API Driver DRV8262 pour Wall-A

## Vue d'ensemble

Driver pour contrôler 2 moteurs DC avec le circuit intégré **DRV8262DDVR** (Dual H-Bridge de Texas Instruments).

## Configuration Matérielle

### Connexions PWM (Timer 5)
- **TIM5_CH1** (PH10) → PWM Moteur Gauche (`SECONDARY_MOTOR_PWM1`)
- **TIM5_CH2** (PH11) → PWM Moteur Droit (`SECONDARY_MOTOR_PWM2`)

### Connexions Direction (GPIO)
- **PG3** → IN1 (Direction moteur gauche)
- **PG4** → IN2 (Direction moteur gauche)
- **PG5** → IN3 (Direction moteur droit)
- **PG6** → IN4 (Direction moteur droit)

### Connexion Enable
- **PF1** → SLEEP (Enable global des moteurs)

## Utilisation de l'API

### Initialisation

```cpp
#include "Drv8262.hpp"
using namespace DriversCustom::Motor;

Drv8262 motorDriver;

// Dans votre fonction d'init (après MX_TIM5_Init)
motorDriver.init();  // Démarre les PWM et active le driver
```

### Contrôle Simple des Moteurs

```cpp
// Faire avancer les deux roues à 50%
motorDriver.setMotors(0.5f, 0.5f);

// Faire reculer les deux roues à 30%
motorDriver.setMotors(-0.3f, -0.3f);

// Tourner sur place (rotation droite)
motorDriver.setMotors(0.4f, -0.4f);

// Arrêter les moteurs
motorDriver.stop();
```

### Contrôle Individuel

```cpp
// Moteur gauche uniquement (avant à 70%)
motorDriver.setLeftDuty(0.7f);

// Moteur droit uniquement (arrière à 40%)
motorDriver.setRightDuty(-0.4f);
```

### Enable/Disable

```cpp
// Activer les moteurs
motorDriver.enable(true);

// Mettre en mode veille (économie d'énergie)
motorDriver.enable(false);
```

## Référence API

### `void init()`
Initialise le driver et démarre les timers PWM. **À appeler une seule fois au démarrage.**

### `void setLeftDuty(float duty)`
Contrôle le moteur gauche.
- **Paramètre:** `duty` = -1.0 (arrière max) à +1.0 (avant max)
- Valeurs proches de 0 = freinage

### `void setRightDuty(float duty)`
Contrôle le moteur droit.
- **Paramètre:** `duty` = -1.0 (arrière max) à +1.0 (avant max)

### `void setMotors(float leftDuty, float rightDuty)`
Contrôle les deux moteurs simultanément.
- **Paramètres:** duty gauche et droit (-1.0 à +1.0)

### `void stop()`
Arrête immédiatement les deux moteurs (duty = 0).

### `void enable(bool en)`
Active ou désactive le driver.
- `true` = moteurs actifs
- `false` = mode veille (SLEEP)

## Exemples d'Utilisation

### Exemple 1: Mouvement Basique

```cpp
void testMotors() {
    Drv8262 motors;
    motors.init();
    
    // Avancer 2 secondes
    motors.setMotors(0.5f, 0.5f);
    osDelay(2000);
    
    // Reculer 2 secondes
    motors.setMotors(-0.5f, -0.5f);
    osDelay(2000);
    
    // Arrêter
    motors.stop();
}
```

### Exemple 2: Rotation

```cpp
void turnRight(float speed, uint32_t durationMs) {
    Drv8262 motors;
    motors.init();
    
    // Rotation sur place
    motors.setMotors(speed, -speed);
    osDelay(durationMs);
    motors.stop();
}
```

### Exemple 3: Dans une Tâche FreeRTOS

Voir [MotorTask.cpp](../../Tasks/MotorTask.cpp) pour un exemple complet d'intégration avec FreeRTOS incluant:
- Séquence de test automatique
- Gestion de l'état d'urgence
- Boucle de contrôle à 200Hz

## Notes Importantes

1. **Fréquence PWM**: Le TIM5 doit être configuré pour une fréquence PWM appropriée (typiquement 20kHz pour moteurs DC)

2. **Limites de Courant**: Le DRV8262 a une protection intégrée, mais surveillez la température et le courant

3. **Dead-time**: Le driver gère automatiquement le dead-time pour éviter les court-circuits

4. **Pin NFAULT**: La pin `PRI_MOTOR_NFAULT` (PF2) peut être lue pour détecter les erreurs du driver

5. **Calibration**: Les valeurs de duty peuvent nécessiter un ajustement selon votre charge mécanique

## Dépannage

| Problème | Solution |
|----------|----------|
| Moteurs ne bougent pas | Vérifier que `enable(true)` est appelé après `init()` |
| Mouvement erratique | Vérifier les connexions GPIO et que TIM5 est bien configuré en PWM |
| Surchauffe | Réduire le duty cycle ou améliorer le refroidissement |
| Moteur dans un seul sens | Vérifier les connexions IN1-IN4 |

## Configuration CubeMX Requise

Assurez-vous que dans votre fichier `.ioc`:
- TIM5 est en mode PWM avec CH1 et CH2 activés
- Les GPIO PG3-PG6 sont configurés en sortie (GPIO_Output)
- PF1 (SLEEP) est configuré en sortie (GPIO_Output)

## Licence

Copyright (c) 2025 - Wall-A Project
