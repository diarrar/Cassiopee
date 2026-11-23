# - Thin X-Ray mask and coincident hits -
import Connector as X
from Connector import connector
import Converter as C
import Geom as D
import numpy

# Preserve both intersections of a thin body, also far from the origin.
thickness = 0.125
for offset in (0., 1.e6):
    top = offset + thickness
    rectangle = D.polyline([(0., offset, 0.), (1., offset, 0.),
                            (1., top, 0.), (0., top, 0.), (0., offset, 0.)])
    body = C.convertArray2Tetra(rectangle)
    coords = X.maskXRay__([body], dim=2, tol=1.e-8)[1]
    # Pass both bodies directly to test duplicate hits at the ray stage.
    repeated = connector.maskXRay([body, body], 0., 2, 0, 1.e-8)[1]
    assert coords.shape == repeated.shape
    for hits in (coords, repeated):
        assert hits.shape[1] > 0 and hits.shape[1] % 2 == 0
        numpy.testing.assert_array_equal(hits[0, 0::2], hits[0, 1::2])
        assert numpy.all(hits[1, 0::2] == offset)
        assert numpy.all(hits[1, 1::2] == top)
