# Deferred Work

## Deferred from: code review de 1-1-projet-stm32cubemx-configure-et-freertos-operationnel-sur-cible (2026-05-10)

- **`latestSnapshot` sans synchronisation** — data race latent dès que les tâches Stories 1.4, 2.1, 3.1 écriront sur `OdoControl::latestSnapshot`, `SensorManager::latestSnapshot`, `ExternalComm::latestSnapshot`. Ajouter mutex ou section critique à implémenter lors de ces stories.

- **`lastCmd[32]` sans invariant de null-termination** — `CommSnapshot::lastCmd` n'a pas d'invariant documenté garantissant `lastCmd[31] == '\0'`. À documenter/enforcer lors de l'implémentation de Story 1.4 (ExternalComm).

- **Définitions static members dans `SystemInit.cpp` — risque SIOF futur** — `OdoControl::latestSnapshot`, `SensorManager::latestSnapshot`, `ExternalComm::latestSnapshot` sont définis dans `SystemInit.cpp` au lieu de leur `.cpp` propriétaire. Si un constructeur non-trivial est ajouté, le Static Initialization Order Fiasco peut survenir. À déplacer vers les `.cpp` respectifs lors de Stories 2.1, 1.4, 3.1.

- **`MX_LWIP_Init()` / `MX_USB_DEVICE_Init()` sans vérification d'erreur** — Code CubeMX dans `Core/Src/main.c`, hors scope modifiable selon NFR-07. Risque de démarrage silencieux si ces inits échouent. À documenter comme limitation de l'architecture CubeMX.

## Deferred from: code review App/ + Tests/ (2026-05-14)

- ~~**[D1][HIGH] `Drv8262` : incohérence TIM_CHANNEL_2/PE11 (commentaire) vs TIM_CHANNEL_3/CCR3 (code)**~~ — **RÉSOLU (2026-05-14) : faux positif.** Le `.ioc` confirme : PE11 = `WHEEL_DIR1` (GPIO direction, pas PWM) ; PE13 = `WHEEL_PWM2` → `TIM1_CH3`. Le code `TIM_CHANNEL_3` + `CCR3` est correct.

- **[D2][HIGH] `SetpointMode::VELOCITY` non géré dans `OdoControl::tick()`** — `OdoControl.cpp:tick`. Si un setpoint VELOCITY est envoyé, `sp.pose` est accédé via la mauvaise branche de l'union → comportement indéfini. Actuellement aucun émetteur ne crée de setpoints VELOCITY, mais l'enum existe et invite à de futures extensions. À implémenter ou à supprimer lors d'une évolution de l'architecture locomotion.

- **[D5][MEDIUM] `Encoder` dépend de `ExternalComm` (inversion de couche HAL→App)** — `Drivers/Encoder/Encoder.cpp:10` inclut `Tasks/ExternalComm.h`. Un driver HAL ne devrait pas dépendre d'une tâche applicative. À corriger en injectant une interface `ILogger` légère dans le constructeur de `Encoder`.

- **[D6][MEDIUM] `Drv8262::init()` utilise `HAL_Delay()` bloquant (100 ms + 10 ms)** — `Drv8262.cpp:init`. Si `init()` est appelé après le démarrage du scheduler FreeRTOS, le busy-wait bloque toutes les tâches de priorité inférieure pendant 110 ms. Latent car le vrai driver n'est pas encore utilisé. À remplacer par `vTaskDelay()` lors de l'intégration matérielle.

- **[D7][MEDIUM] `rxCount` et `lastCmd` mis à jour même pour les commandes CMD inconnues** — `ExternalComm.cpp:_processRxLine`. Un `CMD FOOBAR` valide syntaxiquement (token "CMD" reconnu, verb inconnu) incrémente `rxCount` et écrase `lastCmd`. Comportement potentiellement trompeur pour les diagnostics. À décider selon l'intention de design (comptabiliser ou non les commandes inconnues).

- **[D8][LOW] `BusFormat` retourne des pointeurs vers des `static char buf[]` non réentrants** — `BusFormat.cpp`. Chaque méthode possède son propre buffer statique, donc deux méthodes différentes sont indépendantes. Mais deux appels concurrents à la *même* méthode partagent le buffer. Actuellement sûr (chaque méthode est appelée depuis une seule tâche), mais fragile si l'architecture évolue. À documenter ou migrer vers des buffers fournis par l'appelant.
