# Échecs r4 : statut test par test

Run du 18/11/2026 sur le conteneur Ubuntu (gcc 13, numpy 2), commit ebc4d2b, r4 comparé aux références r8 de la branche (`Data_ubuntu` copié en `Data_ubuntu_r4`, tolérances r4 de `KCore.test` : 1e-5 absolu et relatif). Le r8 du même commit donne 1417 OK et 11 échecs d'environnement.

## Statuts

- **corrigé** : défaut r4 trouvé par cette revue et corrigé après le run ; le test passe en r4.
- **vérifié** : cause établie test par test (égalité dans les données, problème dégénéré, reproduction en r8 sur entrées arrondies en float, comparaison octet à octet), ou résultat identique au r8 à une renumérotation près (comparaison géométrique : nœuds appariés au plus proche voisin à 1e-5 de la diagonale, éléments, faces et cellules comparés comme ensembles de nœuds).
- **reproduit** : le test exécuté avec le build r8, en arrondissant au float tous les tableaux réels en entrée et en sortie de chaque fonction compilée (ce que fait le stockage r4 entre deux appels), échoue aussi avec les tolérances r4. L'écart vient de la précision des données, pas d'un défaut du code r4. Présomption forte, pas une preuve : sur un témoin de 120 tests qui passent en r4, ce banc en fait échouer 3 (2,5 %) et en fait planter 5.
- **dérive** : le r8 arrondi passe, l'écart r4 reste faible (L0 < 1e-2, pas de changement de taille) : arithmétique interne en float (gradients, divergences, lissages).
- **suspect** : le r8 arrondi passe alors que le r4 change une décision (taille de maillage, cellN, objets). À examiner un par un : c'est dans cette catégorie qu'a été trouvé le défaut de blankIntersectingCells.
- **classé** : seulement classé automatiquement (le banc r8 arrondi plante, ou test MPI non passé au banc). Surtout l'AMR et l'IBM.
- **environnement** : propre au conteneur (rendu OSMesa, tkinter absent, tests OCC qui lisent un `cube.cgns` produit par un autre test).

La colonne « Écart » est le classement automatique de la sortie de validCassiopee ; « Comparaison géométrique » est le verdict du comparateur, quand il a pu juger (les maillages structurés ne le sont pas).

**Total : 215 échecs. corrigé 2, vérifié 47, reproduit 86, dérive 28, suspect 17, classé 19, environnement 16**

## corrigé (2)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| Connector | blankIntersectingCellsPT_t1.py | écart L0 1.0e+00 | structuré (non jugé) | passe | centres de faces sommés en double (6c390be), passe en r4 |
| Connector | blankIntersectingCells_t2.py | écart L0 1.0e+00 | maillage différent | passe | centres de faces sommés en double (6c390be), passe en r4 |

## vérifié (47)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| Connector | maskXRayPT_t1.py | écart L0 2.7e-03 | maillage différent | passe | cas 3D identique ; cas 2D : seul le rayon tangent au cercle, perturbé aléatoirement |
| Connector | maskXRay_t1.py | écart L0 2.7e-03 | maillage différent | passe | cas 3D identique ; cas 2D : seul le rayon tangent au cercle, perturbé aléatoirement |
| Connector | prepareAMRDataPT_t1.py | autre DIFF | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Connector | setInterpolationsPT_t1.py | autre DIFF | structuré (non jugé) | échoue | égalité : 8 donneurs sur 796 pour des points exactement sur une face |
| Converter | convertArrays2File_t1.py | autre DIFF |  | échoue | attendu : fichiers binaires comparés octet à octet |
| Converter | convertPyTree2FilePT_t4.py | autre DIFF |  | passe | attendu : fichiers binaires comparés octet à octet |
| Converter | getSizeOfPT_m1.py | autre DIFF | MPI (non jugé) | MPI | attendu : taille en octets, moitié en float32 |
| Converter | getSizeOfPT_t1.py | autre DIFF |  | passe | attendu : taille en octets, moitié en float32 |
| Generator | BBPT_t2.py | écart L0 1.4e+00 | structuré (non jugé) | échoue | problème dégénéré : boîte orientée d'un cube, trois valeurs propres égales |
| Generator | BB_t2.py | écart L0 1.4e+00 | structuré (non jugé) | échoue | problème dégénéré : boîte orientée d'un cube, trois valeurs propres égales |
| Generator | enforcePointPT_t1.py | écart L0 4.2e-02 | structuré (non jugé) | échoue | égalité : x0 = 0,5 à mi-chemin de deux nœuds |
| Generator | enforcePoint_t1.py | écart L0 4.2e-02 | structuré (non jugé) | échoue | égalité : x0 = 0,5 à mi-chemin de deux nœuds |
| Generator | mmgsPT_t1.py | taille différente | maillage différent | échoue | algorithme sensible : même nombre de sommets (718), tous sur la sphère, placés ailleurs |
| Generator | mmgs_t1.py | taille différente | maillage différent | échoue | algorithme sensible : même nombre de sommets (718), tous sur la sphère, placés ailleurs |
| Geom | getCurvatureRadiusPT_t1.py | écart L0 6.4e+00 | structuré (non jugé) | échoue | limite des données : le r8 sur coordonnées arrondies en float donne le même bruit |
| Geom | getCurvatureRadius_t1.py | écart L0 6.4e+00 | maillage différent | échoue | limite des données : le r8 sur coordonnées arrondies en float donne le même bruit |
| Intersector | adaptBoxPT_t1.py | écart L0 6.0e+00 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | adaptBox_t1.py | écart L0 6.0e+00 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | adaptCellsCenterPT_t1.py | autre DIFF | maillage différent | échoue | réf. 3 : mêmes octaèdres (sommets, volume), seule la diagonale de découpe change (diagonales égales) |
| Intersector | adaptCellsNodalPT_t1.py | autre DIFF | maillage différent | échoue | réf. 3 : mêmes octaèdres (sommets, volume), seule la diagonale de découpe change (diagonales égales) |
| Intersector | agglomerateNonStarCellsPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | agglomerateNonStarCells_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | agglomerateSmallCellsPT_t1.py | taille différente | maillage différent | échoue | algorithme sensible : en r8, 4816 cellules (entrées double) contre 4966 (entrées arrondies en float) ; le r4 donne 4966 |
| Intersector | agglomerateSmallCells_t1.py | écart L0 1.9e+00 | maillage différent | échoue | algorithme sensible : en r8, 4816 cellules (entrées double) contre 4966 (entrées arrondies en float) ; le r4 donne 4966 |
| Intersector | booleanMinus_t1.py | écart L0 1.7e+00 | maillage différent | échoue | réf. 1 : bascule locale ; réf. 2 : x0 = 0,4 tombe sur un nœud de la distribution (entrée différente) |
| Intersector | booleanModifiedSolidNGPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | booleanModifiedSolidNG_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | booleanUnion_t1.py | écart L0 1.7e+00 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | computeGrowthRatioPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | computeGrowthRatio_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | convexifyFacesPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | convexifyFaces_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | diffMeshPT_t1.py | taille différente | maillage différent | échoue | algorithme sensible : en r8, 4816 cellules (entrées double) contre 4966 (entrées arrondies en float) ; le r4 donne 4966 |
| Intersector | diffMesh_t1.py | taille différente | maillage différent | échoue | algorithme sensible : en r8, 4816 cellules (entrées double) contre 4966 (entrées arrondies en float) ; le r4 donne 4966 |
| Intersector | diffSurfPT_t1.py | écart L0 1.9e+00 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | diffSurf_t1.py | écart L0 1.9e+00 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | extractNonStarPT_t1.py | écart L0 1.5e+01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | extractPathologicalsPT_t1.py | écart L0 1.5e+01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | extractUncomputablesPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | prepareCellsSplitPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | prepareCellsSplit_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | simplifyCellsPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | simplifyCells_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | splitCellsPT_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Intersector | splitCells_t1.py | écart L0 3.8e-01 | même maillage | échoue | comparaison géométrique : même maillage (renumérotation ou écart de nœuds < 1e-5 de la diagonale) |
| Modeler | box_t1.py | taille différente | maillage différent | échoue | Delaunay 2D (ellipse2D, box2D) : le r8 sur contour arrondi en float donne le résultat r4 (81 points, 121 triangles) |
| Post | isoLine_t1.py | taille différente | maillage différent | passe | égalité : valeurs d'iso égales à des valeurs nodales |

## reproduit (86)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| Connector | adaptF42PT_t1.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | chimeraInfoPT_t1.py | écart L0 4.6e+05 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | extractIBMWallFieldsPT_t1.py | écart L0 1.1e-03 | même maillage, champs différents (2.50e-01) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Connector | getOversetInfoPT_t1.py | écart L0 3.9e+00 |  | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | maximizeBlankedCellsPT_t1.py | écart L0 2.0e+00 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | maximizeBlankedCells_t1.py | écart L0 2.0e+00 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | maximizeBlankedCells_t2.py | écart L0 2.0e+00 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | prepareIBMDataPT_t1.py | autre DIFF | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | setInterpDataIBMPT_t2.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | setInterpDataPT_t4.py | autre DIFF | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | setInterpolationsPT_t2.py | autre DIFF | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | setInterpolationsPT_t3.py | autre DIFF | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | setInterpolationsPT_t4.py | autre DIFF | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Connector | setInterpolationsPT_t5.py | autre DIFF | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Converter | convFsdmPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Converter | normalizePT_t2.py | écart L0 6.2e-04 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Dist2Walls | dist2WallsEikonalOctreePT_t1.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Dist2Walls | dist2WallsEikonalPT_t2.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | T3mesher2DPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | T3mesher2D_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | delaunayPT_t1.py | écart L0 1.9e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | delaunay_t1.py | écart L0 1.9e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | gapfixerPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | gapfixer_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | generateIBMMeshPT_t1.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getAngleRegularityMapPT_t1.py | écart L0 2.8e-02 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getAngleRegularityMapPT_t2.py | écart L0 3.1e-04 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getAngleRegularityMapPT_t3.py | écart L0 4.8e-02 | même maillage, champs différents (1.98e-02) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Generator | getAngleRegularityMap_t1.py | écart L0 2.8e-02 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getNonOrthogonalityMapPT_t3.py | écart L0 3.4e-02 | même maillage, champs différents (2.80e-02) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Generator | getOrthogonalityMapPT_t1.py | écart L0 3.8e-04 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getOrthogonalityMapPT_t2.py | écart L0 2.3e-05 | même maillage, champs différents (2.29e-05) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Generator | getOrthogonalityMap_t1.py | écart L0 3.8e-04 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getRegularityMapPT_t1.py | écart L0 7.4e-05 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | getRegularityMap_t1.py | écart L0 7.4e-05 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | octree2StructPT_t4.py | autre DIFF | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Generator | octree2Struct_t5.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Geom | addSeparationLine_t1.py | taille différente | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Geom | surface_t2.py | écart L0 1.5e-05 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Geom | text2D_t1.py | écart L0 2.1e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Geom | text3D_t1.py | écart L0 1.8e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Geom | volumeFromCrossSectionsPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Geom | volumeFromCrossSections_t1.py | écart L0 1.9e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Initiator | visbalPT_t2.py | écart L0 1.5e-04 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Initiator | wissocqPT_t2.py | écart L0 3.5e-01 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | adaptCellsPT_t2.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | adaptCellsPT_t5.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | adaptCellsPT_t6.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | adaptCells_t2.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanIntersectionNGPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanIntersectionNG_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanIntersectionPT_t1.py | autre DIFF | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanIntersection_t1.py | écart L0 1.7e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanMinusNGPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanMinusNG_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanMinusPT_t1.py | autre DIFF | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanUnionNGPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanUnionNGPT_t3.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | booleanUnionNG_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | conformUnstrPT_t1.py | autre DIFF | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | selfXPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Intersector | xcellnSphYinYangPT_t3.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Modeler | column_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Modeler | cylinder_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Modeler | star_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| OCC | convertCAD2Arrays_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| OCC | convertCAD2PyTreePT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| OCC | meshTRI_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | computeAerodynamicCoefficientsPT_t1.py | écart L0 4.5e-05 | même maillage, champs différents (7.63e-05) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Post | computeDiv2PT_t4.py | écart L0 2.7e-05 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | computeIndicatorFieldPT_t1.py | écart L0 1.0e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | computeIndicatorField_t1.py | écart L0 1.0e+00 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | enforceIndicatorForFinestLevel_t1.py | écart L0 2.0e+03 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | extractIBMWallFieldsPT_t1.py | écart L0 1.1e-03 | même maillage, champs différents (2.50e-01) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Post | extractIBMWallSolutionPT_t1.py | écart L0 1.7e+00 | même maillage, champs différents (1.70e+00) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Post | extractMesh_t6.py | écart L0 2.8e-02 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | extractPlanePT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | extractPlanePT_t2.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | extractPlanePT_t3.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | extractPlane_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | extractPlane_t3.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Post | projectCloudSolutionPT_t1.py | écart L0 4.3e-02 | même maillage, champs différents (4.25e-02) | échoue | le r8 avec données arrondies en float échoue aussi ; même maillage, champs différents |
| Post | projectCloudSolution_t1.py | écart L0 5.0e-02 | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Transform | deformMesh_t1.py | écart L0 2.4e-02 | structuré (non jugé) | échoue | le r8 avec données arrondies en float échoue aussi |
| Transform | splitManifoldPT_t1.py | autre DIFF | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |
| Transform | splitTRIPT_t1.py | taille différente | maillage différent | échoue | le r8 avec données arrondies en float échoue aussi |

## dérive (28)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| Connector | buildFrontIBMPT_t1.py | écart L0 1.8e-04 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Generator | TTMPT_t1.py | écart L0 7.8e-05 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Generator | TTM_t1.py | écart L0 7.8e-05 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Generator | enforceCurvaturePT_t1.py | écart L0 5.1e-04 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Generator | surfaceWalkPT_t1.py | écart L0 6.4e-05 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Generator | surfaceWalk_t1.py | écart L0 6.4e-05 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Initiator | lambPT_t2.py | écart L0 5.8e-04 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Initiator | lamb_t1.py | écart L0 2.8e-05 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeCurl_t2.py | écart L0 1.1e-03 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeDiv_t2.py | écart L0 1.0e-04 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeGrad2PT_t1.py | écart L0 2.6e-05 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeGrad2PT_t3.py | écart L0 1.8e-05 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeGrad2PT_t4.py | écart L0 3.5e-05 | même maillage, champs différents (4.22e-05) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float ; même maillage, champs différents |
| Post | computeGrad2PT_t5.py | écart L0 2.4e-05 | même maillage, champs différents (4.22e-05) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float ; même maillage, champs différents |
| Post | computeGradPT_t1.py | écart L0 2.2e-03 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeGradPT_t2.py | écart L0 2.2e-03 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeGradPT_t4.py | écart L0 1.8e-05 | même maillage, champs différents (1.03e-05) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float ; même maillage, champs différents |
| Post | computeGrad_t2.py | écart L0 2.2e-04 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeNormCurl_t2.py | écart L0 1.1e-03 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeNormGradPT_t1.py | écart L0 2.7e-03 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeNormGradPT_t2.py | écart L0 2.7e-03 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | computeNormGrad_t2.py | écart L0 1.9e-04 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | integMomentNorm_t1.py | écart L0 2.3e-05 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | perlinNoisePT_t1.py | écart L0 3.4e-04 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Post | streamRibbon2_t1.py | écart L0 9.2e-03 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Transform | projectOrthoSmoothPT_t1.py | écart L0 2.6e-05 | structuré (non jugé) | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Transform | projectOrthoSmooth_t1.py | écart L0 4.3e-05 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |
| Transform | projectOrtho_t1.py | écart L0 9.6e-05 | maillage différent | passe | écart faible ; le r8 arrondi passe : arithmétique interne en float |

## suspect (17)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| Apps | MirabelleRefine_t1.py | autre DIFF | structuré (non jugé) | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Connector | blankCells_t2.py | écart L0 1.0e+00 | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Connector | setInterpDataPT_t6.py | taille différente | structuré (non jugé) | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Connector | setInterpDataPT_t9.py | autre DIFF | structuré (non jugé) | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Generator | checkMeshPT_t2.py | autre DIFF |  | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Generator | snapFront_t1.py | écart L0 1.0e+00 | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Generator | stitchedHatPT_t1.py | écart L0 6.3e-02 | structuré (non jugé) | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Generator | stitchedHat_t1.py | écart L0 6.3e-02 | structuré (non jugé) | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Geom | cylinderPT_t1.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Modeler | circle_t1.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Post | computeDiv2PT_t2.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Post | computeDiv2PT_t3.py | taille différente | même maillage, champs différents (1.83e-03) | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe ; même maillage, champs différents |
| Post | extractPlane_t2.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Post | isoSurfMC_t1.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Post | isoSurfPT_t1.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Post | isoSurf_t1.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |
| Post | selectCells3PT_t1.py | taille différente | maillage différent | passe | décision qui bascule (taille, cellN, objets) ; le r8 arrondi passe |

## classé (19)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| Apps | createDragonMesh_t1.py | taille différente | maillage différent | exception | r8 arrondi : exception |
| Connector | IBMinterpolateExtrude2Dto3D_t1.py | taille différente | structuré (non jugé) | plantage | r8 arrondi : plantage |
| Connector | IBMrectilinear_t1.py | taille différente | structuré (non jugé) | plantage | r8 arrondi : plantage |
| Connector | prepAMRFull_m1.py | plantage | MPI (non jugé) | MPI | r8 arrondi : MPI |
| Connector | prepAMRFull_t1.py | taille différente | maillage différent | plantage | r8 arrondi : plantage |
| Connector | prepAMRFull_t2.py | taille différente | maillage différent | plantage | r8 arrondi : plantage |
| Connector | prepareAMRDataPT_m1.py | autre DIFF | MPI (non jugé) | MPI | r8 arrondi : MPI |
| Connector | prepareIBMDataPT_t3.py | taille différente | structuré (non jugé) | plantage | r8 arrondi : plantage |
| Connector | prepareIBMData_m1.py | plantage | MPI (non jugé) | MPI | r8 arrondi : MPI |
| Generator | IBMExtrude2Dto3D_t1.py | taille différente | structuré (non jugé) | plantage | r8 arrondi : plantage |
| Generator | IBMExtrude2Dto3D_t2.py | taille différente | structuré (non jugé) | plantage | r8 arrondi : plantage |
| Generator | generateAMRMeshCartInPT_t1.py | taille différente | maillage différent | plantage | r8 arrondi : plantage |
| Generator | generateAMRMeshMultiRes2DPT_t1.py | taille différente | maillage différent | plantage | r8 arrondi : plantage |
| Generator | generateAMRMeshVarSnearTboxPT_t1.py | taille différente | maillage différent | plantage | r8 arrondi : plantage |
| Generator | generateAMRMeshVariableSnearPT_t1.py | taille différente | maillage différent | plantage | r8 arrondi : plantage |
| Post | computeAerodynamicCoefficientsPT_m1.py | écart L0 4.5e-05 | MPI (non jugé) | MPI | r8 arrondi : MPI |
| Post | extractIBMWallSolutionPT_m1.py | écart L0 1.1e+00 | MPI (non jugé) | MPI | r8 arrondi : MPI |
| Post | extractSlicesPT_t1.py | écart L0 1.1e+01 |  | plantage | r8 arrondi : plantage |
| Post | streamLine2PT_m1.py | écart L0 4.8e-01 | MPI (non jugé) | MPI | r8 arrondi : MPI |

## environnement (16)

| Module | Test | Écart (validCassiopee) | Comparaison géométrique | r8 arrondi | Raison |
|---|---|---|---|---|---|
| CPlot | displayOSMesa_m1.py | autre DIFF |  | MPI | environnement : rendu OSMesa du conteneur |
| CPlot | displayOSMesa_m2.py | autre DIFF |  | MPI | environnement : rendu OSMesa du conteneur |
| CPlot | displayOSMesa_t1.py | autre DIFF |  | échoue | environnement : rendu OSMesa du conteneur |
| CPlot | displayOSMesa_t2.py | autre DIFF |  | passe | environnement : rendu OSMesa du conteneur |
| CPlot | displayOSMesa_t3.py | autre DIFF |  | échoue | environnement : rendu OSMesa du conteneur |
| CPlot | tkPlotXY_m1.py | plantage |  | MPI | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getAllPosPT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getEdgeListOfFacePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getEdgePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getEdgePosInFacePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getEdgeRangeOfFacePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getEdgeVerticesOfFacePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getFaceListOfEdgePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getFaceNoOppOfEdgePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getFacePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
| OCC | getNPtsOnEdgesOfFacePT_t1.py | exception TypeError |  | plantage | tkinter absent ou cube.cgns produit par un autre test |
