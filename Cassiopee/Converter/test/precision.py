"""Precision integration tests; run separately with FP32 and FP64 installs."""
from pathlib import Path
import tempfile
import unittest
import warnings

import numpy
import KCore
import Converter as C
import Converter.Internal as Internal
import Converter.PyTree as CP


def cart(api=1):
    array = C.array('x,y,z', 3, 2, 2, api=api)
    fields = [a.ravel(order='K') for a in array[1]] if api != 1 else array[1]
    for axis, coordinates in enumerate(numpy.indices((3, 2, 2))):
        fields[axis][...] = coordinates.ravel(order='F')
    return array


class PrecisionTests(unittest.TestCase):
    def test_array_allocations_and_expressions(self):
        for api in (1, 2, 3):
            with self.subTest(api=api):
                a = C.initVars(cart(api), '{Density}=2*{x}+{y}')
                for field in a[1]:
                    self.assertEqual(field.dtype, KCore.E_NpyFloat)
                fields = [v.ravel(order='K') for v in a[1]]
                numpy.testing.assert_allclose(fields[3], 2*fields[0]+fields[1])

    def test_node_to_center_fortran(self):
        a = C.node2Center(cart())
        self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
        numpy.testing.assert_allclose(a[1], [[0.5, 1.5], [0.5, 0.5], [0.5, 0.5]])

    def test_incompatible_array_dtype(self):
        for api in (1, 3):
            a = cart(api)
            other = numpy.float64 if KCore.REAL_SIZE == 4 else numpy.float32
            a[1] = a[1].astype(other) if api == 1 else [v.astype(other) for v in a[1]]
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', RuntimeWarning)
                with self.assertRaises((TypeError, ValueError)):
                    C.getMaxValue(a, 'x')

    def test_pytree_allocations(self):
        node = Internal.newDataArray('Density', [1.25, 2.5, 5.0])
        self.assertEqual(node[1].dtype, KCore.E_NpyFloat)
        zone = CP.convertArrays2ZoneNode('cart', [cart(3)])
        zone = CP.initVars(zone, '{Density}=2*{CoordinateX}+{CoordinateY}')
        density = Internal.getNodeFromName(zone, 'Density')[1]
        self.assertEqual(density.dtype, KCore.E_NpyFloat)
        numpy.testing.assert_allclose(density.ravel(order='F'), 2*cart()[1][0]+cart()[1][1])

    def test_cgns_interchange(self):
        zone = CP.convertArrays2ZoneNode('cart', [C.initVars(cart(), '{Density}=2*{x}+{y}')])
        for stored_type in (numpy.float32, numpy.float64):
            for format in ('bin_hdf', 'bin_adf'):
                with self.subTest(stored_type=stored_type, format=format), tempfile.TemporaryDirectory() as tmp:
                    tree = CP.newPyTree(['Base', Internal.copyTree(zone)])
                    Internal._adaptTypes(tree, convertR42R8=stored_type == numpy.float64,
                                         convertR82R4=stored_type == numpy.float32)
                    path = str(Path(tmp) / 'mesh.cgns')
                    CP.convertPyTree2File(tree, path, format)
                    read = CP.convertFile2PyTree(path, format)
                    density = Internal.getNodeFromName(read, 'Density')[1]
                    self.assertEqual(density.dtype, KCore.E_NpyFloat)
                    self.assertTrue(density.flags.f_contiguous)
                    expected = 2*cart()[1][0] + cart()[1][1]
                    numpy.testing.assert_allclose(density.ravel(order='F'), expected)

    def test_codec_metadata_precision(self):
        for format in ('bin_hdf', 'bin_adf'):
            with self.subTest(format=format), tempfile.TemporaryDirectory() as tmp:
                tree = CP.newPyTree(['Base'])
                metadata = Internal.createUniqueChild(Internal.getBases(tree)[0],
                                                       'CodecMetadata', 'UserDefinedData_t')
                values = numpy.array([5., 0., 1., 2**24+1], dtype=numpy.float64)
                Internal.newDataArray('ZData', values, parent=metadata)
                path = str(Path(tmp) / 'metadata.cgns')
                CP.convertPyTree2File(tree, path, format)
                read = CP.convertFile2PyTree(path, format)
                data = Internal.getNodeFromName(read, 'ZData')[1]
                self.assertEqual(data.dtype, numpy.float64)
                numpy.testing.assert_array_equal(data, values)

    def test_tecplot_roundtrip(self):
        for format in ('fmt_tp', 'bin_tp'):
            with self.subTest(format=format), tempfile.TemporaryDirectory() as tmp:
                path = str(Path(tmp) / 'mesh.plt')
                C.convertArrays2File([cart()], path, format)
                a = C.convertFile2Arrays(path, format)[0]
                self.assertEqual(a[1].dtype, KCore.E_NpyFloat)
                numpy.testing.assert_allclose(a[1], cart()[1])


if __name__ == '__main__':
    unittest.main()
