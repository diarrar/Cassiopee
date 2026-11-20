# - convertFile2Arrays (binary Plot3D, endian and real width) -
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

import numpy
import Converter as C
import Converter.Internal as Internal
from Converter import converter


coordinates = ((1.0, 1.5), (-2.0, 2.5), (3.0, -3.5))
conservative = ((4.0, 4.5), (5.0, -5.5), (-6.0, 6.5), (7.0, 7.5), (8.0, 8.5))
functions = ((9.0, -9.5), (-10.0, 10.5))
iblank = (1, 0)


def write_plot3d(path, endian, real_format, int_format='i', blank=False):
    def record(payload):
        marker = struct.pack(endian + 'i', len(payload))
        return marker + payload + marker

    dims = (2, 1, 1)
    data = record(struct.pack(endian + int_format, 1))
    if path.suffix == '.fbin':
        dims += (len(functions),)
    data += record(struct.pack(endian + int_format * len(dims), *dims))

    if path.suffix == '.gbin':
        fields = coordinates
    elif path.suffix == '.qbin':
        data += record(struct.pack(endian + real_format * 4, 0.5, 0.0, 1.0, 0.0))
        fields = conservative
    else:
        fields = functions
    values = [value for field in fields for value in field]
    payload = struct.pack(endian + real_format * len(values), *values)
    if blank:
        payload += struct.pack(endian + int_format * len(iblank), *iblank)
    path.write_bytes(data + record(payload))


def check_arrays(arrays, names, expected):
    array, = arrays
    assert array[0] == names, array[0]
    assert array[2:] == [2, 1, 1], array[2:]
    assert array[1].dtype == Internal.E_NpyFloat, array[1].dtype
    numpy.testing.assert_array_equal(array[1], expected)


def check_invalid(path, payload):
    path.write_bytes(payload)
    try:
        C.convertFile2Arrays(str(path), 'bin_plot3d')
    except OSError:
        pass
    else:
        raise AssertionError(f'Invalid Plot3D file was accepted: {path.suffix}')


with TemporaryDirectory() as directory:
    root = Path(directory)
    for endian in ('<', '>'):
        for real_format in ('f', 'd'):
            for suffix, names, fields in (
                ('.gbin', 'x,y,z', coordinates),
                ('.qbin', 'ro,rou,rov,row,roE', conservative),
                ('.fbin', 'var0,var1', functions),
            ):
                path = root / ('mesh' + suffix)
                write_plot3d(path, endian, real_format)
                check_arrays(C.convertFile2Arrays(str(path), 'bin_plot3d'), names, fields)

    # A basename groups companion files in the native reader.
    prefix = root / 'combined'
    for blank in (False, True):
        write_plot3d(prefix.with_suffix('.gbin'), '>', 'd', 'q', blank)
        write_plot3d(prefix.with_suffix('.qbin'), '<', 'f')
        write_plot3d(prefix.with_suffix('.fbin'), '>', 'f', 'q')
        arrays = converter.convertFile2Arrays(
            str(prefix), 'bin_plot3d', 20, 2, -1.0, None, None, None, None, 1
        )
        names = 'x,y,z,ro,rou,rov,row,roE,var0,var1'
        fields = coordinates + conservative + functions
        if blank:
            names += ',cellN'
            fields += (iblank,)
        check_arrays(arrays, names, fields)

    for suffix in ('.gbin', '.qbin', '.fbin'):
        path = root / ('truncated' + suffix)
        write_plot3d(path, '>', 'd')
        payload = path.read_bytes()
        check_invalid(path, payload[:-5])
        check_invalid(path, payload[:20])
        # The first dimension starts at byte 16 in these 32-bit integer headers.
        check_invalid(path, payload[:16] + struct.pack('>i', 2**30) + payload[20:])
