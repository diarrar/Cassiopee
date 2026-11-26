# Rapport : compilation de Cassiopee en simple précision (r4)

*Rapport de clôture du travail r4, novembre 2026. Branche `claude/zen-brown-uv22o1-r4`.*

> Ce rapport résume le travail et ses résultats, pour pouvoir le reprendre plus tard. Le détail technique est dans [`SinglePrecision.md`](SinglePrecision.md) (conception, frontières avec les bibliothèques externes, correctifs notables) et dans [`SinglePrecision_r4_failures.md`](SinglePrecision_r4_failures.md) (analyse test par test des échecs r4 au 19/11). La suite envisagée est dans [`MixedPrecision_design.md`](MixedPrecision_design.md).

## 1. Résumé

- **Objectif** : pouvoir compiler tout Cassiopee avec `E_Float = float` (option `_r4` d'`ELSAPROD`), première étape d'un Fast en précision réduite.
- **Résultat** :
  - les 19 modules compilent et s'installent en r4 et en r8 avec gcc 13 (vérifié le 26/11/2026) ;
  - *[à compléter avec les runs en cours]* : comparaison de la branche en r8 avec `dev`, et taux de réussite en r4 contre les références r8, avec les tolérances r4 (1e-5) ;
  - au 19/11 (branche d'origine), 1227 tests sur 1428 passaient en r4. Sur les 201 échecs, 94 étaient reproduits par le r8 sur des données arrondies en float (donc dus à la précision des données et non à un bug) ; les autres étaient des dérives numériques faibles, des égalités dans les données de test ou des effets d'environnement (voir § 5 et § 6).
- **Effets de bord utiles** : une vingtaine de bugs présents aussi en double précision, trouvés grâce au r4, ont été corrigés et proposés à onera/Cassiopee (§ 8).
- **Conclusion** : un r4 *global* est faisable, mais il ne répond pas au besoin réel de Fast (précision mixte, fp16, précision variable par zone) et impose un second produit à maintenir. Le travail est donc mis de côté au profit d'une **précision par tableau dans un seul build** (stratégie C, [`MixedPrecision_design.md`](MixedPrecision_design.md)). Une grande partie de ce qui a été fait ici y est réutilisable (§ 10).

## 2. Mécanisme

- Suffixe `_r4` dans `ELSAPROD`, cumulable avec `_i8` et `_DBG` (comme pour les entiers 64 bits).
- `Dist.ESINGLEREAL()` ajoute `-DE_SINGLEREAL` en C/C++, passe `REAL_E="REAL*4"` au préprocesseur Fortran et retire `-r8`.
- `DefTypes.h` définit, selon la précision : `E_NPY_FLOAT`, `E_EPSILON`, `E_REALTOL(x)` (plancher de tolérance à la résolution du float), `E_FloatAsInt` et `E_MAXFLOATINT` (2^24 en r4).
- En Python, `KCore.getRealSize()` interroge le binaire. `Internal.E_NpyFloat` sert de dtype, et `KCore.realTol` plafonne les tolérances géométriques par défaut.
- Nuga calcule toujours en double (`nuga_real_t`), avec des conversions à son interface.
- Les bibliothèques externes (OpenCASCADE, CPlot, tetgen, mmg, netgen, usurp, HDF5, MPI) sont interfacées dans leur propre type.

Installation : `export ELSAPROD=<prod>_r4`, puis `./install`. Les binaires vont dans `Dist/bin/<prod>_r4`, séparés du build r8.

## 3. Organisation de la branche

La branche est construite sur `dev` (`9b1e420`, synchronisé avec onera/Cassiopee le 26/11/2026). On y fusionne d'abord les PR de correctifs r8 encore ouvertes, puis le travail r4 en quinze commits. Ces commits sont thématiques là où les fichiers sont séparables, et regroupés par module ailleurs. Chaque message liste les sujets traités.

| Commit | Contenu |
|---|---|
| fusions | PR ouvertes de `diarrar/Cassiopee` : #3 (scalaires `numpy.float32`), #6 (FSDM `PointRange`), #7 (racine CGNS), #8 à #11 (lecteurs v3d et plot3d), #12 et #13 (zfp) |
| `r4: single precision build switch (infrastructure)` | `Dist.py`, `DefTypes.h`, `DefFunction.h`, KCore Python (`getRealSize`, `realTol`, `castFloatArray`), tolérances de `KCore.test` |
| `r4: Nuga computes in double…` | 127 fichiers de Nuga et l'API Dyn de KCore |
| `r4: KCore interfaces…` | accesseurs numpy et pyTree, géométrie (rayon-triangle, Delaunay, `onedmap`, `spline`), tolérances de fusion relatives |
| `r4: Converter…` | Python (`E_NpyFloat`), entrées/sorties CGNS, tecplot, tau, FSDM, arc, moteur d'expressions, `globalIndex`, MPI |
| `r4: Geom…`, `r4: Transform…`, `r4: Generator…`, `r4: Post…`, `r4: Connector…`, `r4: Dist2Walls…`, `r4: XCore and Intersector…`, `r4: OpenCASCADE and CPlot…`, `r4: other modules…` | adaptations module par module |
| `r4: tests independent of the compiled real precision` | 71 fichiers de test |
| `docs: …` | `SinglePrecision.md`, `SinglePrecision_r4_failures.md` |

Rien n'est perdu par rapport à la branche d'origine (`claude/optimistic-maxwell-xwotl2`, conservée telle quelle) : l'arbre final est identique à celui d'une fusion de cette branche dans la base, une fois retiré ce qui est déjà dans `dev` ou dans les PR.

## 4. Méthode de validation

1. **r8 identique** : `dev` compilé en r8 génère les références (`Data_<prod>`), puis la branche compilée en r8 est comparée à ces références. C'est la garantie que le travail r4 ne change rien pour les utilisateurs actuels.
2. **r4 contre références r8** : les références r8 sont copiées dans `Data_<prod>_r4`, puis on lance la branche en r4. Les références sont converties en float32 à la lecture. Tolérances r4 de `KCore.test` : 1e-5 en absolu et en relatif.
3. **Banc « r8 arrondi »** (utilisé pendant le développement) : le test tourne avec le build r8, mais tous les tableaux réels en entrée et en sortie de chaque fonction compilée sont arrondis au float. Si ce banc échoue aussi, l'écart vient de la précision des données ; s'il passe, l'écart r4 est suspect et doit être examiné.

## 5. Résultats

### 5.1 Historique

| Run | Banc | r4 | r8 |
|---|---|---|---|
| premier run, juno (gcc) | branche d'origine | 851 échecs (dont ~550 dus au banc de comparaison) | — |
| 16/11, juno | après Nuga en double (`4a86414`) | 399 échecs | — |
| 17/11, juno | après tolérances par défaut, garde-fous (`5fe788a`) | 306 échecs, 1 TIMEOUT | — |
| 17/11, Ubuntu | `21d83ba` | 1161 OK, 267 échecs | 1417 OK, 11 échecs (environnement), identique au r8 de `dev` |
| 19/11, Ubuntu | `65bb11a` | 1227 OK, 201 échecs | 1417 OK, 11 échecs, identique |

### 5.2 Validation de la branche propre (26/11/2026, conteneur Ubuntu 24.04, gcc 13, numpy 2.5, Python 3.13)

*À compléter avec les résultats des runs en cours.*

### 5.3 Validation à faire

Validation finale sur juno avec la pile Intel (`juno_r4` contre `Data_juno`), prévue par l'équipe une fois la branche prête.

## 6. Écarts r4 restants

*À compléter (classification du run de la branche propre, comparée à celle du 19/11).*

Au 19/11 (201 échecs), la classification de [`SinglePrecision_r4_failures.md`](SinglePrecision_r4_failures.md) était :

| Statut | Nombre | Signification |
|---|---|---|
| vérifié | 63 | cause établie test par test : égalité dans les données, problème dégénéré, comparaison octet à octet… |
| reproduit | 94 | le banc r8 arrondi échoue aussi : l'écart vient de la précision des données |
| dérive | 28 | écart faible (L0 < 1e-2) dû à l'arithmétique interne en float |
| environnement | 16 | propre au conteneur : rendus OSMesa, tkinter, dépendances entre tests |

Les familles d'écarts « par construction » sont celles où le résultat dépend de manière combinatoire des données : mailleurs Delaunay et booléens de Nuga, AMR, `adaptMesh`, masques et recherche de donneurs. Pour ces tests, le critère en r4 est l'absence de plantage et un résultat plausible ; des références r4 propres seraient nécessaires pour une non-régression.

## 7. Limites connues du mode r4

- **Coordonnées en float** : la résolution est de 1,2e-7 autour de 1, 2e-6 autour de 30 et 2e-3 autour de 30 000. Un maillage de couche limite résolue à la paroi (première maille d'environ 1e-6 de la longueur de référence) n'est pas représentable sur une géométrie réelle. C'est la principale limite physique d'un r4 global.
- **Indices stockés dans des champs réels** : exacts seulement jusqu'à 2^24 (16,7 millions de points par zone). `getInterpolatedPoints`, `getEXPoints`, `identifyMatching*` et `getIndexField` lèvent une erreur au-delà. En `_r4_i8`, `createGlobalIndex` refuse plus de 2^31 points.
- **Tolérances C++ en dur** : environ 500 lignes entre 1e-10 et 1e-15. Elles ne sont traitées qu'au cas par cas (`E_REALTOL`, `realMergeTol`), quand la validation les désigne.
- **Algorithmes sensibles aux entrées** (Delaunay, booléens, AMR, agglomération) : résultats différents du r8 par construction ; netgen et tetgen y sont eux-mêmes sensibles.
- **Pas de références r4** : la non-régression du mode r4 repose sur les références r8 et des tolérances relâchées.

## 8. Bugs présents aussi en double précision, trouvés grâce au r4

| Bug | Devenir |
|---|---|
| isoSurfMC : 3/4 de la sortie non initialisés ; netgen : `api` passé comme `center` ; masque X-Ray : `indir` non initialisé ; netgen/tetgen : connectivité fausse en api 3 | fusionnés dans onera/Cassiopee (#873) |
| Écriture FSDM en i8 : tampons int32 écrits en INT64 (segfault) | corrigé en amont (#874) |
| `Geom/pyproject.toml` : fichier `UIUCAirfoils.cgns` non installé | corrigé en amont (`cf31cad`) |
| Lecteurs CGNS : racine renvoyée incomplète | #7 → onera #876 |
| v3d et plot3d : fichiers de l'autre précision illisibles ; plot3d : ordre des octets, placement des champs, format du fichier de fonctions, entiers 8 octets | #8 à #11 → onera #881 à #884 |
| Scalaires `numpy.float32` refusés ou mal aiguillés | #3 |
| FSDM : conditions limites NGON en `PointRange` | #6 |
| zfp : tableaux 2D Fortran mal disposés ; entrées float32 ou non contiguës | #12, #13 |
| Restent à traiter : lecteur VTK legacy (ASCII, POLYDATA binaire, `FIELD`), compresseurs sz/fpc/cellN, `Gmsh.py` en i8, `spline`, `Internal.isValue` ; à discuter : garde-fous de dtype dans KCore, graine du masque X-Ray, `extractPlane` sur une surface | voir `point_extraction_r8.md` (branche `claude/zen-brown-uv22o1`) |

## 9. Bilan et leçons

- **Coût d'un r4 global**, en ordre de grandeur :
  - 436 fichiers modifiés, dont 127 pour faire calculer Nuga en double ;
  - environ 1100 créations numpy sans dtype dans les modules, traitées seulement là où un test les a révélées ;
  - environ 500 tolérances en dur ;
  - un second jeu de références à créer et à maintenir.
- **Ce qui a bien marché** :
  - le contrôle systématique « r8 identique » à chaque étape ;
  - le banc « r8 arrondi », qui sépare les écarts dus à la précision des données des vrais bugs ;
  - la comparaison géométrique des maillages, qui ignore la renumérotation ;
  - valgrind sur les échecs intermittents.
- **Ce que le r4 a appris sur le code** : beaucoup de seuils absolus (1e-10, 1e-12) supposent des coordonnées autour de 1 et le double. Les rendre relatifs à la taille du maillage améliore la robustesse en r8 aussi.
- **Limite de l'approche** : une précision unique, fixée à la compilation, ne permet ni la précision mixte, ni le fp16, ni une précision variable dans l'espace. C'est pourtant la trajectoire visée pour Fast.

## 10. Recommandation et réutilisation

La suite retenue est la **précision par tableau dans un seul build** ([`MixedPrecision_design.md`](MixedPrecision_design.md)). Ce qui est réutilisable de ce travail :

| Élément | Réutilisation |
|---|---|
| Calculs internes en double (générateurs, `rotate`, `onedmap`, tanh, Musker, rayon-triangle, Eikonal, Delaunay…) | directe : rend les algorithmes robustes à des entrées float32, sans rien changer en r8 |
| Tolérances relatives (`realMergeTol`, seuils à la résolution des coordonnées) | directe |
| Lecteurs indépendants de la précision du build | déjà en PR |
| Garde-fous de dtype | sous forme d'accesseurs typés, sans conversion sur place |
| Infrastructure `_r4` | à garder éventuellement comme détecteur en CI |
| Nuga en `nuga_real_t` | inutile si `E_Float` reste `double` |

## 11. Reprendre le travail

```
export ELSAPROD=<prod>_r4      # ou MACHINE=<prod>_r4 avec Envs/sh_Cassiopee_local
./install
```
- **Références** : copier `*/test/Data_<prod>` en `*/test/Data_<prod>_r4`. Les références r8 sont converties en float32 à la lecture.
- **Lancer la base** : `validCassiopee -r`. Pour les tests MPI hors `mpirun` sur un nœud alloué, utiliser `OMPI_MCA_pml=ob1 OMPI_MCA_btl=self,vader`.
- **Vérifier** que la branche compilée en r8 reste identique à `dev` avant toute modification.
