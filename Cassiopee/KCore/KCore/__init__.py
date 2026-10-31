"""Core module for Cassiopee."""
from .kcore import *
import numpy as _numpy

# These describe the compiled extension, independently of the current shell.
E_NpyFloat = _numpy.dtype(REAL_DTYPE).type

__version__ = "2026.11"
