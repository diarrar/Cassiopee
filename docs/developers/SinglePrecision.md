# Compilation de Cassiopee en simple précision (r4)

Bilan au 15/11/2026, branche `claude/admiring-planck-933s9c` (17 commits au-dessus de `dev`, `dev` fusionné à 0ae977d).

## Objectif et principe

Cassiopee calcule historiquement en double précision : `E_Float` vaut `double`, le Fortran est compilé avec `-r8`.
L'objectif est de pouvoir compiler l'ensemble des modules avec `E_Float = float`, les bibliothèques tierces restant inchangées.

Le mécanisme suit celui des entiers 64 bits (`_i8`) :

- suffixe `_r4` dans `ELSAPROD` (par exemple `juno_gcc_r4`), cumulable avec `_i8` et `_DBG` ;
- `Dist.ESINGLEREAL()` ajoute `-DE_SINGLEREAL` en C/C++, passe `REAL_E="REAL*4"` au préprocesseur Fortran et retire `-r8`/`-fdefault-real-8` ;
- `DefTypes.h` : `E_DOUBLEREAL` découle de l'absence de `E_SINGLEREAL` ; `E_NPY_FLOAT` (NPY_DOUBLE ou NPY_FLOAT32), `E_EPSILON`, `E_FloatAsInt` dépendent de la précision ;
- côté Python, `Converter.Internal.E_NpyFloat` (numpy.float64 ou numpy.float32) est le dtype des numpys échangés avec les modules compilés ;
- les points d'entrée NumPy de KCore (`getFromArray*`, `getFromNumpyArray`) refusent un numpy qui n'est pas dans la précision compilée, au lieu de le lire corrompu.

Installation : `export ELSAPROD=<prod>_r4` (ou `MACHINE=<prod>_r4` avec `sh_Cassiopee_local`), puis `./install`. Les binaires vont dans `Dist/bin/<prod>_r4`.

## Frontières avec les bibliothèques en double

Les bibliothèques tierces et quelques noyaux internes travaillent en double quelle que soit la précision compilée. La conversion se fait à leur interface :

| Composant | Traitement |
|---|---|
| OpenCASCADE (OCC) | locales `Standard_Real` aux appels qui renvoient par référence, recopie dans les `E_Float` |
| tetgen, mmg (Generator) | types `REAL`/`double` des bibliothèques à l'interface |
| usurp (Post) | conversion explicite des tableaux, usurp travaille en `real(kind=rd)` |
| CPlot | les zones gardent leur stockage `double`, conversion à la copie (`zoneCopyField`) |
| Expression (Converter) | moteur SIMD en double, vues directes en r8, tampons convertis et recopiés en r4 |
| lecteur arc, FSDM | buffers double ou types mémoire HDF5 adaptés |
| HDF/ADF CGNS | lecture dans la précision compilée (conversion HDF5 à la volée, ou copie pour ADF), écriture pilotée par le dtype |
| MPI | `E_PCM_FLOAT`, `XMPI_FLOAT` (XCore), `E_MPI_FLOAT` (Converter.Mpi4py) |
| Compressor (zfp, sz, fpc) | non traité : décompression en double, sorties float64 |
| Netgen (Generator), zoltan (XCore) | non traités |
| Nuga (Delaunay, booléens) | en cours : passage du noyau en double, voir ci-dessous |

## État par lot

**Lot 0, infrastructure : terminé.** Validé par un run r8 complet de la branche identique aux références `dev` (1428 OK).

**Lot 1, couche Python : explicite terminé, implicite ouvert.** Les `numpy.float64` explicites sont remplacés dans les modules et dans 62 tests. Les créations NumPy sans dtype (environ 1100 dans les modules, dont Geom 467 et Connector 212) ne sont traitées qu'au fur et à mesure que le garde-fou les signale.

**Lot 2, modules C++ : terminé.** Tous les modules compilent en r4 avec gcc 12. Fortran : compile, non relu (21 fichiers avec des littéraux `D0`, promus en double dans les expressions).

**Lot 3, entrées/sorties : CGNS terminé.** Formats binaires legacy compilés mais non relus ligne à ligne.

**Lot 4, frontières : au cas par cas**, voir le tableau.

**Lot 5, validation : en cours.** Tolérances de `KCore/test.py` en r4 : 1e-5 absolu, 1e-6 relatif. Les références pickle (`.ref*`) sont converties dans la précision compilée à la lecture, ce qui permet de comparer un run r4 aux références r8.

## Validation

Protocole retenu, tout en gcc sur juno :

1. qualification de `dev` gcc contre les références officielles `Data_juno` (stack Intel) : 1357 identiques, 71 écarts imputables au compilateur (ordre d'itération de conteneurs, dernier bit propagé dans des décisions combinatoires, lissages itératifs). Ces 71 références ont été régénérées en gcc pour constituer `Data_juno_gcc` ;
2. r8 de la branche contre `Data_juno_gcc` : 1428 OK ;
3. r4 de la branche contre `Data_juno_gcc` copié en `Data_juno_gcc_r4` : premier dépouillement, 851 FAILED dont environ 550 dus au banc de comparaison (références relues en float64), corrigés depuis.

Le répertoire de références dépend d'`ELSAPROD` (`Data_<ELSAPROD>`), il faut amorcer `Data_<prod>_r4` depuis `Data_<prod>` avant un run r4. Un `update` depuis un build r4 n'écrase donc jamais les références r8.

Pour les tests MPI lancés hors `mpirun` (import de `Converter.Mpi` dans un test séquentiel), l'initialisation singleton d'Open MPI peut échouer sur un nœud alloué ; `OMPI_MCA_pml=ob1 OMPI_MCA_btl=self,vader` règle le cas sur un nœud. validCassiopee ne détecte pas ces échecs (motifs non reconnus), il faut relire la sortie.

## Écarts r4 restants, par famille

- **Mailleur Delaunay Nuga** (T3mesher2D, gapfixer, fittingPlaster, conformUnstr, splitManifold, booléens, collarMesh, xcelln) : les prédicats géométriques sont réglés pour le double ; en float une arête contrainte non retrouvée fait fuir le coloriage du domaine (`error finalizing`, code 77). Relâcher `ZERO_M`/`EPSILON` aggrave la triangulation. Décision : faire calculer Nuga en double avec conversion aux interfaces.
- **Écarts numériques entre 1e-5 et 3e-3** sur gradients, divergences, projections, lissages : accumulation d'arrondi float, à couvrir par les tolérances r4 ou à accepter test par test.
- **Décisions qui basculent** (cellN, indicateurs, intersections de rayons, blanking, P1ConsInterpolation) : écarts qualitatifs à examiner un par un.
- **Non identifiés** : TIMEOUT octree2Struct, adaptCellsPT_t9 ; segfault connectMatchPT_m3, prepAMRFull_m1 ; tetgen `error no 3` ; `sharpEdges set is empty` ; mapSplit `getDistantIndex`.
- **Attendus** : getSizeOf (tailles en octets), tests comparant des dtypes.

## Correctifs notables trouvés par la validation r4

- `globalIndex` stockait un entier 64 bits dans chaque case d'un champ `E_Float` (corruption mémoire en r4) : `E_FloatAsInt`.
- Branches `#else` de `#ifdef E_DOUBLEREAL` jamais compilées (parenthèses manquantes dans Intersector/PolyMeshTools).
- Eikonal (Dist2Walls) : critère de convergence absolu 1e-6, inaccessible en float, boucle infinie ; critère relatif en r4.
- Ambiguïtés `std::max(x, 1.e-12)` et `E_max` avec `E_Float = float` : surcharges mixtes dans `DefFunction.h`, littéraux castés.
