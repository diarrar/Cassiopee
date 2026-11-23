# - FV IBM boundary face and point ordering -
import numpy

import Connector.AMR as AMR
import Converter as Converter
import Converter.Internal as Internal
import Converter.PyTree as C
import Generator.PyTree as G

zone = G.cartHexa((0., 0., 0.), (1., 1., 1.), (7, 2, 2))
vertices = C.getFields(Internal.__GridCoordinates__, zone)[0][1]
quads = Converter.array('x,y,z', vertices.shape[1], 6, 'QUAD')
quads[1][:] = vertices
quads[2][:] = numpy.arange(6) + numpy.array([[1], [2], [9], [8]])
exterior = C.convertArrays2ZoneNode('Exterior', [quads])
tree = C.newPyTree(['Base', zone])

facesByWall = {
    'IBMWall0': numpy.array([2, 0, 4], dtype=Internal.E_NpyInt),
    'IBMWall1': numpy.empty(0, dtype=Internal.E_NpyInt),
    'IBMWall2': numpy.array([3], dtype=Internal.E_NpyInt),
}

def points(indices, offset):
    if indices.size == 0:
        return [[]]
    array = Converter.array('CoordinateX,CoordinateY,CoordinateZ', indices.size, 0, 'NODE')
    array[1][:] = [indices + 0.5, indices + offset, indices - offset]
    return [array]

donor = {name: points(indices, 10.) for name, indices in facesByWall.items()}
wall = {name: points(indices, 20.) for name, indices in facesByWall.items()}
AMR._addIBCDatasets__(tree, exterior, donor, wall, None,
                     {'spatial discretization': {'type': 'FV'}}, facesByWall)

boundaries = Internal.getNodesFromType(tree, 'BC_t')
assert [node[0] for node in boundaries] == ['IBMWall0', 'IBMWall2']
for bc in boundaries:
    expected = facesByWall[bc[0]]
    first, last = Internal.getNodeFromName1(bc, 'ElementRange')[1].ravel()
    assert last - first + 1 == expected.size
    section = Internal.getNodeFromName1(zone, bc[0])
    connectivity = Internal.getNodeFromName1(section, 'ElementConnectivity')[1].reshape(-1, 4)
    numpy.testing.assert_array_equal(vertices[0, connectivity-1].mean(axis=1), expected + 0.5)
    for name, values in (('DonorPointCoordinates', donor), ('WallPointCoordinates', wall)):
        coordinates = Internal.getNodeFromName(bc, name)
        for axis, row in zip('XYZ', values[bc[0]][0][1]):
            actual = Internal.getNodeFromName1(coordinates, 'Coordinate' + axis)[1]
            numpy.testing.assert_array_equal(actual, row)
