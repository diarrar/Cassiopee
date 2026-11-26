# Note de conception : précision réduite et mixte dans Cassiopee, pour Fast

*Version 1, 26/11/2026. Document de travail à discuter avec les développeurs de Fast et de Cassiopee.*

## 1. Objectif

Le projet vise à faire tourner **Fast en précision réduite**. La trajectoire prévue est la suivante :

1. fp32 complet ;
2. précision mixte : variables conservatives en double, tableaux de travail en précision réduite ;
3. éventuellement fp16 ou bf16 pour certains tableaux intermédiaires ;
4. éventuellement une précision qui varie dans l'espace (par zone) ;
5. en perspective, le portage de Fast sur GPU.

Les motivations sont d'explorer le sujet et de monter en compétence, de gagner en performance (Fast est limité par la bande passante mémoire), et de préparer le GPU. Le stage DAAA-2027-16 (février à septembre 2027) porte sur la précision mixte contrôlée dans Fast.

Fast et Cassiopee fonctionnent ensemble. Si Cassiopee ne suit pas, Fast en précision réduite restera une version de laboratoire, sans perspective de mise en production. **Il faut donc une seule version de Cassiopee, maintenue et validée, qui supporte cette trajectoire.**

## 2. Constat : une précision globale ne suffit pas

Le premier travail (branche `claude/optimistic-maxwell-xwotl2`) fait d'`E_Float` un type réel **global**, choisi à la compilation (`ELSAPROD=..._r4`). Cette approche :
- couvre l'étape 1 (fp32 complet), mais **ne peut exprimer ni l'étape 2 (deux précisions dans le même calcul), ni le fp16** (ni `E_Float` ni la norme CGNS ne le connaissent), **ni l'étape 4** ;
- impose un **second produit** à maintenir et à valider : un second jeu de références, environ 1100 créations numpy sans dtype à auditer, Nuga réécrit pour calculer en double dans un build float (122 fichiers), et 201 tests r4 encore en échec ;
- contient des choix qui **gêneraient** la précision mixte :
  - `K_PYTREE::getValueAF` convertit sur place, vers la précision compilée, un nœud d'un autre type ;
  - `K_NUMPY::getFromNumpyArray` refuse tout autre type ;
  - la lecture des fichiers ramène tout à la précision compilée.

  Fast passe environ 400 fois par ces deux accesseurs (210 appels à `getValueAF`, 203 à `getFromNumpyArray`).

## 3. Principe retenu : la précision par tableau

> **Un seul build de Cassiopee, `E_Float = double`. Chaque tableau de l'arbre porte son propre type réel (float64, float32, et float16 pour le stockage). Cassiopee respecte ce type, ne le change jamais en silence, et calcule en double là où la robustesse l'exige.**

Ce choix laisse le comportement de Cassiopee inchangé pour tous les utilisateurs actuels : arbres en float64, mêmes résultats, mêmes références. La précision devient une propriété des données, choisie par l'appelant (Fast), et non une option de compilation.

## 4. Règles côté Cassiopee

| # | Règle | Ce que ça implique |
|---|---|---|
| R1 | **Aucune conversion silencieuse dans les accesseurs C/C++.** | Les accesseurs vérifient le dtype et lèvent une erreur Python claire en cas de désaccord. Une conversion, quand il en faut une, est explicite (Python ou fonction dédiée). |
| R2 | **Des accesseurs typés.** | `template<typename T> T* K_PYTREE::getValueA(node, hook)` et `template<typename T> E_Int K_NUMPY::getFromNumpyArray(o, T*&, ...)`, avec un trait `NpyType<T>` (`double`→`NPY_FLOAT64`, `float`→`NPY_FLOAT32`, stockage 16 bits→`NPY_FLOAT16`). Une fonction `getRealType(node)` et une macro de dispatch servent aux noyaux écrits en templates. Les fonctions actuelles (`getValueAF`…) deviennent le cas `T = E_Float` : le code existant ne change pas. |
| R3 | **Les algorithmes acceptent tout type réel.** | Noyaux critiques pour la performance : templates sur le type (deux types quand donneur et receveur diffèrent). Autres fonctions (prétraitement, post-traitement) : copie en double en entrée, calcul, retour dans le type demandé (règle à fixer, voir Q2). |
| R4 | **Les calculs géométriques et topologiques restent en double.** | Mailleurs, booléens, Nuga, connectivité, recherche de donneurs : on calcule en double, même sur des entrées float32. Les tolérances de coïncidence sont relatives à la taille du maillage, pas absolues. |
| R5 | **Les entrées/sorties conservent le type.** | Écriture R4/R8 selon le dtype (déjà le cas en CGNS). Lecture qui garde le type du fichier, sans le ramener en double. Le fp16 n'a pas de type CGNS : à l'écriture, il est converti en R4 (voir Q3). |
| R6 | **MPI suit le dtype.** | Le type MPI des échanges de champs est déduit du dtype du tableau, et non fixé à `MPI_DOUBLE`. |

### État actuel de `dev` au regard de ces règles (constaté)

- **R1, R2** : `getValueAF`, `getValueAI` et `getFromNumpyArray` ne vérifient aucun dtype. Un numpy float32 est lu comme du double, ce qui donne des valeurs fausses et une lecture hors du tableau.
- **R5** :
  - la lecture CGNS HDF garde les données R4 en float32 (`getArrayR4`, `NPY_FLOAT`) ;
  - en revanche, la lecture pickle convertit R4→R8 (`Internal._adaptTypes`, `convertR42R8=True` par défaut) ;
  - les lecteurs v3d et plot3d acceptent les fichiers dans l'autre précision depuis les PR #8 à #11 (soumises à onera, #881 à #884).
- **R3** :
  - les compresseurs sz, fpc et cellN lisent leur entrée comme du double contigu ;
  - les scalaires `numpy.float32` étaient refusés ou mal aiguillés, ce que corrige #3.

## 5. Règles côté Fast

| # | Règle | Ce que ça implique |
|---|---|---|
| F1 | **Une précision de Fast découplée de celle de Cassiopee.** | Aujourd'hui, `REAL_E` et `-r8` viennent de `KCore.Dist.getPP()` et `getForArgs()` (via `createFortranBuilder`) : la précision de Fast est celle de Cassiopee. Il faut une option propre à Fast, par exemple `Dist.getPP(realSize=...)` ou une variable dédiée. Ainsi, Fast en fp32 complet tourne au-dessus d'un Cassiopee r8 inchangé. |
| F2 | **Un type réel propre à Fast en C++.** | Les 2534 occurrences d'`E_Float` du C++ de Fast désignent aujourd'hui le type de Cassiopee. Il faut un type propre (`fast_real_t`, ou plusieurs types pour la précision mixte), comme `nuga_real_t` l'a fait pour Nuga. |
| F3 | **Des transferts paramétrés par deux types.** | Les transferts de la boucle de Fast sont ceux de FastC (`FastC/TRANSFERT/*.h`, `K_FASTC::setIBCTransfersCommonVar2`), pas ceux de Connector. Pour la précision mixte et la variation par zone, ils doivent être des templates sur un type donneur et un type receveur. |
| F4 | **Des tableaux Python typés.** | Les 108 `numpy.float64` explicites de Fast (`param_real`, linelets IBM…) deviennent des choix de précision, tableau par tableau. |
| F5 | **Un chemin implicite à décider.** | Le code tangent et adjoint généré par Tapenade (53 fichiers `*_d.for` et ADJOINT) est en `REAL*8` en dur. On peut le garder en double, avec conversion à l'interface, ou le régénérer en `REAL_E`. |

## 6. Ce que devient le travail r4 déjà fait

| Élément | Devenir |
|---|---|
| Correctifs valables aussi en r8 (isoSurfMC, netgen, X-Ray, lecteurs, FSDM, zfp, scalaires numpy…) | **Repris**, déjà en PR ou intégrés (voir `point_extraction_r8.md`). |
| Calculs internes en double (générateurs, `rotate`, `onedmap`, tanh, Musker, rayon-triangle, Eikonal, Delaunay…) | **Repris** : c'est la règle R4 appliquée. Ils rendent les algorithmes robustes à des entrées float32, et ne changent rien en r8. |
| Tolérances relatives aux coordonnées (`realMergeTol`), formats `"d"` | **Repris**, au titre de la règle R4. |
| Garde-fous de dtype | **Repris sous forme d'accesseurs typés (R1, R2)**, sans la conversion sur place. |
| `E_NpyFloat` partout dans le Python | **Remplacé** par la règle « respecter le dtype de l'entrée ». Le défaut reste float64. |
| Nuga réécrit avec `nuga_real_t` | **Inutile**, puisqu'`E_Float` reste `double`. |
| Build r4 global (`_r4`) | **Optionnel** : à garder, si on le souhaite, comme détecteur en CI (compilation et sous-ensemble de tests), sans références propres. Voir Q5. |

## 7. Plan de mise en œuvre

| Phase | Période | Contenu | Critère de fin |
|---|---|---|---|
| 0 | décembre 2026 | Valider cette note avec les deux équipes. Finir d'extraire les bugs latents r8 restants (VTK, compresseurs, Gmsh, spline, isValue). | Note validée, PR ouvertes. |
| 1 | décembre 2026 à janvier 2027 | Cassiopee : accesseurs typés (R1, R2), lecture qui garde le type (R5), MPI selon le dtype (R6), compresseurs (R3). | r8 identique sur toute la base de tests ; nouveaux tests sur des arbres float32 (accès, aller-retour R4/R8). |
| 2 | janvier à février 2027 | Fast : précision découplée (F1), type propre (F2), tableaux Python typés (F4). **Preuve de concept : Taylor-Green avec les champs de Fast en float32 au-dessus d'un Cassiopee r8 inchangé.** | Taylor-Green à Mach 0,1 et 0,5, en r8 et en fp32, comparés. C'est le point de départ du stage. |
| 3 | stage, février à septembre 2027 | Stratégies de précision mixte dans Fast, transferts mixtes (F3), stockage fp16, métriques. Adaptations de Cassiopee au besoin (Post sur des champs float32, etc.). | Livrables du stage. |
| 4 | après le stage | Décider de l'avenir du build r4 global et de la généralisation au GPU. | — |

## 8. Questions ouvertes

| # | Question | Proposition |
|---|---|---|
| Q1 | Un accesseur reçoit un dtype inattendu : erreur, ou copie convertie ? | Erreur dans les accesseurs C. Conversion explicite, côté Python, quand on la veut. |
| Q2 | Quel dtype pour le résultat d'une fonction de Cassiopee appelée sur des données float32 ? | Le dtype de l'entrée, sauf pour les quantités géométriques dérivées (volumes, normales), à discuter. |
| Q3 | Comment écrire du fp16 en CGNS, qui n'a pas de type 16 bits ? | Conversion en R4 à l'écriture, avec un marqueur (nœud descripteur) pour retrouver le fp16 à la lecture, si besoin. |
| Q4 | La lecture pickle convertit R4→R8 : faut-il garder ce comportement ? | Garder le type (R5), au moins sur option, pour les reprises de calcul en précision mixte. |
| Q5 | Le build r4 global : abandon, détecteur en CI, ou produit ? | Détecteur en CI (compilation et sous-ensemble de tests), sans références propres. |
| Q6 | Comment s'appelle l'option de précision de Fast, et où vit-elle ? | Option propre à Fast dans ses scripts d'installation, qui s'appuie sur une version de `Dist.getPP()` paramétrable. |

## 9. Références

- `point_extraction_r8.md` : état des PR et des bugs latents.
- Branche r4 : `claude/optimistic-maxwell-xwotl2`, en particulier `docs/developers/SinglePrecision.md` et `docs/developers/SinglePrecision_r4_failures.md`.
- Stage DAAA-2027-16, « Précision mixte contrôlée dans le code Fast ».
- Plan de portage GPU de Fast : branche `claude/serene-mccarthy-bfdnxx` de `diarrar/Fast`.
