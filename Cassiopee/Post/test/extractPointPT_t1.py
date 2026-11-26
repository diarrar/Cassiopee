# - extractPoint (pyTree) -
import Converter.PyTree as C
import Generator.PyTree as G
import Post.PyTree as P
import KCore.test as test
import KCore
# values are computed in the compiled real precision: comparison tolerance and
# margin on the interpolation error bounds (unchanged in double precision)
TOLVAL = 1.e-10 if KCore.getRealSize() == 8 else 1.e-6
ERRMARGIN = 0. if KCore.getRealSize() == 8 else 1.e-6

# Maillage en noeuds
ni = 10; nj = 10; nk = 10;
a = G.cart((0,0,0), (1./(ni-1),1./(nj-1),1./(nk-1)), (ni,nj,nk))

# Create a function
def F(x,y,z): return 2*x*x*x*x*x + 2.*y*y*z + z*z

# Init by function
a = C.initVars(a, 'F', F, ['CoordinateX','CoordinateY','CoordinateZ'], isVectorized=True)
a = C.initVars(a, 'centers:G', 1.)

# 2nd order
val = P.extractPoint(a, (0.55, 0.38, 0.12), 2)
ref = [0.15309556470050301, 1.0]
print("Test1... done.")
for i in range(len(val)):
    if abs(val[i]-ref[i]) > TOLVAL:
        print('DIFF: reference: '+str(ref[i])+'.')
        print('DIFF: courant: '+str(val[i])+'.')

# 3rd order
val = P.extractPoint(a, (0.55, 0.38, 0.12), 3)
ref = [0.1502804068485496, 1.0]
print("Test2... done.")
for i in range(len(val)):
    if abs(val[i]-ref[i]) > TOLVAL:
        print('DIFF: reference: '+str(ref[i])+'.')
        print('DIFF: courant: '+str(val[i])+'.')

# 5th order
val = P.extractPoint(a, (0.55, 0.38, 0.12), 5)
ref = [0.14970315744608428, 1.0000000000061293]
print("Test3... done.")
for i in range(len(val)):
    if abs(val[i]-ref[i]) > TOLVAL:
        print('DIFF: reference: '+str(ref[i])+'.')
        print('DIFF: courant: '+str(val[i])+'.')

# Liste de points
val = P.extractPoint(a, [(0.55, 0.38, 0.12), (0.3,0.3,0.3)], 2)
ref = [[0.15309556470050301, 1.0], [0.15505766397398768, 1.0000000000000002]]
print("Test4... done.")
for i in range(len(val)):
    for j in range(len(val[i])):
        if abs(val[i][j]-ref[i][j]) > TOLVAL:
            print('DIFF: reference: '+str(ref[i])+'.')
            print('DIFF: courant: '+str(val[i])+'.')
