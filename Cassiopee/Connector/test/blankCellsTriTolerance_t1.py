# - Node blanking near a triangular mask -
import numpy

import Connector as X
import Converter as C
import Geom as D
import KCore
import Transform as T


def blank(body, coordinates):
    count = len(coordinates)
    points = C.array('x,y,z', count, 0, 'NODE')
    points[1][:] = numpy.asarray(coordinates).T
    active = C.array('cellN', count, 0, 'NODE')
    active[1][:] = 1.
    return X.blankCellsTri([points], [active], body, blankingType=0)[0][1][0]


# The bounding box must admit points within the vertex contact tolerance.
tetra = C.array('x,y,z', 4, 4, 'TRI')
tetra[1][:] = [[0., 1., 0., 0.], [0., 0., 1., 0.], [0., 0., 0., 1.]]
tetra[2][:] = numpy.array([[1, 3, 2], [1, 2, 4], [1, 4, 3], [2, 3, 4]]).T
tol = KCore.realTol(1.e-12)
numpy.testing.assert_array_equal(
    blank(tetra, [(-0.25 * tol, 0., 0.), (0.1, 0.1, 0.1), (-2. * tol, 0., 0.), (0., 0., 0.)]),
    [0., 0., 1., 0.])

# All three points are inside the bounding box, but only the middle one is inside the airfoil.
airfoil = C.convertArray2Tetra(T.addkplane(D.naca('0012', N=501)))
numpy.testing.assert_array_equal(
    blank(airfoil, [(0.9280001, 0.01, 0.), (0.9280001, 0.00999, 0.), (0.9280001, 0.0101, 0.)]),
    [1., 0., 1.])

# A ray to a face centroid can be coplanar with the triangle on an extruded front.
contour = C.array('x,y,z', 7, 1, 1)
contour[1][0] = [0.024, 0.028, 0.028, 0.026, 0.026, 0.024, 0.024]
contour[1][1] = [0.02, 0.02, 0.022, 0.022, 0.028, 0.028, 0.02]
contour[1][2] = 0.
front = C.convertArray2Tetra(T.addkplane(contour))
numpy.testing.assert_array_equal(
    blank(front, [(0.0265, 0.028, 0.5), (0.0265, 0.0215, 0.5), (0.026, 0.028, 0.)]),
    [1., 0., 0.])
