# - bcastZone (MPI) -
import numpy
from Converter import Internal
from Converter import Mpi4py as Mpi

assert Mpi.size >= 2
root = 1
real = Internal.E_NpyFloat
otherReal = numpy.float64 if real == numpy.float32 else numpy.float32
x = numpy.arange(24, dtype=otherReal).reshape((4, 6))[::2, ::2]
y = numpy.arange(6, dtype=real).reshape((2, 3), order='F') + 10
z = numpy.zeros((2, 3), dtype=real)
nodePressure = x + 20
cellPressure = numpy.array([[101., 102.]], dtype=otherReal)
zone = None
if Mpi.rank == root:
    zone = Internal.newZone('Zone', zsize=[[2, 1, 0], [3, 2, 0]])
    grid = Internal.newGridCoordinates(parent=zone)
    xNode = Internal.newDataArray('CoordinateX', x, parent=grid)
    xNode[1] = x  # Exercise a non-contiguous incoming NumPy view.
    Internal.newDataArray('CoordinateY', y, parent=grid)
    Internal.newDataArray('CoordinateZ', z, parent=grid)
    nodes = Internal.newFlowSolution(parent=zone)
    Internal.newDataArray('Pressure', nodePressure, parent=nodes)
    Internal.newDataArray('Unused', z, parent=nodes)
    centers = Internal.newFlowSolution(Internal.__FlowSolutionCenters__, 'CellCenter', parent=zone)
    centerNode = Internal.newDataArray('Pressure', cellPressure, parent=centers)

received = Mpi.bcastZone(zone, root=root, variables=['centers:Pressure', 'nodes:Pressure'])
for containerName, name, expected in (
    (Internal.__GridCoordinates__, 'CoordinateX', x),
    (Internal.__GridCoordinates__, 'CoordinateY', y),
    (Internal.__GridCoordinates__, 'CoordinateZ', z),
    (Internal.__FlowSolutionNodes__, 'Pressure', nodePressure),
    (Internal.__FlowSolutionCenters__, 'Pressure', cellPressure),
):
    container = Internal.getNodeFromName1(received, containerName)
    actual = Internal.getNodeFromName1(container, name)[1]
    assert actual.dtype == real
    assert actual.flags.f_contiguous
    numpy.testing.assert_array_equal(actual, expected)
assert Internal.getNodeFromName2(received, 'Unused') is None
centers = Internal.getNodeFromName1(received, Internal.__FlowSolutionCenters__)
assert Internal.getValue(Internal.getNodeFromName1(centers, 'GridLocation')) == 'CellCenter'
if Mpi.rank == root:
    assert xNode[1] is x
    assert centerNode[1].dtype == otherReal
    assert Internal.getNodeFromName2(zone, 'Unused') is not None

centersOnly = Mpi.bcastZone(zone, root=root, coord=False, variables=['centers:Pressure'])
assert Internal.getNodeFromName1(centersOnly, Internal.__GridCoordinates__) is None
assert Internal.getNodeFromName1(centersOnly, Internal.__FlowSolutionNodes__) is None
centers = Internal.getNodeFromName1(centersOnly, Internal.__FlowSolutionCenters__)
assert Internal.getValue(Internal.getNodeFromName1(centers, 'GridLocation')) == 'CellCenter'

try:
    Mpi.bcastZone(zone, root=root, variables=['Missing'])
except ValueError as error:
    assert 'Missing' in str(error)
else:
    raise AssertionError('Missing field should raise an error on every rank')

if Mpi.rank == root:
    Internal.setName(grid, 'GridCoordinates#Deformed')
deformed = Mpi.bcastZone(zone, root=root)
grid = Internal.getNodeFromType1(deformed, 'GridCoordinates_t')
numpy.testing.assert_array_equal(Internal.getNodeFromName1(grid, 'CoordinateX')[1], x)
