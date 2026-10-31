"""Check field operations and the Stream2 matrix/vector real ABI."""
import unittest
import numpy
import KCore
import Converter as C
import Generator as G
import Post as P


class PrecisionTests(unittest.TestCase):
    def test_gradient(self):
        a = G.cart((0., 0., 0.), (.1, .1, .1), (6, 6, 6))
        a = C.initVars(a, '{f}=2*{x}+3*{y}+4*{z}')
        grad = P.computeGrad(a, 'f')
        self.assertEqual(grad[1].dtype, KCore.E_NpyFloat)
        expected = numpy.array([[2.], [3.], [4.]])+numpy.zeros_like(grad[1])
        tolerance = 256*numpy.finfo(KCore.E_NpyFloat).eps
        numpy.testing.assert_allclose(grad[1], expected, rtol=tolerance, atol=tolerance)

    def test_stream2(self):
        a = G.cart((0., 0., 0.), (.1, .1, .1), (6, 6, 6))
        for name, value in [('u', 1.), ('v', 0.), ('w', 0.)]:
            a = C.initVars(a, name, value)
        lines = P.streamLine2([a], (.1, .2, .2), ['u', 'v', 'w'], N=10, dir=1)
        self.assertGreater(len(lines), 0)
        for line in lines:
            self.assertEqual(line[1].dtype, KCore.E_NpyFloat)
            self.assertGreaterEqual(C.getNPts(line), 2)
            self.assertTrue(numpy.isfinite(line[1]).all())
            numpy.testing.assert_allclose(line[1][1:3], .2, atol=1.e-6)


if __name__ == '__main__':
    unittest.main()
