"""Real-valued NGon operations in the selected precision."""
import unittest
import KCore
import Generator as G
import Intersector as X


class PrecisionTests(unittest.TestCase):
    def test_cube_volume(self):
        a = G.cartNGon((0, 0, 0), (1, 1, 1), (2, 2, 2))
        self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
        self.assertAlmostEqual(X.volume(a), 1., places=6)


if __name__ == '__main__':
    unittest.main()
