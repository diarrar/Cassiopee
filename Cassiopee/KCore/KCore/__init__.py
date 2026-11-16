"""Core module for Cassiopee."""
from .kcore import *
from .Dist import ESINGLEREAL

__version__ = "2026.11"

# Default geometric tolerance of the python APIs, scaled with the compiled
# real precision: values below the resolution of float are meaningless in r4.
if ESINGLEREAL():
    def realTol(tol): return max(tol, 1.e-6)
else:
    def realTol(tol): return tol
