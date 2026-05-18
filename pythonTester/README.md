# Wall-A Serial Monitor

Interface graphique Python pour surveiller et contrôler le robot Wall-A via liaison série.

## Lancement

```powershell
py -3.11 main.py
```

## Structure des fichiers

```
pythonTester/
├── main.py           # Point d'entrée
├── config.py         # Constantes (port, baudrate, taille des buffers…)
├── parser.py         # Parsing des trames série reçues
├── data_store.py     # Stockage des données en mémoire circulaire
├── serial_worker.py  # Thread de lecture/écriture série
└── app.py            # Interface graphique (fenêtre principale)
```

## Format des trames reçues

Le firmware envoie des lignes texte de la forme :

```
DOMAIN SUBDOMAIN key1:val1 key2:val2 time:123456
```

Exemple : `ODOM POSE x:0.12 y:-0.04 angle:1.57 time:4823`

`parse_frame()` extrait le domaine, sous-domaine, et un dictionnaire de variables flottantes.
Le champ `time` (ms embarqué) sert à synchroniser l'axe temporel des graphes.

## Onglet Surveillance

- **Tableau** : dernière valeur de chaque variable reçue + variation sur la fenêtre glissante
- **Graphe** : historique glissant de 3 secondes (configurable via `PLOT_WINDOW` dans `config.py`)
- **Console** : lignes brutes non parsées, colorées par niveau (WARN, ALT/HLT, info)

## Onglet Commandes

Envoie des commandes au firmware via le même port série. Format attendu par `ExternalComm::_processRxLine()` :

| Section | Commande envoyée | Description |
|---------|-----------------|-------------|
| Vitesse | `CMD MOVE_VEL v w` | Consigne vitesse linéaire (m/s) et angulaire (rad/s) |
| Pose cible | `CMD MOVE_POSE x y angle` | Consigne de position absolue |
| Arrêt | `CMD MOVE_STOP` | Arrêt immédiat |
| PID | `CMD PID P:val I:val D:val` | Réglage des gains de l'asservissement |

Le journal des envois affiche chaque commande en vert (envoyée) ou rouge (port non connecté).

## Configuration

Éditer `config.py` pour changer :

| Constante | Défaut | Rôle |
|-----------|--------|------|
| `DEFAULT_COM_PORT` | `COM31` | Port série sélectionné au démarrage |
| `PLOT_WINDOW` | `3` | Durée visible sur le graphe (secondes) |
| `MAX_POINTS` | `500` | Taille des buffers circulaires par variable |
| `RECONNECT_DELAY` | `1.0` | Délai de reconnexion automatique (secondes) |

## Architecture interne

```
Port série
    │
    ▼
SerialWorker (thread)        ◄── write(cmd) ── Onglet Commandes
    │ queue.Queue
    ▼
_poll() — toutes les 100 ms
    │
    ├── parse_frame()
    │
    ├── vars trouvées ──► DataStore ──► Tableau + Graphe
    │
    └── ligne brute   ──► Console
```

`SerialWorker` lit en continu dans son thread et dépose les lignes dans une `queue.Queue`.
L'écriture (`write`) est protégée par un verrou pour rester thread-safe.
`_poll()` draine la queue depuis le thread principal (Tkinter) toutes les 100 ms.
