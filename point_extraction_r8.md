# Point d'étape : extraction des correctifs indépendants du fp32 (26/11/2026, complété le même jour)

## Contexte

La branche r4 (`claude/optimistic-maxwell-xwotl2`) contient des correctifs qui valent aussi en double précision (r8), mêlés au travail fp32. Il s'agit de les sortir en PR courtes vers `dev`, puis vers onera/Cassiopee. Les développeurs Cassiopee demandent **une seule catégorie de problème par PR**.

## 1. État des PR

### Intégré dans `dev` d'onera
| PR fork | Contenu | onera |
|---|---|---|
| #1 | isoSurfMC (sortie non initialisée), netgen (`api` passé comme `center`), masque X-Ray (`indir` non initialisé), netgen/tetgen en api 3 | fusionnée (#873) |

### Corrigé en amont pendant la validation
| Problème | Correctif amont |
|---|---|
| Écriture FSDM en i8 : tampons `Poly2D` en int32 écrits en INT64 (segfault de `convFsdmPT_t1`/`t2`) | #874 onera |
| `Geom/pyproject.toml` : le motif `"Data"` n'installait pas `UIUCAirfoils.cgns` (`airfoil_t1`, `airfoilPT_t1`) | `cf31cad` onera |
| `printf` en `SF_D_` de netgen défaits par le report manuel de la PR #1 | `2c2f138` |

### Soumis à onera, en attente de validation
| PR fork | Catégorie | onera |
|---|---|---|
| #7 | Racine incomplète renvoyée par les lecteurs CGNS HDF et ADF | #876 |
| #8 | Fichiers v3d et plot3d écrits dans l'autre précision réelle | #881 |
| #9 | plot3d : ordre des octets des réels et de l'iblank (lecture et écriture) | #882 |
| #10 | plot3d : placement des champs q/f, format du fichier de fonctions écrit | #883 |
| #11 | plot3d : fichiers de fonctions à entiers 8 octets | #884 |

#9, #10 et #11 sont empilées sur #8 : il faut les fusionner dans l'ordre #8 → #9 → #10 → #11. Sur un banc de 80 fichiers plot3d générés indépendamment, `dev` en relit correctement 12 ; après chaque étape, le compte passe à 18, 32, 60 puis 80.

### Ouvertes sur le fork, pas encore soumises à onera
| PR fork | Catégorie |
|---|---|
| #3 | Scalaires `numpy.float32` refusés ou mal aiguillés (`T._rotate`, snear, `getFromList`…) |
| #6 | Écriture FSDM : conditions limites NGON en `PointRange` (dernière face oubliée, indices décalés ; segfault à la relecture sur `dev`) |
| #12 | zfp : tableaux 2D en ordre Fortran mal disposés à la décompression |
| #13 | zfp : contrôle des entrées de `pack` (float32, vues non contiguës, éléments d'une liste) |

Toutes ces branches sont à jour avec `dev` (`04858e6`) et se fusionnent sans conflit, y compris sur `dev` d'onera.

## 2. Reste à extraire : bugs latents vérifiés

« Vérifié » signifie : reproduit sur une compilation i8 locale de `dev`, ou établi en lisant le code.

| # | Problème | Vérification | Découpage proposé |
|---|---|---|---|
| 1 | **Lecteur VTK legacy** (`GenIO_binvtk.cpp`) ; la doc r4 notait déjà « la lecture ASCII échoue aussi en r8 » | compilation i8 : un fichier ASCII contenant un tétraèdre donne 0 élément | 3 PR : **a)** ASCII : les `CELLS` ne sont jamais lues (code commenté, « force to triangle for now »), les `CELL_TYPES` sont donc lus de travers ; **b)** binaire `POLYDATA` : `fread(cells, sizeof(E_Int), …)` dans un tampon `int` (débordement en i8), sans inversion des octets ; **c)** `FIELD` à 2 composantes : `for` sans accolades, deux virgules de suite, nom de variable vide |
| 2 | **Compresseurs sz, fpc, cellN** : entrée lue comme du double contigu sans contrôle | code. Le lecteur CGNS de `dev` garde les données R4 en float32 (`getArrayR4`, `NPY_FLOAT`), donc compresser un champ R4 donne des valeurs fausses en r8 | 1 PR, même correction que #13 pour zfp |
| 3 | **`Converter/Gmsh.py`** : connectivité créée en `numpy.int32` en dur, fausse en i8 | code (ligne 53) | 1 PR d'une ligne (`E_NpyInt`) |
| 4 | **`spline`** (`evalSpline`, `N`) : un paramètre hors du vecteur de nœuds laisse `k = -1`, et `xt[k-d]` lit avant le début du tableau | code (rare en double) | 1 petite PR |
| 5 | **`Internal.isValue`** : un nœud float32 comparé à un float Python renvoie toujours `False` (dtype différent) | code (touche les fichiers R4) | 1 petite PR |
| 9 | **`Transform` `joinAll`** (zones multi-éléments) : `npts += nptsk` dans la boucle sur les connectivités, les points sont comptés une fois par connectivité. La moitié du tableau de sortie n'est pas initialisée (valgrind : 124 lectures non initialisées sur `dev`). Trouvé le 26/11 grâce à l'échec intermittent de `joinPT_t4` en r4 (tous les points fusionnés en un seul) | correctif prêt et testé (r4 et r8 : `joinPT_t4` passe, lectures non initialisées supprimées) | 1 PR |
| 10 | **`KCore.test.checkObject_`** : `IndexError` quand deux listes comparées n'ont pas la même longueur, au lieu de signaler un écart | sortie de `generateAMRMeshStepPT_t2` en r4 | 1 petite PR (outillage de test) |
| 11 | **`FldArray<int>::releaseMemory`** lit un membre non initialisé sur les `FldArrayI` multi-éléments créés par `getFromArray3` | valgrind | à examiner |

## 3. À discuter avec les développeurs avant toute PR

| # | Problème | Pourquoi en discuter |
|---|---|---|
| 6 | **Garde-fous de dtype dans KCore** : `getFromNumpyArray`, `getFromArray*` et `getValueAF` lisent un numpy float32 comme du double (cas des fichiers R4 en r8). Fast passe environ 400 fois par ces accesseurs (210 `getValueAF`, 203 `getFromNumpyArray`). | La correction change le comportement : refuser, ou convertir. La branche r4 convertit **sur place** dans `getValueAF`, ce qui gênerait la précision mixte de Fast. Mieux vaut des accesseurs typés (`getValueAF<T>`, `getFromNumpyArray<T>`) qui vérifient le dtype sans conversion silencieuse. |
| 7 | **Masque X-Ray** : la graine de perturbation des rayons est réinitialisée à chaque triangle (`idum = -1`), donc un rayon qui retombe sur un nœud garde indéfiniment la même perturbation. | Le défaut existe en r8, mais la correction change certains résultats r8 dans les cas ambigus. |
| 8 | **`extractPlane` sur une surface** : selon la doc r4, le r8 est déjà dégénéré (167 segments pour 29 points). | Pas encore analysé. Il faut investiguer avant de parler de correctif. |

## 4. Ce qui reste dans la série r4 (ne change rien en r8)

Ces changements de la branche r4 laissent le r8 identique et ne servent qu'au r4. Ils iront dans la série de PR r4, pas dans les correctifs r8 :
- calculs internes repassés en double : générateurs cartésiens, `rotate`, générateurs trigonométriques, `onedmap`, loi en tanh, loi de Musker, intersection rayon-triangle, `solveQuadratic` d'Eikonal, Delaunay de `K_COMPGEOM`, centres de faces de `blankIntersectingCells`, `stitchedHat` ;
- formats `"d"` explicites dans les `PYPARSETUPLE_` ;
- tolérances `E_REALTOL` et `realMergeTol` ;
- Nuga en double ;
- infrastructure r4 : `DefTypes.h`, `Dist.py`, `E_NpyFloat`, `getRealSize`, tolérances de `test.py`.

## 5. Proposition

1. **Ouvrir maintenant les PR des points 1 à 5** : sept petites PR, une catégorie chacune, chacune testée en i8 avec un exemple avant/après.
2. **Points 6 à 8** : les présenter aux développeurs Cassiopee. Le point 6 est à concevoir avec la précision mixte de Fast en tête (stage DAAA-2027-16).
3. **Ensuite, la série r4**, en commençant par l'infrastructure.

## 6. Outils de vérification disponibles dans la session

- Compilation i8 locale de KCore, Converter, Geom, Transform, Generator et Post (gcc 13, HDF5 série 1.10.10), dans un worktree séparé (`/home/user/cass-dev`).
- Banc plot3d : 80 fichiers générés par un script Python indépendant de Cassiopee, plus un programme C++ qui appelle les lecteurs et écrivains de `GenIO`.
- Module zfp compilé seul, avec un banc d'allers-retours (1D à 4D, ordre C et Fortran, float32, vues).
