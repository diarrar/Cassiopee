# - extractPoint : maillage cartesien -
import Converter as C
import Generator as G
import Post as P
import KCore
# values are computed in the compiled real precision: comparison tolerance and
# margin on the interpolation error bounds (unchanged in double precision)
TOLVAL = 1.e-10 if KCore.getRealSize() == 8 else 1.e-6
ERRMARGIN = 0. if KCore.getRealSize() == 8 else 1.e-6
#
# Maillage en noeuds
ni = 11; nj = 11; nk = 11;
a = G.cartTetra((0,0,0), (1./(ni-1),1./(nj-1),1./(nk-1)), (ni,nj,nk))

# Create a function
def F(x,y,z):
    return 2*x*x*x*x*x + 2.*y*y*z + z*z

(x,y,z) = (0.55,0.38,0.12)
val0 = F(x,y,z)

# init by function
a = C.initVars(a, 'F', F, ['x','y','z'], isVectorized=True)

cnt = 0
val = P.extractPoint([a], (x,y,z), 3)
err = 0.010897126
if abs(val[0]-val0) > err+ERRMARGIN:
    print('DIFF: reference: '+str(err)+'.')
    print('DIFF: courant: '+str(val[0]-val0)+'.')
