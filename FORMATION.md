## FreeRTOS — Sections critiques

### `taskENTER_CRITICAL()` vs `taskENTER_CRITICAL_FROM_ISR()`

FreeRTOS distingue deux contextes d'exécution :
- **Tâche** : code normal dans une tâche FreeRTOS
- **ISR** : code dans un handler d'interruption

`taskENTER_CRITICAL()` désactive toutes les interruptions via `PRIMASK`. **Interdit dans une ISR.**

`taskENTER_CRITICAL_FROM_ISR()` sauvegarde et restaure le masque d'interruptions (`BASEPRI`), ce qui est obligatoire dans une ISR pour ne pas écraser l'état existant.

```c
// Dans une ISR
UBaseType_t isrm = taskENTER_CRITICAL_FROM_ISR();
// accès à la donnée partagée
taskEXIT_CRITICAL_FROM_ISR(isrm);
```

### Quand l'utiliser

Uniquement si ces 3 conditions sont vraies simultanément :
1. Tu es dans une **ISR**
2. Tu accèdes à une **donnée partagée** avec une tâche ou une autre ISR
3. La donnée n'est **pas atomique** (struct, buffer, variable 64-bit, compteur multi-étapes...)

### Bonnes pratiques

| Règle | Pourquoi |
|---|---|
| Section la plus courte possible | Chaque µs bloque toutes les autres interruptions |
| Jamais `taskENTER_CRITICAL()` dans une ISR | Bloque le CPU indéfiniment |
| Préférer les fonctions `FromISR` de FreeRTOS | `xQueueSendFromISR`, `xSemaphoreGiveFromISR`... déjà thread-safe |
| Toujours restaurer avec la valeur retournée | `taskEXIT_CRITICAL_FROM_ISR(isrm)` — pas une constante |
| Pas de logique complexe dans la section | Juste lire/écrire les données, rien d'autre |

> Si FreeRTOS propose une version `FromISR` de la fonction → utilise-la.
> Si tu manipules tes propres structures partagées → utilise `taskENTER/EXIT_CRITICAL_FROM_ISR`.


### `portYIELD_FROM_ISR`

Quand une ISR envoie des données dans une queue, FreeRTOS peut détecter qu'une tâche de priorité plus haute était en attente. `portYIELD_FROM_ISR` permet de basculer immédiatement sur cette tâche à la sortie de l'ISR, sans attendre le prochain tick scheduler.

```c
BaseType_t woken = pdFALSE;

for (uint32_t i = 0; i < len; i++)
    xQueueSendFromISR(_rxQueue, &buf[i], &woken);
    // woken est mis à pdTRUE si une tâche haute priorité a été débloquée

portYIELD_FROM_ISR(woken);  // si woken == pdTRUE → context switch immédiat
```

- `xQueueSendFromISR` met `woken = pdTRUE` si une tâche attendait sur cette queue avec une priorité plus haute que la tâche courante
- `portYIELD_FROM_ISR(woken)` déclenche un context switch à la fin de l'ISR uniquement si nécessaire

Sans cet appel, la tâche haute priorité attendrait jusqu'au prochain tick (jusqu'à 1 ms de délai inutile).
