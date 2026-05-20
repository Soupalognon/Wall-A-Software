# Guide de tuning — Asservissement vitesse Wall-A

## Architecture de contrôle

```
Consigne (v, w)
     │
     ├──► EMA setpoint ──────────────────────────────────────┐
     │    (SPEED_EMA_ALPHA / ANGLE_EMA_ALPHA)                │
     │    lisse les échelons de commande (~10 Hz → 200 Hz)   │
     │                                                        │
     │    ┌───── consigne lissée ──────────────────────────── ┤
     │    │                                                   │
     │    ├──► Feedforward ────────────────────────────────── ┤
     │    │    (duty ≈ FF_GAIN × consigne_lissée)             │
     │    │                                                   ▼
     │    └──► Erreur = consigne_lissée − mesure → PID ──► duty total → Moteur → Encodeur
     │                                                                         │
     │                                          EMA mesure ◄──────────────────┘
     │                                          (VEL_EMA_ALPHA — lisse le bruit encodeur)
```

Le **EMA setpoint** rampe chaque échelon de commande pour éviter les à-coups moteur.  
Le **feedforward** fournit ~95 % du duty nécessaire.  
Le **PID** corrige uniquement le résidu d'erreur.  
Le **filtre EMA mesure** élimine le bruit d'encodeur qui provoquerait des grésillements.

---

## Paramètres clés — `Config.h`

### Filtres EMA

| Paramètre | Valeur actuelle | Rôle |
|---|---|---|
| `VEL_EMA_ALPHA` | 0.1 | Lissage **mesure** encodeur (0=max lisse, 1=brut) |
| `SPEED_EMA_ALPHA` | 0.15 | Lissage **consigne** vitesse linéaire v |
| `ANGLE_EMA_ALPHA` | 0.05 | Lissage **consigne** vitesse angulaire w |

> Ces trois alphas sont indépendants. `VEL_EMA_ALPHA` agit sur la mesure ; `SPEED_EMA_ALPHA` et `ANGLE_EMA_ALPHA` agissent sur la consigne avant qu'elle n'entre dans le PID et le feedforward.

### Feedforward

| Paramètre | Valeur actuelle | Rôle |
|---|---|---|
| `FF_GAIN_V` | 0.58 | Duty par m/s — calibré expérimentalement |
| `FF_GAIN_W` | FF_GAIN_V × WHEEL_BASE / 2 | Duty par rad/s — dérivé automatiquement |

### PID vitesse linéaire (speed)

| Paramètre | Valeur actuelle | Rôle |
|---|---|---|
| `PID_KP_DEFAULT` | 0.3 | Gain proportionnel |
| `PID_KI_DEFAULT` | 0.1 | Gain intégral |
| `PID_KD_DEFAULT` | 0.0 | Gain dérivé |
| `PID_I_MAX_SPEED` | 1.0 | Plafond de l'intégrale |

### PID vitesse angulaire (angle)

| Paramètre | Valeur actuelle | Rôle |
|---|---|---|
| `PID_KP_ANGLE_DEFAULT` | 0.6 | Gain proportionnel |
| `PID_KI_ANGLE_DEFAULT` | 0.1 | Gain intégral |
| `PID_KD_ANGLE_DEFAULT` | 0.01 | Gain dérivé |
| `PID_I_MAX_ANGLE` | 0.5 | Plafond de l'intégrale |

### Limites

| Paramètre | Valeur actuelle | Rôle |
|---|---|---|
| `MAX_DUTY` | 1.0 | Duty maximum envoyé aux moteurs |

---

## Procédure de tuning

### Étape 1 — Calibrer le feedforward

Le feedforward doit estimer le duty nécessaire pour chaque vitesse cible.

1. Mettre P, I, D à 0.
2. Mettre tous les filtres EMA à 1 (`VEL_EMA_ALPHA = 1`, `SPEED_EMA_ALPHA = 1`, `ANGLE_EMA_ALPHA = 1`) pour mesurer la vitesse brute sans lag.
3. Tester des duties fixes (0.1, 0.2, 0.3...) et noter la vitesse stable obtenue.
4. Calculer : `FF_GAIN_V = duty / vitesse_stable`
5. Vérifier que `FF_GAIN_W = FF_GAIN_V × WHEEL_BASE / 2` est correct pour w.

> Un bon feedforward réduit l'erreur statique à <10 % de la consigne avant même d'allumer le PID.

---

### Étape 2 — Régler le filtre EMA mesure (`VEL_EMA_ALPHA`)

Le filtrage VEL_EMA_ALPHA permet de réduire le bruit induit par l'encodeur. Si il saute brutalement d'une position a l'autre alors cela fait "grésiller" le moteur.

Conditions expérimentales: Il faut mettre une consigne fixe de vitesse au moteur, par exemple : CMD MOVE_VEL 1.0 0.0 ou CMD MOVE_VEL 0.0 2.0

Un filtre trop faible → grésillements moteur.  
Un filtre trop fort → réponse lente et lag de mesure.

| Symptôme | Action |
|---|---|
| Moteur grésille / vibre à vitesse stable | Diminuer `VEL_EMA_ALPHA` (ex: 0.1 → 0.05) |
| Réponse lente, le PID semble "endormi" | Augmenter `VEL_EMA_ALPHA` (ex: 0.1 → 0.3) |

Valeur de départ recommandée : **0.1**

---

### Étape 3 — Régler P (proportionnel)

Mettre I=0, D=0. Augmenter P progressivement.

Conditions expérimentales: Il faut faire une transition entre deux vitesse, par exemple 0 et 1m/s. Par exemple : CMD MOVE_VEL 0.0 0.0 et CMD MOVE_VEL 1.0 0.0

| Symptôme | Diagnostic | Action |
|---|---|---|
| Vitesse stable mais erreur résiduelle | P trop faible (normal sans I) | Augmenter P ou passer à l'étape 4 |
| Oscillations croissantes | P trop élevé | Réduire P de 20-30 % |
| Grésillements avec P élevé | Bruit amplifié | Réduire `VEL_EMA_ALPHA` en premier |

Valeurs de départ : **P_speed = 0.3**, **P_angle = 0.6**

---

### Étape 4 — Régler I (intégral)

L'intégrale élimine l'erreur statique résiduelle après le feedforward + P.

Conditions expérimentales: Il faut faire une transition entre deux vitesse, par exemple 0 et 1m/s. Par exemple : CMD MOVE_VEL 0.0 0.0 et CMD MOVE_VEL 1.0 0.0

1. Partir de I = 0.05–0.1.
2. Observer la convergence vers la consigne.

| Symptôme | Diagnostic | Action |
|---|---|---|
| Erreur statique persistante | I trop faible ou `PID_I_MAX` trop bas | Augmenter I ou `PID_I_MAX` |
| Dépassement (overshoot) fort, longue convergence | Windup — intégrale trop agressive | Réduire I ou réduire `PID_I_MAX` |
| Vitesse stable au-dessus de la consigne | `PID_I_MAX × I` dépasse le duty max utile | Réduire `PID_I_MAX` |

> Règle : `I × PID_I_MAX ≤ MAX_DUTY`. Au-delà, l'intégrale accumule sans effet utile (windup).

Valeur de départ recommandée : **I = 0.1**

---

### Étape 5 — Régler D (dérivé) — optionnel

Le D amortit les oscillations mais amplifie le bruit de mesure.  
Avec un bon feedforward et un filtre EMA, D est souvent inutile sur la vitesse linéaire.

Conditions expérimentales: Il faut faire une transition entre deux vitesse, par exemple 0 et 1m/s. Par exemple : CMD MOVE_VEL 0.0 0.0 et CMD MOVE_VEL 1.0 0.0

| Symptôme | Action |
|---|---|
| Oscillations persistantes malgré P réduit | Essayer D = 0.005–0.01 |
| Grésillements apparaissent avec D | Réduire D ou `VEL_EMA_ALPHA` |
| Réponse plus lente avec D | Normal — D est un frein, pas un moteur |

Valeurs de départ : **D_speed = 0.0**, **D_angle = 0.01**

---

### Étape 6 — Affiner les filtres EMA consigne (`SPEED_EMA_ALPHA` / `ANGLE_EMA_ALPHA`)

Une fois que les traisition entre 2 état est fait et que le moteur atteind sa consigne de manière stable, il faut passer au dynamique. Il faut envoyer des commandes en continu et voir si le moteur ne "saccade" pas.
Si c'est le cas alors il faut ralentir un peu la consigne avec ces deux filtres

Les commandes arrivent à ~10 Hz, la boucle tourne à 200 Hz. Sans lissage, chaque échelon de commande provoque un pic de FF+P qui secoue le robot. Ces alphas transforment l'échelon en rampe.

À faire une fois le PID stable, car l'impact des à-coups se juge mieux en boucle fermée.

| Symptôme | Action |
|---|---|
| À-coups moteur à chaque nouvelle commande | Diminuer l'alpha concerné |
| Robot trop lent à réagir / déphasage visible | Augmenter l'alpha concerné |
| w réagit trop vite par rapport à v | Diminuer `ANGLE_EMA_ALPHA` séparément |

> `ANGLE_EMA_ALPHA` est typiquement plus bas que `SPEED_EMA_ALPHA` car w est plus sensible aux instabilités.

Valeurs de départ : **`SPEED_EMA_ALPHA = 0.15`**, **`ANGLE_EMA_ALPHA = 0.05`**

---

## Commandes de tuning en live (sans recompiler)

```
CMD PID_SPEED P:0.3 I:0.1 D:0.0     ← vitesse linéaire
CMD PID_ANGLE P:0.6 I:0.1 D:0.01    ← vitesse angulaire
CMD PID P:0.3 I:0.1 D:0.0           ← les deux simultanément

CMD MOVE_VEL 0.5 0.0                 ← test vitesse linéaire (v=0.5 m/s)
CMD MOVE_VEL 0.0 3.0                 ← test vitesse angulaire (w=3.0 rad/s)
CMD MOVE_STOP
```

---

## Valeurs de référence (obtenues expérimentalement)

| Paramètre | Valeur |
|---|---|
| `FF_GAIN_V` | 0.58 duty/(m/s) |
| `VEL_EMA_ALPHA` (mesure) | 0.1 |
| `SPEED_EMA_ALPHA` (consigne v) | 0.15 |
| `ANGLE_EMA_ALPHA` (consigne w) | 0.05 |
| PID speed : P / I / D | 0.3 / 0.1 / 0.0 |
| PID angle : P / I / D | 0.6 / 0.1 / 0.01 |

---

## Checklist — Diagnostic rapide

- **Grésillements à vitesse stable** → réduire `VEL_EMA_ALPHA`
- **À-coups moteur à chaque commande** → réduire `SPEED_EMA_ALPHA` et/ou `ANGLE_EMA_ALPHA`
- **Erreur statique** → vérifier feedforward, puis augmenter I
- **Overshoot / dépassement** → réduire I ou `PID_I_MAX`
- **Oscillations** → réduire P, puis essayer D
- **Convergence trop lente** → augmenter I ou augmenter `VEL_EMA_ALPHA`
- **w trop élevé par rapport à la consigne** → vérifier `FF_GAIN_W` (doit être ≈ 10× plus petit que `FF_GAIN_V`)
- **Robot réagit trop lentement aux changements de w** → augmenter `ANGLE_EMA_ALPHA`
