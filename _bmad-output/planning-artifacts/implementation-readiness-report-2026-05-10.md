---
stepsCompleted:
  - step-01-document-discovery
  - step-02-prd-analysis
  - step-03-epic-coverage-validation
  - step-04-ux-alignment
  - step-05-epic-quality-review
  - step-06-final-assessment
filesIncluded:
  prd: _bmad-output/planning-artifacts/prd.md
  architecture: _bmad-output/planning-artifacts/architecture.md
  epics: _bmad-output/planning-artifacts/epics.md
  ux: null
---

# Implementation Readiness Assessment Report

**Date:** 2026-05-10
**Project:** bmad - robot CDR

## Document Inventory

| Type | Fichier | Statut |
|------|---------|--------|
| PRD | prd.md (15 213 o) | ✅ Trouvé |
| Architecture | architecture.md (22 443 o) | ✅ Trouvé |
| Épics & Stories | epics.md (24 835 o) | ✅ Trouvé |
| UX Design | — | ⚠️ Manquant |

---

## PRD Analysis

### Exigences Fonctionnelles (FR)

| ID | Titre | Description synthétique |
|----|-------|------------------------|
| FR-01 | Contrôle moteur boucle fermée | Tâche OdoControl 200Hz : lecture encodeurs → odométrie → PID → moteurs. Seul consommateur IEncoderHAL/IMotorHAL. Budget < 4ms/tick. |
| FR-02 | Consigne mouvement via mailbox | MotionPlanner écrit via xQueueOverwrite (depth-1). OdoControl lit via xQueuePeek sans blocage. xQueueReset() = arrêt urgence O(1). |
| FR-03 | Bus pub/sub IBus multi-topic | ExternalComm implémente IBus. 4 topics : TELEMETRY, ALERT, LOG, HEALTH. Politique overwrite/drop par topic dans BUS_CONFIG[]. Injection constructeur. |
| FR-04 | Communication externe tri-canal ASCII | UART (priorité terrain) + USB + Ethernet. Format `TOPIC PAYLOAD\n`. UART écrase en cas de simultanéité. Parsing sscanf sur premier token. |
| FR-05 | Gestion capteurs extensible | SensorManager orchestre jusqu'à MAX_SENSORS=15 ISensor*. Ajout = nouvelle classe concrète uniquement. SensorManager agnostique aux types concrets. |
| FR-06 | Gestion actionneurs extensible | ActuatorManager implémente IActuatorManager, jusqu'à MAX_ACTUATORS=10 IActuator*. Commande via commandById(id, cmd). Découplé d'ExternalComm. |
| FR-07 | Alarmes capteurs faible latence | SensorManager → xTaskNotify(motionPlannerHandle, bitmask, eSetBits). MotionPlanner se réveille sans polling. Latence ≤ 1 tick FreeRTOS. |
| FR-08 | Formatage IBus centralisé | Toutes publications via BusFormat:: uniquement. snprintf inline interdit dans classes métier. |
| FR-09 | Configuration compile-time centralisée | Toutes constantes (fréquences, stacks, priorités, PID) en constexpr dans App/Config.h. Zéro magic number dans classes. |
| FR-10 | Câblage statique dans SystemInit | SystemInit::boot() instancie tout en statique et injecte toutes dépendances. Seul fichier contenant new (interdit partout ailleurs). |

**Total FRs : 10**

### Exigences Non-Fonctionnelles (NFR)

| ID | Titre | Description synthétique |
|----|-------|------------------------|
| NFR-01 | Déterminisme temps-réel | OdoControl à 200Hz ± 50µs. Publications IBus non-bloquantes depuis OdoControl (overwrite). |
| NFR-02 | Empreinte mémoire prévisible | RAM connue à la compilation. Zéro allocation dynamique. Tableaux tailles fixes. Stacks dans Config.h validées par linker. |
| NFR-03 | Testabilité sur host | 100% des classes App/ instanciables sur PC x86 avec mocks injectés. Tests Google Test sans toolchain ARM ni hardware. |
| NFR-04 | Sécurité inter-tâches par construction | Aucun accès direct mémoire inter-tâches. Seuls canaux autorisés : IBus::publish(), xTaskNotify(), xQueueOverwrite(), xQueuePeek(). |
| NFR-05 | Extensibilité sans régression | Ajout capteur/actionneur/module = zéro modification classe existante. Seul SystemInit modifié pour câbler un nouveau composant. |
| NFR-06 | Conformité conventions de code | Membres privés préfixés _. Constantes ALL_CAPS. Interfaces préfixées I. Fichiers PascalCase. Guards #ifndef APP_<DOSSIER>_<FICHIER>_H. |
| NFR-07 | Périmètre App/ et Tests/ uniquement | Code manuel exclusivement dans App/ et Tests/. Core/, Drivers/, Middlewares/ = CubeMX uniquement, jamais modifiés manuellement. |
| NFR-08 | Convention anglais exclusif | Tous identifiants en anglais (classes, méthodes, variables, fichiers, topics IBus, messages ASCII). |

**Total NFRs : 8**

### Exigences additionnelles / Contraintes

- **Plateforme :** STM32 + FreeRTOS CMSIS V2. HAL CubeMX uniquement via `App/`. C++17, `-fno-exceptions`, `-fno-rtti`, `-Wall`, `-Wextra`.
- **Point d'entrée :** main.cpp = 3 lignes strictement (SystemInit::boot() + vTaskStartScheduler() + boucle vide).
- **Autorité architecturale :** En cas de contradiction entre PRD et architecture.md, architecture.md fait foi.
- **Limites capacitives :** MAX_SENSORS=15, MAX_ACTUATORS=10 (compile-time).
- **Périphériques CubeMX :** TIM (encodeurs ×2), PWM (moteurs ×2), UART, USB CDC, Ethernet.

### Évaluation de complétude PRD

Le PRD est **bien structuré et complet** pour un projet embedded/IoT brownfield. Points forts :
- Critères de succès mesurables et vérifiables
- User journeys concrets avec préconditions et résultats attendus
- Critères de test spécifiés pour chaque FR
- Séparation claire MVP / Growth / Vision

Point d'attention : absence d'un document UX (non applicable ici — projet sans interface graphique humaine, uniquement protocole ASCII). Cela ne constitue pas une lacune pour ce type de projet.

---

## Epic Coverage Validation

### Matrice de couverture FR

| FR | Exigence (résumé) | Couverture Epic | Story(ies) | Statut |
|----|-------------------|-----------------|------------|--------|
| FR-01 | OdoControl 200Hz PID boucle fermée | Epic 2 | Story 2.1 | ✅ Couvert |
| FR-02 | Mailbox consigne xQueueOverwrite/Peek/Reset | Epic 2 | Story 2.2 | ✅ Couvert |
| FR-03 | IBus multi-topic + injection constructeur | Epic 1 | Story 1.2, 1.4 | ✅ Couvert |
| FR-04 | Communication tri-canal UART/USB/ETH ASCII | Epic 1 | Story 1.4 | ✅ Couvert |
| FR-05 | SensorManager ISensor[15] extensible | Epic 3 | Story 3.1 | ✅ Couvert |
| FR-06 | ActuatorManager IActuator[10] commandById | Epic 3 | Story 3.2 | ✅ Couvert |
| FR-07 | Alarmes xTaskNotify bitmask ≤1 tick | Epic 3 | Story 3.1 | ✅ Couvert |
| FR-08 | BusFormat centralisé, snprintf interdit | Epic 1 | Story 1.3 | ✅ Couvert |
| FR-09 | Config.h constexpr, zéro magic number | Epic 1 | Story 1.2 | ✅ Couvert |
| FR-10 | SystemInit câblage statique, zéro new | Epic 1 | Story 1.5 | ✅ Couvert |

### Couverture NFR dans les critères d'acceptation

| NFR | Titre | Couverture dans stories | Statut |
|-----|-------|------------------------|--------|
| NFR-01 | Déterminisme temps-réel | Story 2.1 : budget < 4ms/cycle mesuré via timer HAL | ✅ Adressé |
| NFR-02 | Empreinte mémoire prévisible | Story 1.5 : grep `new/delete`, Story 3.4 : zéro allocation dynamique drivers | ✅ Adressé |
| NFR-03 | Testabilité sur host | Toutes les stories Epic 1-3 : tests PC host sans toolchain ARM | ✅ Adressé |
| NFR-04 | Sécurité inter-tâches | Mentionné dans AC Story 2.2 (MotionPlanner), pas de story dédiée à la validation exhaustive | ⚠️ Partiel |
| NFR-05 | Extensibilité sans régression | Story 3.4 : démontre le pattern avec drivers concrets, Story 3.1 (SensorManager agnostique) | ✅ Adressé |
| NFR-06 | Conventions de code | Story 1.2 : guards uniquement. Pas d'AC systématique sur préfixes `_`, `I`, PascalCase | ⚠️ Partiel |
| NFR-07 | Périmètre App/ et Tests/ | Story 1.5 : grep `new/delete App/`. Story 1.1 mentionne la non-modification Core/ | ✅ Adressé |
| NFR-08 | Anglais exclusif identifiants | **Aucun AC ne vérifie cette règle** dans aucune story | ❌ Non adressé |

### Exigences manquantes / Lacunes de couverture

#### ❌ NFR-08 — Anglais exclusif : aucune story ne le valide
- **Impact :** Un agent IA ou développeur peut introduire des identifiants français sans violation détectable.
- **Recommandation :** Ajouter un AC dans Story 1.2 ou Story 1.6 : *"Given `App/` est scanné, When `grep -rn '[a-záéèêîïôùûüçœ]' App/` est exécuté sur les identifiants, Then zéro occurrence dans les noms de classes/méthodes/variables."*

#### ⚠️ NFR-04 — Sécurité inter-tâches : pas de validation exhaustive
- **Impact :** Des accès mémoire inter-tâches illicites pourraient passer en review sans filet de sécurité outillé.
- **Recommandation :** Ajouter un AC dans Story 1.6 ou Story 2.2 : *"Given `App/Tasks/` est scanné, When `grep -r 'extern\|global\|shared'` est exécuté, Then zéro accès mémoire partagé direct."*

#### ⚠️ NFR-06 — Conventions de code : vérification incomplète
- **Impact :** Seuls les guards sont vérifiés dans les AC. Les préfixes `_`, `I`, PascalCase, `ALL_CAPS` ne sont jamais vérifiés programmatiquement.
- **Recommandation :** Étendre les AC de Story 1.2 avec des grep couvrant les conventions de nommage, ou créer une Story 1.7 dédiée à un script de lint des conventions.

### Statistiques de couverture

- **Total FRs PRD :** 10
- **FRs couverts dans epics :** 10
- **Taux de couverture FR :** **100%** ✅
- **Total NFRs PRD :** 8
- **NFRs pleinement adressés :** 6
- **NFRs partiellement adressés :** 2 (NFR-04, NFR-06)
- **NFRs non adressés :** 1 (NFR-08)
- **Taux de couverture NFR :** **75%** ⚠️

---

## UX Alignment Assessment

### Statut du document UX

**Non trouvé** — aucun fichier `*ux*.md` dans `_bmad-output/planning-artifacts/`.

### Évaluation de la nécessité d'une UX

| Critère | Résultat |
|---------|----------|
| Le PRD mentionne-t-il une interface utilisateur graphique ? | Non |
| Y a-t-il des composants web/mobile implicites ? | Non |
| L'application est-elle destinée à des utilisateurs finaux non-techniques ? | Non |
| L'epics.md confirme l'absence de UX ? | Oui — *"Aucun document UX — projet embarqué sans interface graphique."* |

### Conclusion

✅ **L'absence de document UX est justifiée.** Ce projet est un firmware embarqué STM32 dont l'unique "interface" est un protocole ASCII sur UART/USB/Ethernet, destiné à des développeurs et outils de supervision. Aucune interface graphique n'est requise.

### Avertissements

Aucun avertissement UX — l'absence est délibérée et documentée.

---

## Epic Quality Review

### Valeur utilisateur et indépendance des Epics

| Epic | Titre | Objectif user-centric | Indépendance | Verdict |
|------|-------|----------------------|--------------|---------|
| Epic 1 | Squelette Système | ✅ Développeur peut démarrer FreeRTOS + valider archi communication | ✅ Autonome | ✅ OK (titre technique, objectif user-centric acceptable pour projet firmware) |
| Epic 2 | Contrôle Locomotion | ✅ Développeur peut piloter robot boucle fermée via ASCII | ✅ Utilise uniquement Epic 1 | ✅ OK |
| Epic 3 | Écosystème Capteurs-Actionneurs | ✅ Développeur peut intégrer 15 capteurs + 10 actionneurs | ✅ Utilise Epic 1 + 2 | ✅ OK |

### Analyse des dépendances de stories

**Epic 1 — Chaîne de dépendance :**
- Story 1.1 → autonome ✅
- Story 1.2 → dépend de 1.1 (projet CubeMX) ✅ backward
- Story 1.3 → dépend de 1.2 (IBus interface) ✅ backward
- Story 1.4 → dépend de 1.2 + 1.3 (IBus + BusFormat) ✅ backward
- Story 1.5 → dépend de 1.1-1.4 (câble tout) ✅ backward
- Story 1.6 → dépend de 1.2-1.5 ✅ backward ⚠️ Mentionne "prêts pour Epic 2 et 3" (forward reference informative — non bloquante)

**Epic 2 — Chaîne de dépendance :**
- Story 2.1 → dépend de Epic 1 (IBus, IEncoderHAL, IMotorHAL) ✅
- Story 2.2 → dépend de 2.1 (mailbox OdoControl) ✅ backward

**Epic 3 — Chaîne de dépendance :**
- Story 3.1 → dépend de Epic 1 + 2 (nécessite `motionPlannerHandle` d'Epic 2) ✅
- Story 3.2 → dépend de Epic 1 ✅
- Story 3.3 → dépend de Epic 1 (IBus) — ⚠️ voir violation critique ci-dessous
- Story 3.4 → dépend de Epic 1 (ISensor, IActuator) ✅
- Story 3.5 → dépend de 3.1-3.4 ✅ backward

### Violations identifiées

---

#### 🔴 VIOLATION CRITIQUE — Story 3.3 : mécanisme `subscribe` IBus non défini

**Story :** 3.3 — Monitoring
**AC problématique :** *"Given `Monitoring` souscrit à `Topic::ALERT` via `IBus`, When un module publie une alerte, Then `Monitoring` reçoit et agrège le message sans polling actif"*

**Problème :** L'interface `IBus` telle que définie par FR-03 et le PRD ne possède qu'une méthode `publish(Topic, const char*)`. **Aucun mécanisme de subscription n'est défini.** ExternalComm EST IBus (implémentation unique) et route tout vers UART/USB/ETH. Comment Monitoring peut-il "s'abonner" à des topics via cette interface ?

**Impact :** Story 3.3 est **non implémentable en l'état** sans modification de l'interface IBus ou introduction d'un mécanisme observer/callback. Cela constitue un trou architectural non résolu qui bloquera l'implémentation.

**Recommandations (deux options) :**
1. **Option A — Étendre IBus :** Ajouter `registerSubscriber(Topic, ISubscriber*)` à IBus. Mais cela complexifie l'interface et risque de casser NFR-01 (non-bloquant). À valider dans architecture.md.
2. **Option B — Réviser Monitoring :** Redéfinir Monitoring comme consommateur direct d'une file de monitoring dédiée (style mailbox) alimentée par SensorManager, pas par subscription IBus. Modifier Story 3.3 en conséquence.

---

#### 🟠 PROBLÈME MAJEUR — Story 3.5 : story de tests pure sans valeur utilisateur livrée

**Story :** 3.5 — Tests unitaires SensorManager, ActuatorManager, Monitoring et drivers
**Problème :** Cette story regroupe uniquement des tests sans fonctionnalité nouvelle livrée. Elle viole le pattern cohérent établi dans Epics 1 et 2 où chaque story intègre ses propres critères de test. De plus, elle crée une dépendance sur 3.1+3.2+3.3+3.4 — elle ne peut pas commencer tant que toutes ces stories ne sont pas terminées.

**Impact :** Si un développeur complète 3.1 mais pas 3.3, les tests de 3.5 ne peuvent pas tourner partiellement. Cela réduit la détection rapide de régressions.

**Recommandation :** Distribuer les AC de test de Story 3.5 dans les stories correspondantes (3.1, 3.2, 3.3, 3.4) comme c'est fait dans Epics 1 et 2. Supprimer Story 3.5 comme story indépendante ou la convertir en story d'intégration système.

---

#### 🟡 PRÉOCCUPATION MINEURE — Story 3.4 : trop large (6 drivers en une story)

**Story :** 3.4 — Drivers concrets
**Problème :** Implémentation de 6 classes concrètes (ProximitySensor, TemperatureSensor, CurrentSensor, Pump, Servo, LinearTransducer) dans une seule story. Charge de travail potentiellement trop importante pour une story "indépendamment complétable".

**Impact :** Risque de story incomplète si le temps est limité — difficile à livrer partiellement.

**Recommandation :** Scinder en deux stories : 3.4a (drivers ISensor : 3 capteurs) et 3.4b (drivers IActuator : 3 actionneurs). Chacune peut être livrée indépendamment.

---

#### 🟡 PRÉOCCUPATION MINEURE — Titres d'Epics techniques

**Epics :** "Squelette Système", "Écosystème Capteurs-Actionneurs"
**Problème :** Titres orientés technique plutôt qu'outcome utilisateur.
**Impact :** Faible — les objectifs user-centric sont clairement dans les descriptions.
**Recommandation :** Reformuler optionnellement : "Squelette Système" → "Architecture de Communication Opérationnelle", "Écosystème Capteurs-Actionneurs" → "Supervision Temps-Réel Capteurs et Actionneurs".

### Checklist de conformité aux bonnes pratiques

| Critère | Epic 1 | Epic 2 | Epic 3 |
|---------|--------|--------|--------|
| Epic livre de la valeur utilisateur | ✅ | ✅ | ✅ |
| Epic fonctionne indépendamment | ✅ | ✅ | ✅ |
| Stories correctement dimensionnées | ✅ | ✅ | ⚠️ (3.4 trop large) |
| Pas de dépendances forward | ✅ | ✅ | ⚠️ (3.3 suppose subscribe) |
| Critères d'acceptation BDD Given/When/Then | ✅ | ✅ | ✅ |
| Critères testables et mesurables | ✅ | ✅ | ⚠️ (3.3 non implémentable) |
| Traçabilité FR maintenue | ✅ | ✅ | ✅ |

---

## Synthèse et Recommandations

### Statut global de préparation à l'implémentation

> ## ⚠️ NÉCESSITE DES CORRECTIONS
>
> Epics 1 et 2 sont **prêts à implémenter immédiatement**. Epic 3 contient une violation critique bloquante qui doit être résolue avant implémentation de Story 3.3. Les autres problèmes sont correctifs mais non bloquants.

---

### Tableau de bord des problèmes

| Sévérité | Problème | Impacte | Bloquant |
|----------|----------|---------|----------|
| 🔴 Critique | Story 3.3 : mécanisme IBus `subscribe` non défini | Epic 3 Story 3.3 | Oui — Story 3.3 non implémentable |
| 🟠 Majeur | Story 3.5 : story de tests pure (pattern incohérent) | Epic 3 | Non |
| 🟠 Majeur | NFR-08 : aucune AC ne valide l'anglais exclusif des identifiants | Toutes stories | Non |
| 🟡 Mineur | NFR-04 : validation inter-tâches incomplète dans les AC | Stories 1.6, 2.2 | Non |
| 🟡 Mineur | NFR-06 : conventions de code partiellement vérifiées (guards seulement) | Story 1.2 | Non |
| 🟡 Mineur | Story 3.4 : trop large — 6 drivers en une story | Epic 3 | Non |

---

### Actions requises avant implémentation de Story 3.3

**Action 1 — Résoudre le mécanisme de subscription Monitoring ↔ IBus (BLOQUANT)**

Choisir une option et mettre à jour epics.md et architecture.md :

- **Option A :** Étendre `IBus` avec `registerSubscriber(Topic, ISubscriber*)`. Implique une modification de l'interface existante — valider compatibilité NFR-01 (non-bloquant).
- **Option B :** Monitoring reçoit les données via une mailbox dédiée alimentée directement par SensorManager (pas via IBus). Modifier Story 3.3 pour refléter ce design.
- **Option C :** Monitoring est une tâche qui lit directement un buffer partagé protégé — à valider contre NFR-04.

---

### Actions recommandées (non bloquantes)

**Action 2 — Ajouter AC NFR-08 dans Story 1.2 ou Story 1.6**

```
Given App/ est scanné pour les identifiants non-anglais,
When grep est lancé sur les noms de classes, méthodes et membres,
Then zéro occurrence d'identifiant contenant des caractères accentués ou mots français.
```

**Action 3 — Distribuer les tests de Story 3.5 dans les stories 3.1-3.4**

Ajouter à chaque story son AC de test complet, puis soit supprimer Story 3.5, soit la renommer en "Story 3.5 : Validation d'intégration système Epic 3" avec un périmètre clair.

**Action 4 — Scinder Story 3.4 en deux stories**
- Story 3.4a : Drivers ISensor concrets (ProximitySensor, TemperatureSensor, CurrentSensor) + tests
- Story 3.4b : Drivers IActuator concrets (Pump, Servo, LinearTransducer) + tests

**Action 5 — Compléter les AC NFR-04 et NFR-06**

Dans Story 1.6 : ajouter grep sur accès mémoire inter-tâches et sur les conventions de nommage (préfixes `_`, `I`, PascalCase, `ALL_CAPS`).

---

### Séquence d'implémentation recommandée

```
Phase immédiate (aucune correction requise) :
  → Epic 1 complet (Stories 1.1 → 1.6) ✅
  → Epic 2 complet (Stories 2.1 → 2.2) ✅

Avant Epic 3 Story 3.3 :
  → Résoudre Action 1 (mécanisme Monitoring ↔ IBus)
  → Mettre à jour architecture.md si IBus étendu

Epic 3 ordre suggéré :
  → 3.1 (SensorManager) → 3.2 (ActuatorManager) → [résoudre Action 1] → 3.3 (Monitoring) → 3.4a → 3.4b
```

---

### Bilan final

Ce rapport a identifié **6 problèmes** répartis en 3 catégories.

- **FR Coverage : 100%** — toutes les exigences fonctionnelles sont tracées.
- **NFR Coverage : 75%** — 2 partielles, 1 absente.
- **Epic Quality : Epics 1 et 2 excellents** — Epic 3 contient 1 violation critique et 2 problèmes majeurs.
- **UX : N/A** — justifié pour un projet firmware embarqué.

**L'équipe peut commencer l'implémentation d'Epics 1 et 2 immédiatement.** Epic 3 nécessite la résolution de Story 3.3 (mécanisme de subscription Monitoring) avant d'aller plus loin.

---

*Rapport généré le 2026-05-10 par l'évaluation de préparation à l'implémentation BMAD.*
