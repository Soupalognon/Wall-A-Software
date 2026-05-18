# Guide de tuning — Asservissement vitesse Wall-A

## Architecture de contrôle

```
Consigne (v, w)
     │
     ├──► Feedforward ──────────────────────────────────┐
     │    (duty ≈ FF_GAIN × consigne)                   │
     │                                                   ▼
     └──► Erreur = consigne − mesure → PID ──► duty total → Moteur → Encodeur
                                                                         │
                                          Filtre EMA ◄───────────────────┘
                                          (lisse le bruit de quantification)
```

Le **feedforward** fournit ~95 % du duty nécessaire.  
Le **PID** corrige uniquement le résidu d'erreur.  
Le **filtre EMA** élimine le bruit d'encodeur qui provoquerait des grésillements.

---

## Paramètres clés — `Config.h`

| Paramètre | Valeur actuelle | Rôle |
|---|---|---|
| `VEL_EMA_ALPHA` | 0.1 | Lissage vitesse encodeur (0=max lisse, 1=brut) |
| `FF_GAIN_V` | 0.58 | Duty par m/s — calibré expérimentalement |
| `FF_GAIN_W` | FF_GAIN_V × WHEEL_BASE / 2 | Duty par rad/s — dérivé automatiquement |
| `PID_I_MAX_SPEED` | 1.0 | Plafond de l'intégrale vitesse linéaire |
| `PID_I_MAX_ANGLE` | 0.5 | Plafond de l'intégrale vitesse angulaire |
| `MAX_DUTY` | 1.0 | Duty maximum envoyé aux moteurs |

---

## Procédure de tuning

### Étape 1 — Calibrer le feedforward

Le feedforward doit estimer le duty nécessaire pour chaque vitesse cible.

1. Mettre P, I, D à 0.
2. Tester des duties fixes (0.1, 0.2, 0.3...) et noter la vitesse stable obtenue.
3. Calculer : `FF_GAIN_V = duty / vitesse_stable`
4. Vérifier que `FF_GAIN_W = FF_GAIN_V × WHEEL_BASE / 2` est correct pour w.

> Un bon feedforward réduit l'erreur statique à <10 % de la consigne avant même d'allumer le PID.

---

### Étape 2 — Régler le filtre EMA (`VEL_EMA_ALPHA`)

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

| Symptôme | Diagnostic | Action |
|---|---|---|
| Vitesse stable mais erreur résiduelle | P trop faible (normal sans I) | Augmenter P ou passer à l'étape 4 |
| Oscillations croissantes | P trop élevé | Réduire P de 20-30 % |
| Grésillements avec P élevé | Bruit amplifié | Réduire `VEL_EMA_ALPHA` en premier |

Valeur de départ recommandée : **P = 0.3**

---

### Étape 4 — Régler I (intégral)

L'intégrale élimine l'erreur statique résiduelle après le feedforward + P.

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
Avec un bon feedforward et un filtre EMA, D est souvent inutile.

| Symptôme | Action |
|---|---|
| Oscillations persistantes malgré P réduit | Essayer D = 0.005–0.01 |
| Grésillements apparaissent avec D | Réduire D ou `VEL_EMA_ALPHA` |
| Réponse plus lente avec D | Normal — D est un frein, pas un moteur |

Valeur de départ recommandée : **D = 0.0** (ajouter seulement si nécessaire)

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
| `VEL_EMA_ALPHA` | 0.1 |
| PID speed : P / I / D | 0.3 / 0.1 / 0.0 |
| PID angle : P / I / D | 0.6 / 0.1 / 0.01 |

---

## Checklist — Diagnostic rapide

- **Grésillements à vitesse stable** → réduire `VEL_EMA_ALPHA`
- **Erreur statique** → vérifier feedforward, puis augmenter I
- **Overshoot / dépassement** → réduire I ou `PID_I_MAX`
- **Oscillations** → réduire P, puis essayer D
- **Convergence trop lente** → augmenter I ou augmenter `VEL_EMA_ALPHA`
- **w trop élevé par rapport à la consigne** → vérifier `FF_GAIN_W` (doit être ≈ 10× plus petit que `FF_GAIN_V`)
