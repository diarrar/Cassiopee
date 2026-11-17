"""Core module for Cassiopee."""
from .kcore import *

__version__ = "2026.11"

# Compiled real precision, asked to the binary (4: single precision, 8: double)
E_REALSIZE = getRealSize()

# Default geometric tolerance of the python APIs, scaled with the compiled
# real precision: values below the resolution of float are meaningless in r4.
if E_REALSIZE == 4:
    def realTol(tol): return max(tol, 1.e-6)
else:
    def realTol(tol): return tol

# Cast a real numpy to another real dtype (references or files written in
# double read by a single precision build). Finite values beyond the range of
# the target type (sentinels such as DBL_MAX, E_MAX_FLOAT in double) become the
# largest finite value of the target type (E_MAX_FLOAT in single precision)
# instead of inf; inf and nan are kept.
def castFloatArray(v, dtype):
    import numpy
    with numpy.errstate(over='ignore'):
        w = v.astype(dtype, order='K')
    if numpy.dtype(dtype).itemsize < v.dtype.itemsize:
        over = numpy.isfinite(v) & ~numpy.isfinite(w)
        if over.any():
            big = numpy.finfo(dtype).max
            w[over] = numpy.where(v[over] > 0, big, -big)
    return w
