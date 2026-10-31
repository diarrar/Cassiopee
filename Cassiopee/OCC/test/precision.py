"""OpenCascade binary64 SDK to Cassiopee real-field conversions."""
from pathlib import Path
import unittest
import numpy
import KCore
import OCC


class PrecisionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hook = OCC.readCAD(str(Path(__file__).with_name('cube.step')))

    @classmethod
    def tearDownClass(cls):
        OCC.freeHook(cls.hook)

    def test_bounds(self):
        self.assertEqual(OCC.getNbFaces(self.hook), 6)
        self.assertEqual(OCC.getNbEdges(self.hook), 12)
        bounds = OCC.getBoundingBox(self.hook)
        self.assertEqual(len(bounds), 6)
        self.assertTrue(numpy.isfinite(bounds).all())
        self.assertTrue((numpy.array(bounds[:3]) < bounds[3:]).all())

    def test_edge_meshes(self):
        edges = OCC.meshAllEdgesOCC(self.hook, .1)
        self.assertEqual(len(edges), 12)
        for a in edges:
            self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
            self.assertTrue(numpy.isfinite(a[1]).all())
            self.assertGreaterEqual(a[1].shape[1], 2)


if __name__ == '__main__':
    unittest.main()
