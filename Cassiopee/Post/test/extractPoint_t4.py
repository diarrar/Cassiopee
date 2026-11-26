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
# extrapolation
# Maillage en noeuds
ni = 11; nj = 11; nk = 11;
a = G.cart((0,0,0), (1./(ni-1),1./(nj-1),1./(nk-1)), (ni,nj,nk))

eps = 1.e-4
(x,y,z) = (1-eps,1.+eps,1+eps)
val0 =  (1-eps)*(1.+eps)*(1+eps)

# init by function
a = C.initVars(a, '{F}={x}*{y}*{z}')

extrapOrders = [0,1]; constraints = [0,40.]
sol = [0.,0.99990000000000001,0.,1.0000852499999999]
err = [1.00009999,0.000199989999,1.00009999,1.4739999e-05]
cnt = 0
for extrapOrder in extrapOrders:
    for constraint in constraints:
        val = P.extractPoint([a], (x,y,z), 2, extrapOrder, constraint)
        if abs(val[0]-val0) > err[cnt]+1e-12+ERRMARGIN:
            print('DIFF: reference: '+str(err[cnt])+'.')
            print('DIFF: courant: '+str(abs(val[0]-val0))+'.')
        cnt += 1
