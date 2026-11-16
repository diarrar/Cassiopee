# Compilation de Cassiopee en simple précision (r4)

Bilan au 16/11/2026, branche `claude/admiring-planck-933s9c` (31 commits au-dessus de `dev`, `dev` fusionné à 0ae977d).

## Objectif et principe

Cassiopee calcule historiquement en double précision : `E_Float` vaut `double`, le Fortran est compilé avec `-r8`.
L'objectif est de pouvoir compiler l'ensemble des modules avec `E_Float = float`, les bibliothèques tierces restant inchangées.

Le mécanisme suit celui des entiers 64 bits (`_i8`) :

- suffixe `_r4` dans `ELSAPROD` (par exemple `juno_gcc_r4`), cumulable avec `_i8` et `_DBG` ;
- `Dist.ESINGLEREAL()` ajoute `-DE_SINGLEREAL` en C/C++, passe `REAL_E="REAL*4"` au préprocesseur Fortran et retire `-r8`/`-fdefault-real-8` ; le suffixe est retiré pour la recherche de la configuration dans `installBase.json` ;
- `DefTypes.h` : `E_DOUBLEREAL` découle de l'absence de `E_SINGLEREAL` ; `E_NPY_FLOAT` (NPY_DOUBLE ou NPY_FLOAT32), `E_EPSILON` (1e-12 ou 1e-6) et `E_FloatAsInt` (entier de la taille de `E_Float`) dépendent de la précision ;
- `DefFunction.h` : en r4, surcharges `double` et mixtes float/double de `E_min`/`E_max`, pour que les appels avec un littéral double restent non ambigus ;
- côté Python, `Converter.Internal.E_NpyFloat` (numpy.float64 ou numpy.float32) est le dtype des numpys échangés avec les modules compilés, et `KCore.realTol(tol)` plafonne à 1e-6 en r4 les tolérances géométriques par défaut des API ;
- les points d'entrée NumPy de KCore (`getFromArray*`, `getFromNumpyArray`) refusent un numpy qui n'est pas dans la précision compilée, au lieu de le lire corrompu. C'est le seul changement de comportement en r8 : un float32 passé par erreur est maintenant refusé.

Installation : `export ELSAPROD=<prod>_r4` (ou `MACHINE=<prod>_r4` avec `sh_Cassiopee_local`), puis `./install`. Les binaires vont dans `Dist/bin/<prod>_r4`, séparés du build r8.

## Nuga calcule en double

Le noyau géométrique Nuga (Delaunay, booléens, conformizers, arbres de boîtes et kd-trees, prédicats) est réglé pour le double et n'est pas robuste en float : relâcher `ZERO_M` et `EPSILON` casse la triangulation. Il calcule donc en double quelle que soit la précision compilée :

- `nuga_real_t` (double) dans `Nuga/include/defs.h` remplace `E_Float` dans les 122 fichiers de Nuga, et `K_FLD::FloatArray` est `DynArray<double>` ;
- les conversions sont dans Nuga : `DynArray` depuis et vers `FldArray` de tout type réel, `ArrayAccessor<FldArrayF>` qui remet ses entrées et points en double, surcharges de `KdTree`, `BbTree` et `BoundingBox` acceptant des points et boîtes en `E_Float` (`NUGA_OTHER_REAL`) ;
- l'API Dyn de KCore (`K_ARRAY::getFromArray`/`buildArray` sur `DynArray`) travaille dans le type de Nuga ;
- les modules convertissent localement ce qui reste : Intersector (PolyMeshTools, chimère conservative), Connector (`maskGen`, `blankCellsTetra`, `setInterpDataCons`), Generator (T3mesher2D, quad2pyra, qualité), Converter (DynArrayIO), Post, Transform, le cœur de maillage CAO d'OCC (`OCCSurface`, `CADviaOCC` en `nuga_real_t`), KCore (`CompGeom/trianglesIntersection`, `Connect/cleanConnectivity`, `Linear/eigen`).

En double précision tous ces types sont identiques, le binaire r8 ne change pas. Un mailleur Delaunay autonome construit à partir des sources de Nuga donne le même maillage en r8 et en r4.

## Frontières avec les bibliothèques en double

| Composant | Traitement |
|---|---|
| OpenCASCADE (OCC) | locales `Standard_Real` aux appels qui renvoient par référence, recopie dans les `E_Float` ; cœur de maillage en `nuga_real_t` |
| tetgen, mmg (Generator) | types `REAL`/`double` des bibliothèques à l'interface |
| usurp (Post) | conversion explicite des tableaux, usurp travaille en `real(kind=rd)` |
| CPlot | les zones gardent leur stockage `double`, conversion à la copie (`zoneCopyField`) |
| Expression (Converter) | moteur SIMD en double, vues directes en r8, tampons convertis et recopiés en r4, tableaux float acceptés |
| lecteur arc, FSDM | buffers double ou types mémoire HDF5 adaptés |
| HDF/ADF CGNS | lecture dans la précision compilée (conversion HDF5 à la volée, ou copie pour ADF), écriture pilotée par le dtype |
| MPI | `E_PCM_FLOAT`, `XMPI_FLOAT` (XCore), `E_MPI_FLOAT` (Converter.Mpi4py) |
| Nuga | calcule en double, voir ci-dessus |
| Compressor (zfp, sz, fpc) | non traité : décompression en double, sorties float64 |
| Netgen (Generator), zoltan (XCore) | non traités |

## État par lot

**Lot 0, infrastructure : terminé.** Validé par un run r8 complet de la branche identique aux références `dev` gcc (1428 OK).

**Lot 1, couche Python : explicite terminé, implicite ouvert.** Les `numpy.float64` explicites sont remplacés dans les modules et dans 62 tests, les polices de Geom et les tableaux vides des générateurs AMR sont typés. Les créations NumPy sans dtype (environ 1100 dans les modules, dont Geom 467 et Connector 212) ne sont traitées qu'au fur et à mesure que le garde-fou les signale. Les tolérances par défaut des API (83 dans 24 fichiers) passent par `KCore.realTol`.

**Lot 2, modules C++ : terminé.** Tous les modules compilent en r4 avec gcc 12. Un balayage syntaxique des 1218 sources C++ dans les deux modes ne montre aucune différence r4/r8. Fortran : compile, non relu (21 fichiers avec des littéraux `D0`, promus en double dans les expressions).

**Lot 3, entrées/sorties : CGNS terminé.** Formats binaires legacy compilés mais non relus ligne à ligne.

**Lot 4, frontières : fait**, sauf Compressor, Netgen et zoltan, voir le tableau.

**Lot 5, validation : en cours.** Tolérances de `KCore/test.py` en r4 : 1e-5 absolu, 1e-6 relatif. Les références pickle (`.ref*`), arrays et pyTrees, sont converties dans la précision compilée à la lecture, ce qui permet de comparer un run r4 aux références r8.

## Validation

Protocole retenu, tout en gcc sur juno :

1. qualification de `dev` gcc contre les références officielles `Data_juno` (stack Intel) : 1357 identiques, 71 écarts imputables au compilateur (ordre d'itération de conteneurs, dernier bit propagé dans des décisions combinatoires, lissages itératifs). Ces 71 références ont été régénérées en gcc pour constituer `Data_juno_gcc` ;
2. r8 de la branche contre `Data_juno_gcc` : 1428 OK ;
3. r4 de la branche contre `Data_juno_gcc` copié en `Data_juno_gcc_r4` : 851 FAILED au premier run (dont environ 550 dus au banc de comparaison), 399 au second (commit 4a86414, Nuga en double). Le troisième, après la correction des tolérances par défaut, est à faire.

Le répertoire de références dépend d'`ELSAPROD` (`Data_<ELSAPROD>`), il faut amorcer `Data_<prod>_r4` depuis `Data_<prod>` avant un run r4. Un `update` depuis un build r4 n'écrase donc jamais les références r8.

Pour les tests MPI lancés hors `mpirun` (import de `Converter.Mpi` dans un test séquentiel), l'initialisation singleton d'Open MPI peut échouer sur un nœud alloué ; `OMPI_MCA_pml=ob1 OMPI_MCA_btl=self,vader` règle le cas sur un nœud. validCassiopee ne détecte pas ces échecs (motifs non reconnus), il faut relire la sortie.

## Écarts r4 restants, par famille (run du 16/11, 399 FAILED)

- **Mailleurs, booléens et conformizers Nuga** (T3mesher2D, gapfixer, fittingPlaster, conformUnstr, splitManifold, collarMesh, diffMesh, intersection) : cause identifiée, les contours remis au mailleur n'étaient pas fermés parce que `G.close` et `T.join` ont des tolérances par défaut de 1e-12, sous la résolution du float. Traité par `KCore.realTol`, à confirmer par le prochain run.
- **Écarts numériques purs** (129 tests) : 66 sous 1e-3 (gradients, divergences, projections, lissages, accumulation d'arrondi float) à couvrir par une tolérance r4 de test relevée ou à accepter test par test ; 63 au-dessus, dont 42 au-dessus de 1e-1 (cellN, indicateurs, intersections de rayons, blanking, P1ConsInterpolation) qui sont des décisions qui basculent, à examiner un par un.
- **Tailles de maillage différentes** (92 tests) : mailleurs et booléens, à rejuger après la correction des tolérances.
- **Valeurs float64 dans le résultat** (7 tests) : décompresseurs de Compressor, tests à typer.
- **Non identifiés** : TIMEOUT octree2Struct ; segfault connectMatchPT_m3, streamLine2PT_m1, prepareIBMData_m1 ; tetgen `error no 3` ; `sharpEdges set is empty` ; mapSplit `getDistantIndex`.
- **Attendus** (15 tests) : getSizeOf (tailles en octets), tests comparant des dtypes.

Les tests dont le résultat est combinatoirement sensible à la précision (booléens, mailleurs, AMR, adaptMesh) différeront des références r8 par construction. Leur critère en r4 est l'absence de plantage et un résultat plausible ; des références r4 propres devront être générées une fois le code stabilisé.

## Correctifs notables trouvés par la validation r4

- `globalIndex` stockait un entier 64 bits dans chaque case d'un champ `E_Float` (corruption mémoire en r4) : `E_FloatAsInt`.
- Branches `#else` de `#ifdef E_DOUBLEREAL` jamais compilées (parenthèses manquantes dans Intersector/PolyMeshTools).
- Eikonal (Dist2Walls) : critère de convergence absolu 1e-6, inaccessible en float, boucle infinie ; critère relatif en r4.
- Ambiguïtés `std::max(x, 1.e-12)` et `E_max` avec `E_Float = float` : surcharges mixtes dans `DefFunction.h`, littéraux castés.
- Tolérances géométriques par défaut de 1e-12 dans les API Python : contours non fermés, échec du mailleur Delaunay ; `KCore.realTol`.
- `Converter.Mpi4py` diffusait les champs en `MPI.DOUBLE` ; datatype dérivé de `E_NpyFloat`.
- Tolérance relative de l'alignement des points donneurs de l'AMR (Connector) relâchée en r4.
