# Compilation de Cassiopee en simple précision (r4)

Bilan au 17/11/2026, branche `claude/admiring-planck-933s9c` et sa suite `claude/optimistic-maxwell-xwotl2` (`dev` fusionné à 0ae977d).

## Objectif et principe

Cassiopee calcule historiquement en double précision : `E_Float` vaut `double`, le Fortran est compilé avec `-r8`.
L'objectif est de pouvoir compiler l'ensemble des modules avec `E_Float = float`, les bibliothèques tierces restant inchangées.

Le mécanisme suit celui des entiers 64 bits (`_i8`) :

- suffixe `_r4` dans `ELSAPROD` (par exemple `juno_gcc_r4`), cumulable avec `_i8` et `_DBG` ;
- `Dist.ESINGLEREAL()` ajoute `-DE_SINGLEREAL` en C/C++, passe `REAL_E="REAL*4"` au préprocesseur Fortran et retire `-r8`/`-fdefault-real-8` ; le suffixe est retiré pour la recherche de la configuration dans `installBase.json` ;
- `DefTypes.h` : `E_DOUBLEREAL` découle de l'absence de `E_SINGLEREAL` ; `E_NPY_FLOAT` (NPY_DOUBLE ou NPY_FLOAT32), `E_EPSILON` (1e-12 ou 1e-6), `E_REALTOL(x)` (plafonne à 1e-6 en r4 une tolérance de coïncidence écrite pour le double) et `E_FloatAsInt` (entier de la taille de `E_Float`) dépendent de la précision ;
- `DefFunction.h` : en r4, surcharges `double` et mixtes float/double de `E_min`/`E_max`, pour que les appels avec un littéral double restent non ambigus ;
- côté Python, la précision est demandée au binaire (`KCore.getRealSize()`, 4 ou 8) et non déduite d'`ELSAPROD` : `Converter.Internal.E_NpyFloat` (numpy.float64 ou numpy.float32) est le dtype des numpys échangés avec les modules compilés, et `KCore.realTol(tol)` plafonne à 1e-6 en r4 les tolérances géométriques par défaut des API ;
- les points d'entrée NumPy de KCore (`getFromArray*`, `getFromNumpyArray`, macro `K_NUMPY_CHECK_FLOAT` pour les lectures directes des modules) refusent un numpy qui n'est pas dans la précision compilée, au lieu de le lire corrompu ; le chemin pyTree (`K_PYTREE::getValueAF`) convertit le nœud en place avec un avertissement. C'est le seul changement de comportement en r8 : un float32 passé par erreur est maintenant refusé ou converti.

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
| lecteur arc, FSDM, tau | buffers double ou types mémoire HDF5/netCDF adaptés |
| tecplot binaire, plot3d | la précision du fichier est lue (drapeaux ou séparateurs) puis convertie, un build r4 relit les fichiers double |
| HDF/ADF CGNS | lecture dans la précision compilée (conversion HDF5 à la volée, ou copie pour ADF), écriture pilotée par le dtype |
| MPI | `E_PCM_FLOAT`, `XMPI_FLOAT` (XCore), `E_MPI_FLOAT` (Converter.Mpi4py) |
| Nuga | calcule en double, voir ci-dessus |
| Compressor (zfp, sz, fpc, cellN) | compression en float64 (données compressées identiques dans les deux précisions), retour au dtype compilé à la décompression |
| Netgen (Generator), zoltan (XCore) | non traités |

## État par lot

**Lot 0, infrastructure : terminé.** Validé par un run r8 complet de la branche identique aux références `dev` gcc (1428 OK).

**Lot 1, couche Python : explicite terminé, implicite ouvert.** Les `numpy.float64` explicites sont remplacés dans les modules et dans 62 tests, les polices de Geom et les tableaux vides des générateurs AMR sont typés. Les créations NumPy sans dtype (environ 1100 dans les modules, dont Geom 467 et Connector 212) ne sont traitées qu'au fur et à mesure que le garde-fou les signale. Les tolérances par défaut des API (83 dans 24 fichiers) passent par `KCore.realTol`.

**Lot 2, modules C++ : terminé.** Tous les modules compilent en r4 avec gcc 12. Un balayage syntaxique des 1218 sources C++ dans les deux modes ne montre aucune différence r4/r8. Fortran : compile, non relu (21 fichiers avec des littéraux `D0`, promus en double dans les expressions).

**Lot 3, entrées/sorties : CGNS, tecplot, plot3d, tau, FSDM, arc terminés.** Autres formats binaires (vtk, ply, stl, gmsh, v3d) compilés mais non relus ligne à ligne.

**Lot 4, frontières : fait**, sauf Netgen et zoltan, voir le tableau.

**Lot 5, validation : en cours.** Tolérances de `KCore/test.py` en r4 : 1e-5 absolu, 1e-5 relatif. Les références pickle (`.ref*`), arrays et pyTrees, sont converties dans la précision compilée à la lecture, ce qui permet de comparer un run r4 aux références r8.

## Validation

Protocole retenu, tout en gcc sur juno :

1. qualification de `dev` gcc contre les références officielles `Data_juno` (stack Intel) : 1357 identiques, 71 écarts imputables au compilateur (ordre d'itération de conteneurs, dernier bit propagé dans des décisions combinatoires, lissages itératifs). Ces 71 références ont été régénérées en gcc pour constituer `Data_juno_gcc` ;
2. r8 de la branche contre `Data_juno_gcc` : 1428 OK ;
3. r4 de la branche contre `Data_juno_gcc` copié en `Data_juno_gcc_r4` : 851 FAILED au premier run (dont environ 550 dus au banc de comparaison), 399 au second (commit 4a86414, Nuga en double), 321 au troisième (commit 7081c36, tolérances par défaut), 306 FAILED et 1 TIMEOUT au quatrième (commit 5fe788a, 17/11).

Validation complémentaire sur un conteneur Ubuntu 24.04 (gcc 13, Python 3.13, numpy 2.5, OCC 7.6, machine `ubuntu`), même protocole avec des références générées par le r8 de la branche (`Data_ubuntu`, copiées en `Data_ubuntu_r4`) :

| Run | Commit | r4 | r8 |
|---|---|---|---|
| référence | 0e92489 | 1146 OK, 281 FAILED, 1 TIMEOUT | 1416 OK, 11 FAILED, 1 TIMEOUT (références créées) |
| 17/11 | 21d83ba | 1161 OK, 267 FAILED, 0 TIMEOUT | 1417 OK, 11 FAILED : r8 inchangé |
| 18/11 | 0fde385 | 1169 OK, 259 FAILED, 0 TIMEOUT | 1417 OK, 11 FAILED : r8 inchangé |
| 18/11 | 43a52d1 | 1188 OK, 240 FAILED, 0 TIMEOUT | 1417 OK, 11 FAILED : r8 inchangé |
| 18/11 | 647a6d3 | 1192 OK, 236 FAILED | 1417 OK, 11 FAILED : r8 inchangé |
| 19/11 | c19ca89 | 1213 OK, 215 FAILED | 1417 OK, 11 FAILED : r8 inchangé ; build r8 de `dev` identique sur les 1417 tests |

Les 11 échecs r8 et 16 des échecs r4 tiennent au conteneur (pas de `tkinter`, rendus OSMesa, tests OCC qui lisent un `cube.cgns` produit par un autre test). 54 tests en échec dans le run juno de 5fe788a passent sur ce banc (sphère et tore en double, Eikonal). Avec numpy 2, les scalaires `numpy.float32` sont beaucoup plus fréquents qu'avec numpy 1 (règles NEP 50) : le banc juno, s'il est en numpy 1.x, ne voit pas cette famille.

Comparaison géométrique des 47 échecs Intersector (nœuds appariés au plus proche voisin, faces et cellules comparées comme ensembles de nœuds, champs après permutation) : 21 donnent le même maillage renuméroté (groupe à 3,77e-1, adaptBox, diffSurf, extractNonStar...), 25 diffèrent par des décisions qui basculent (quelques cellules des booléens, raffinement adaptCells, diagonales des octaèdres du raffinement tétraédrique à égalité de longueur, agglomération des petites cellules qui varie déjà en double selon l'arrondi des entrées, x0 = 0,4 sur un nœud de la distribution de booleanMinus_t1), 1 était un vrai défaut (pi de Nuga, corrigé).

Répartition des 267 échecs r4 du run du 17/11 : taille de maillage différente 90, écart maximal ≥ 1 : 34, entre 1e-2 et 1 : 33, entre 1e-4 et 1e-2 : 22, sous 1e-4 : 27, autres différences (valeurs de nœuds, objets) 33, exceptions 8 (Compressor en float64, connectMatch_t3, P1ConsChimeraCoeffs, fittingPlaster d'extractSlices, MirabelleRefine), plantages 3 (prepAMRFull_m1, extractIBMWallSolutionPT_m1, streamLine2PT_m1), environnement 16. snapFront_t1 alterne entre OK et FAILED d'un run à l'autre.

Le répertoire de références dépend d'`ELSAPROD` (`Data_<ELSAPROD>`), il faut amorcer `Data_<prod>_r4` depuis `Data_<prod>` avant un run r4. Un `update` depuis un build r4 n'écrase donc jamais les références r8.

Pour les tests MPI lancés hors `mpirun` (import de `Converter.Mpi` dans un test séquentiel), l'initialisation singleton d'Open MPI peut échouer sur un nœud alloué ; `OMPI_MCA_pml=ob1 OMPI_MCA_btl=self,vader` règle le cas sur un nœud. validCassiopee ne détecte pas ces échecs (motifs non reconnus), il faut relire la sortie.

## Écarts r4 restants, par famille (run du 16/11, 399 FAILED)

- **Mailleurs, booléens et conformizers Nuga** (T3mesher2D, gapfixer, fittingPlaster, conformUnstr, splitManifold, collarMesh, diffMesh, intersection) : cause identifiée, les contours remis au mailleur n'étaient pas fermés parce que `G.close` et `T.join` ont des tolérances par défaut de 1e-12, sous la résolution du float. Traité par `KCore.realTol`, confirmé par le run suivant (conformUnstr passe, T3mesher2D maille).
- **`error finalizing` restants** (agglomerateSmallCells, diffSurf, diffMesh, en tableau et en pyTree) : la sphère de `D.sphere` était calculée en float, `sin(pi)` y vaut -8,7e-8, les pôles devenaient des anneaux de points distincts que Nuga ne fusionne pas (fusion à 1e-12), et `diffSurf` gardait les cellules intérieures à la sphère. Reproduit et corrigé hors Cassiopee : les générateurs trigonométriques calculent en double. extractSlicesPT_t1 (fichier CGNS lu) reste à examiner.
- **AMR, prepAMRFull_m1/t1/t2** : le masque X-Ray gardait des faces ambiguës après ses passes de perturbation, `blankCells` ignorait ce statut et passait au Fortran une table de rayons non initialisée. Reproduit hors Cassiopee et corrigé.
- **Écarts numériques purs** (129 tests) : 66 sous 1e-3 (gradients, divergences, projections, lissages, accumulation d'arrondi float) à couvrir par une tolérance r4 de test relevée ou à accepter test par test ; 63 au-dessus, dont 42 au-dessus de 1e-1 (cellN, indicateurs, intersections de rayons, blanking, P1ConsInterpolation) qui sont des décisions qui basculent, à examiner un par un.
- **Tailles de maillage différentes** (92 tests) : mailleurs et booléens, à rejuger après la correction des tolérances.
- **Valeurs float64 dans le résultat** (7 tests) : décompresseurs de Compressor, tests à typer.
- **Non identifiés** : TIMEOUT octree2Struct ; segfault connectMatchPT_m3, streamLine2PT_m1, prepareIBMData_m1 ; tetgen `error no 3` ; `sharpEdges set is empty` ; mapSplit `getDistantIndex`.
- **Attendus** (15 tests) : getSizeOf (tailles en octets), tests comparant des dtypes.

Les tests dont le résultat est combinatoirement sensible à la précision (booléens, mailleurs, AMR, adaptMesh) différeront des références r8 par construction. Leur critère en r4 est l'absence de plantage et un résultat plausible ; des références r4 propres devront être générées une fois le code stabilisé.

## Limites connues du mode r4

- Les indices stockés dans des champs réels (`getInterpolatedPoints`, `getInterpolatedEXPoints`, `setInterpolations`) ne sont exacts que jusqu'à 2^24, soit 16,7 millions de points par zone.
- `E_FloatAsInt` vaut `int32` en r4 : en `_r4_i8`, l'index global de `createGlobalIndex` est tronqué au-delà de 2^31.
- Les tolérances C++ en dur (environ 500 lignes entre 1e-10 et 1e-15 hors tiers) ne sont traitées qu'au cas par cas avec `E_REALTOL` ; les gardes anti-division par zéro peuvent rester telles quelles, les seuils de coïncidence géométrique sont à passer sous `E_REALTOL` quand la validation les désigne.

## Correctifs notables trouvés par la validation r4

- `globalIndex` stockait un entier 64 bits dans chaque case d'un champ `E_Float` (corruption mémoire en r4) : `E_FloatAsInt`.
- Branches `#else` de `#ifdef E_DOUBLEREAL` jamais compilées (parenthèses manquantes dans Intersector/PolyMeshTools).
- Eikonal (Dist2Walls) : critère de convergence absolu 1e-6, inaccessible en float, boucle infinie ; critère relatif en r4.
- Scalaires `numpy.float32` : `numpy.float64` dérive de `float`, pas `numpy.float32`. Les valeurs calculées à partir des coordonnées étaient refusées ou mal aiguillées : `T._rotate` envoyait un angle vers la surcharge à deux axes, `PyFloat_Check` refusait les `snear` (getIBMPtsWithFront, octree, octree3, zipper), `K_ARRAY::getFromList` les lisait comme des entiers. Tests de type élargis à `numpy.floating` en Python, macro `K_PYFLOAT_CHECK` (kPython.h) en C++.
- Références converties en float32 : les sentinelles `DBL_MAX` devenaient `inf` et `diffArrays` refusait le tableau (getCurvatureRadius). `KCore.castFloatArray` les ramène à `FLT_MAX` (`E_MAX_FLOAT` en r4).
- `intersectRayTriangle` calcule en double : sur la couture de `D.sphere`, la coordonnée barycentrique du bon triangle vaut -1,5e-6 en float (tolérance 1e-6), le rayon était rejeté par tous les triangles et le point projeté sur la face opposée (projectDir, projectAllDirs, projectRay).
- Masque X-Ray, essai non retenu : passer sous `E_REALTOL` la tolérance de coïncidence de `compIntersect` et celle de fusion de `compactZ` fait perdre 6 points de percée sur 1,56 million au cas 3D de maskXRay_t1 (identique au r8 sinon) sans corriger les cas 2D, dont l'écart de 2,7e-3 vient du seul rayon tangent au cercle et de sa perturbation aléatoire.
- Nuga : `PI`, `PI_2`, `PI_4`, `SQRT3` venaient de `K_CONST` (E_Float, donc arrondis au float en r4) ; Nuga calculait en double avec pi = 3,14159274 et l'interpolation P1 conservative ne trouvait plus aucune intersection (P1ConsInterpolationPT_t1 : toutes les cellules à 0). Littéraux double dans `defs.h`.
- Générateurs cartésiens (`cart`, `cartHexa`, `cartTetra`, `cartPenta`, `cartPyra`, `cartNGon`) : origine et pas lus et combinés en double, les nœuds r4 sont exactement l'arrondi float des nœuds r8 (avant : -3 + 10*0,3 = 1,2e-7 au lieu de 4e-16, plans presque confondus qui faisaient échouer le booléen de P1ConservativeChimeraCoeffs). `cartr1`, `cartr2` non traités.
- Loi en tanh (`StretchF.for`, enforce*, distrib2) : Newton calculé en double ; en float la régularité qui guide le choix du nombre de points était fausse à 1e-4.
- Variables `double` lues avec le format réel d'E_Float (`R_`, `"f"` en r4) : Python n'écrivait que 4 octets sur 8. checkPointInCEBB répondait au hasard, pointedHat construisait son sommet avec des octets de pile, idem nurbs, agglomerateNonStarCells, extractBadVolCells, immerseNodes. Formats `"d"` explicites ; un balayage de tous les appels `PYPARSETUPLE_` (format réel contre type déclaré) ne trouve pas d'autre occurrence.
- `onedmap` (hyper2D, map) : l'abscisse curviligne cumulée sur toute la ligne était en float, les deux branches d'une ligne repliée (sillage d'un maillage en C) ne tombaient plus sur les mêmes points et connectMatch ne trouvait plus le raccord. slope, paramFunc et interp sont des templates (`CompGeom/onedmapT.h`), onedmap utilise l'instanciation double en r4.
- streamLine2 (Stream2) : le point d'essai qui oriente les faces d'une cellule NGON était décalé de 1e-8, et la perturbation des rayons coplanaires de 1e-6 fois une normale non normée (1e-8 sur des faces de 0,1), sous la résolution des coordonnées ; faces orientées à l'envers (la ligne rebondit entre deux cellules) ou indétermination. Décalages de 1e-3 fois la taille de la face en r4.
- `cleanConnectivity` (et `V_cleanConnectivity`) : tolérances de fusion positives sous `E_REALTOL` ; un point interpolé deux fois sur une arête partagée (isoSurf, isoLine...) n'était pas fusionné et l'isoligne de extractSlices était morcelée.
- `indiceStruct2Unstr` : la tolérance porte sur le carré de la distance (1e-14, soit 1e-7) ; nœuds non retrouvés (indice -1 lu comme le dernier nœud par numpy) dans addNormalLayers, couche extrudée effondrée et coordonnées NaN dans MirabelleRefine. Plancher (1e-6)^2 en r4.
- `T.rotate` : angle, axe, trigonométrie et quaternions en double (cos(90°) valait -4,4e-8, la copie tournée ne partageait plus ses bords, merge_t1).
- Égalités dans les données de test, laissées en l'état : enforcePoint_t1 (x0 = 0,5 à mi-chemin de deux nœuds), booleanMinus_t1 cas 2 (x0 = 0,4 sur un nœud).
- Compressor : sz, zfp, fpc et cellN lisaient un champ float32 comme du float64, les données décompressées étaient fausses en r4 (écarts jusqu'à 1e117, NaN pour fpc). `_packNode` compresse en float64 et `_unpackNode` revient au dtype compilé ; le format compressé ne change pas. Indépendamment de la précision, zfp rend des valeurs fausses pour un numpy 2D en ordre Fortran (aussi en r8, non traité).
- Loi de paroi de Musker (IBM : computeFrictionVelocity, setIBCTransfers) : pow(y+ + 10,6 ; 9,6) dépasse le plus grand float pour y+ > ~9000, la loi et sa dérivée devenaient inf et utau était faux d'un facteur 2 à 10. Temporaires en double.
- setInterpolations : la somme des coefficients pondérée par le cellN devait valoir 1 à 1e-8 près, un tiers des receveurs de setInterpolationsPT_t1 étaient extrapolés au lieu d'interpolés. `E_REALTOL`.
- Octree en rapport 3 (checkBalancing3, mergeElement) : seuils absolus de 1e-10 sur des tailles 3 fois plus grandes, raffinement en cascade (octree 8796 points au lieu de 1024). Seuil à la résolution des coordonnées.
- checkCartesian : tolérance de 1e-12 sur les différences de pas ; Internal.isValue : un scalaire réel était comparé en float64 au nœud float32.
- Tests adaptés (valeurs écrites en dur ou dtype) : extractPoint (5), newDataArrayPT_t1.
- Écarts attendus vérifiés : getCurvatureRadius (courbure par différences finies, limitée par la précision des coordonnées : le r8 sur coordonnées arrondies en float donne le même bruit), BB_t2 (boîte orientée d'un cube indéterminée, valeurs propres égales), mmgs (même nombre de sommets, optimisation sensible aux entrées), isoLine_t1 (valeurs d'iso égales aux valeurs nodales), convertArrays2File / convertPyTree2FilePT_t4 (comparaison octet à octet de fichiers binaires).
- Eikonal, solveur `fim_old` (défaut de `Dist2Walls.eikonal`, TIMEOUT d'eikonal_t2 au run du 17/11) : la dernière formule de `solveQuadratic` perd ses chiffres significatifs en float (discriminant de l'ordre de 12h² obtenu par différence de termes de l'ordre de 80), la mise à jour d'un nœud pouvait dépasser sa valeur courante de plus que le critère et le nœud restait indéfiniment dans la liste active. `solveQuadratic` calcule en double, et en r4 un nœud dont la mise à jour ne diminue pas la valeur est convergé. Reproduit hors Cassiopee (grille 128³ et sphère de l'eikonal_t2) : r4 converge en 0,4 s, écart au r8 de 3e-7 ; binaire r8 inchangé bit à bit.
- Ambiguïtés `std::max(x, 1.e-12)` et `E_max` avec `E_Float = float` : surcharges mixtes dans `DefFunction.h`, littéraux castés.
- Tolérances géométriques par défaut de 1e-12 dans les API Python : contours non fermés, échec du mailleur Delaunay ; `KCore.realTol`.
- `Converter.Mpi4py` diffusait les champs en `MPI.DOUBLE` ; datatype dérivé de `E_NpyFloat`.
- Tolérance relative de l'alignement des points donneurs de l'AMR (Connector) relâchée en r4.
- Masque X-Ray (`maskXRay.cpp`) : quand des faces restaient ambiguës, `compactZ` n'était pas appelé et `blankCells` utilisait `indir` non initialisé avec un Z vide (sortie de tableau dans `k6searchblankednodesx2d`). Les tables sont désormais toujours cohérentes et `blankCells` avertit. En r4 seulement, la perturbation des rayons change à chaque passe : le germe aléatoire était réinitialisé à chaque triangle, un rayon qui retombait sur un nœud recevait indéfiniment la même perturbation. Les points de percée des cas ordinaires sont identiques bit à bit, en r8 comme en r4.
- Générateurs `D.circle`, `D.sphere`, `D.torus`, `D.cone`, `G.cylinder` : angles et trigonométrie en double, résultat stocké en `E_Float`. Pôles, coutures et fermetures à 360° sont exacts à 1e-16 près, au lieu de 1e-7. Code inchangé en r8.
