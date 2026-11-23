# - getCircumCircleMap (array) -
import Geom as D
import Generator as G
import Converter as C
import KCore.test as test
import numpy

# The squared area from the side lengths can round negative in single precision.
x, y = 0.0101, 0.00001
triangle = C.array('x,y,z', 3, 1, 'TRI')
triangle[1][:] = [[0., 1., x], [0., 0., y], [0., 0., 0.]]
triangle[2][:, 0] = [1, 2, 3]
radius = G.getCircumCircleMap(triangle)[1][0, 0]
expected = numpy.hypot(x, y) * numpy.hypot(1. - x, y) / (2. * y)
numpy.testing.assert_allclose(radius, expected, rtol=2.e-6)

a = D.sphere((0,0,0), 1, 50)
a = C.convertArray2Tetra(a)
n = G.getCircumCircleMap(a)
assert numpy.isfinite(n[1]).all()
test.testA([n],1)
