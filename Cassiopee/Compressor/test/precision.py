"""Compression of real fields in the installed precision."""
import unittest
import numpy
import KCore
from Compressor import compressor, sz, zfp
import Compressor.PyTree as X
import Converter.Internal as Internal


class PrecisionTests(unittest.TestCase):
    def test_fpc(self):
        for dtype in (numpy.float32, numpy.float64):
            for order in ('C', 'F'):
                with self.subTest(dtype=dtype, order=order):
                    a = numpy.array(numpy.arange(32).reshape(4, 8)/4, dtype=dtype, order=order)
                    b = compressor.uncompressFpc(compressor.compressFpc(a))
                    self.assertEqual(b.dtype, KCore.E_NpyFloat)
                    self.assertEqual(b.flags.f_contiguous, a.flags.f_contiguous)
                    numpy.testing.assert_array_equal(a, b)

    def test_celln(self):
        a = (numpy.arange(96).reshape(12, 8) % 3).astype(KCore.E_NpyFloat)
        b = compressor.uncompressCellN(compressor.compressCellN(a))
        self.assertEqual(b.dtype, KCore.E_NpyFloat)
        numpy.testing.assert_array_equal(a, b)

        other = numpy.float64 if KCore.REAL_SIZE == 4 else numpy.float32
        with self.assertRaises(TypeError):
            compressor.compressCellN(a.astype(other))

    def test_zfp(self):
        for dtype in (numpy.float32, numpy.float64):
            with self.subTest(dtype=dtype):
                a = numpy.arange(32, dtype=dtype).reshape(4, 8)
                b = zfp.unpack(zfp.pack(a, reversible=True), reversible=True)
                self.assertEqual(b.dtype, KCore.E_NpyFloat)
                numpy.testing.assert_array_equal(a, b)

    def test_sz(self):
        for dtype in (numpy.float32, numpy.float64):
            with self.subTest(dtype=dtype):
                a = numpy.arange(128, dtype=dtype).reshape(16, 8)
                b = sz.unpack(sz.pack(a, {}), {})
                self.assertEqual(b.dtype, KCore.E_NpyFloat)
                numpy.testing.assert_allclose(a, b, rtol=1.e-5, atol=1.e-5)

    def test_pytree_metadata(self):
        a = numpy.array(numpy.arange(96).reshape(12, 8) % 3, dtype=KCore.E_NpyFloat, order='F')
        for codec in (0, 1, 2, 5):
            with self.subTest(codec=codec):
                node = Internal.newDataArray('Density', a.copy(order='F'))
                X._packNode(node, ctype=codec)
                self.assertEqual(Internal.getNodeFromName(node, 'ZData')[1].dtype, numpy.float64)
                X._unpackNode(node)
                self.assertEqual(node[1].dtype, KCore.E_NpyFloat)
                self.assertTrue(node[1].flags.f_contiguous)
                numpy.testing.assert_allclose(node[1], a, rtol=1.e-5, atol=1.e-5)


if __name__ == '__main__':
    unittest.main()
