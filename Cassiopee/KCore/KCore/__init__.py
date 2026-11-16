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
