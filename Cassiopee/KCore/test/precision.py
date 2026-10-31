"""Run against each installed precision: python precision.py."""
import ctypes
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import numpy
import KCore
import KCore.kcore
import KCore.Dist as Dist


class PrecisionTests(unittest.TestCase):
    def test_compiled_dtype(self):
        self.assertIn(KCore.REAL_SIZE, (4, 8))
        self.assertEqual(numpy.dtype(KCore.E_NpyFloat).itemsize, KCore.REAL_SIZE)
        self.assertEqual(numpy.dtype(KCore.REAL_DTYPE), KCore.E_NpyFloat)
        # A runtime environment variable cannot change the installed ABI.
        with patch.dict(os.environ, {"CASSIOPEE_PRECISION": "single"}):
            self.assertEqual(KCore.empty(7, 64).dtype, KCore.E_NpyFloat)

    def test_aligned_allocations(self):
        for shape in ((), (0, 4), (19,), (7, 3), (2, 3, 4)):
            for alignment in (0, 16, 32, 64, 128):
                with self.subTest(shape=shape, alignment=alignment):
                    a = KCore.empty(shape, alignment)
                    self.assertEqual(a.dtype, KCore.E_NpyFloat)
                    self.assertEqual(a.shape, shape)
                    self.assertTrue(a.flags.f_contiguous)
                    self.assertEqual(a.ctypes.data % max(alignment, KCore.REAL_SIZE), 0)
                    a[...] = 1.25
                    numpy.testing.assert_array_equal(a, numpy.full(shape, 1.25))

    def test_allocation_lifetime(self):
        a = KCore.empty((13, 7), 64)
        a[...] = numpy.arange(91).reshape(a.shape)
        view = a[:, 2:5]
        del a
        numpy.testing.assert_array_equal(view, numpy.arange(91).reshape(13, 7)[:, 2:5])

    def test_invalid_allocation(self):
        for shape, alignment in ((-1, 64), ((2, -1), 64), ((3,), -1), ((3,), 3)):
            with self.subTest(shape=shape, alignment=alignment):
                with self.assertRaises(ValueError):
                    KCore.empty(shape, alignment)

    def test_openmp(self):
        old = KCore.getOmpMaxThreads()
        try:
            KCore.setOmpMaxThreads(2)
            self.assertEqual(KCore.getOmpMaxThreads(), 2)
        finally:
            KCore.setOmpMaxThreads(old)

    @unittest.skipIf(os.name == 'nt', "Fortran symbol names differ on Windows")
    def test_fortran_real_abi(self):
        # Exercise a real Fortran kernel with the compiled C++/NumPy real type.
        # The extension hides Fortran symbols; link the installed static library
        # into a temporary probe, retaining exactly the kernel under test.
        with tempfile.TemporaryDirectory() as tmp:
            target = str(Path(tmp) / 'kernel.so')
            _, libs, paths = Dist.checkFortranLibs()
            command = [Dist.getDistUtilsCompilers()[0], '-shared', '-fPIC',
                       '-Wl,-u,k6compstructcellcenter_',
                       str(Path(Dist.getLibPath()) / 'libkcore.a')]
            command += ['-L'+p for p in paths] + ['-l'+name for name in libs]
            subprocess.run(command + ['-o', target], check=True, capture_output=True)
            lib = ctypes.CDLL(target)
            kernel = lib.k6compstructcellcenter_
        kernel.restype = None
        integer = ctypes.c_int64 if "_i8" in os.getenv("ELSAPROD", "") else ctypes.c_int
        real = ctypes.c_float if KCore.REAL_SIZE == 4 else ctypes.c_double
        iptr, rptr = ctypes.POINTER(integer), ctypes.POINTER(real)
        kernel.argtypes = [iptr]*5 + [rptr]*4
        dims = [integer(n) for n in (2, 2, 2, 8, 1)]
        xyz = numpy.array([[1, 3, 1, 3, 1, 3, 1, 3],
                           [2, 2, 6, 6, 2, 2, 6, 6],
                           [4, 4, 4, 4, 8, 8, 8, 8]], dtype=KCore.E_NpyFloat)
        out = numpy.full(3, -999, dtype=KCore.E_NpyFloat)
        kernel(*[ctypes.byref(n) for n in dims],
               *[a.ctypes.data_as(rptr) for a in xyz], out.ctypes.data_as(rptr))
        numpy.testing.assert_array_equal(out, [2, 4, 6])


if __name__ == "__main__":
    unittest.main()
