"""Run with mpiexec -n 2 python precision_m2.py in each precision."""
from mpi4py import MPI
import numpy
import KCore
import Converter.Internal as Internal
import Generator as G
from XCore import xcore

comm = MPI.COMM_WORLD
if comm.size != 2:
    raise RuntimeError('precision_m2.py requires exactly two MPI ranks')

# One hexahedral cell per rank; one interface face and one real cell field.
mesh = G.cartNGon((float(comm.rank), 0., 0.), (1., 1., 1.), (2, 2, 2), api=3)
parents = numpy.zeros((6, 2), dtype=Internal.E_NpyInt, order='F')
parents[:, 0] = 1
field = numpy.array([comm.rank+1.25], dtype=KCore.E_NpyFloat)
interface = [[1-comm.rank, numpy.array([1], dtype=Internal.E_NpyInt)]]
received = xcore.exchangeFields(mesh, parents, [field], interface)
assert len(received) == 1 and len(received[0]) == 1
assert received[0][0].dtype == KCore.E_NpyFloat
numpy.testing.assert_array_equal(received[0][0], [1-comm.rank+1.25])
comm.Barrier()
if comm.rank == 0:
    print('XCore.exchangeFields:', KCore.REAL_DTYPE, 'two-rank exchange passed')
