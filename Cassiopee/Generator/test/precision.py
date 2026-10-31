"""Numerical and external-library precision boundaries (default build)."""
import unittest
import numpy
import KCore
import Converter as C
import Generator as G
import Transform as T


def tetra_surface():
    a = C.array('x,y,z', 4, 4, 'TRI')
    a[1][...] = [[0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
    a[2][...] = numpy.array([[1, 3, 2], [1, 2, 4], [2, 3, 4], [3, 1, 4]]).T
    return a


class PrecisionTests(unittest.TestCase):
    def test_cart_and_translation(self):
        for api in (1, 3):
            with self.subTest(api=api):
                a = G.cart((1., 2., 3.), (.5, .25, .125), (4, 3, 2), api=api)
                b = T.translate(a, (.125, .25, .5))
                for i, shift in enumerate((.125, .25, .5)):
                    self.assertEqual(b[1][i].dtype, KCore.E_NpyFloat)
                    numpy.testing.assert_allclose(b[1][i], a[1][i]+shift)

    def test_volume(self):
        a = G.cart((1., 2., 3.), (.5, .25, .125), (4, 3, 2))
        v = G.getVolumeMap(a)
        self.assertEqual(v[1].dtype, KCore.E_NpyFloat)
        numpy.testing.assert_allclose(v[1], .5*.25*.125)

    def test_tetgen_conversion(self):
        a = G.generator.tetgen(tetra_surface(), 1., 1.2, 0, [], 'pq')
        self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
        self.assertEqual(a[3], 'TETRA')
        self.assertGreaterEqual(C.getNPts(a), 4)
        volume = G.getVolumeMap(a)[1].sum()
        self.assertAlmostEqual(volume, 1./6, places=6)

    def test_mmgs_conversion(self):
        a = G.mmgs(tetra_surface(), hmin=.2, hmax=.4, optim=1)
        self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
        self.assertEqual(a[3], 'TRI')
        self.assertGreaterEqual(C.getNPts(a), 4)
        self.assertTrue(numpy.isfinite(a[1]).all())


if __name__ == '__main__':
    unittest.main()
