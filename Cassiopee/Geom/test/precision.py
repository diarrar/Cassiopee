"""Geometry allocations and xatlas boundary in the compiled real type."""
import unittest
import numpy
import KCore
import Geom as D
import Generator as G


class PrecisionTests(unittest.TestCase):
    def test_naca(self):
        a = D.naca(12., N=51)
        self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
        self.assertTrue(numpy.isfinite(a[1]).all())
        self.assertAlmostEqual(float(a[1][0].min()), 0., places=6)
        self.assertAlmostEqual(float(a[1][0].max()), 1., places=6)

    def test_uv(self):
        arrays = D.getUV(G.cartTetra((0, 0, 0), (1, 1, 1), (3, 3, 1)), texResolution=16)
        self.assertGreater(len(arrays), 0)
        for a in arrays:
            self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
            self.assertTrue(numpy.isfinite(a[1]).all())
        numpy.testing.assert_allclose(arrays[0][1][:2].max(axis=1), [2., 2.])


if __name__ == '__main__':
    unittest.main()
