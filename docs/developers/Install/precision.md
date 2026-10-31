# Floating-point precision

Cassiopee uses double precision by default. A source build can select either
`single` (IEEE binary32, NumPy `float32`) or `double` (IEEE binary64, NumPy
`float64`). The selection applies to `E_Float`, Fortran `REAL_E`, field
allocations and MPI transfers of those fields. Third-party libraries and
file formats can still require their own fixed-precision types; conversions
at those interfaces must preserve the format.
The viewer retains its existing binary64 coordinate/field cache and converts
incoming Cassiopee arrays. Codec dimension metadata also remains binary64,
including when reading CGNS files, to preserve dimensions above `2**24`.

## Build and install

Prepare the compilers and dependencies as described in [ubuntu.md](ubuntu.md).
On a cluster, load your usual compiler, MPI and Python environment and use your
existing Cassiopee machine configuration instead of the `ubuntu` example below.
From the repository root, select one environment before running the installer:

```bash
export CASSIOPEE="$PWD"
export MACHINE=ubuntu

# Simple precision
source "$CASSIOPEE/Cassiopee/Envs/sh_Cassiopee_r4"
export OMP_NUM_THREADS=4
./install

# Or double precision (the default)
source "$CASSIOPEE/Cassiopee/Envs/sh_Cassiopee_r8"
export OMP_NUM_THREADS=4
./install
```

Both environment scripts belong to this repository. No activation script or
absolute path from the cloud development environment is required.

The scripts set `CASSIOPEE_PRECISION=single` or `double`. The single precision
environment adds `_r4` to `ELSAPROD` (for example, `ubuntu_r4`). Its build and
default installation directories are separate from `ubuntu`. Setuptools also
uses a separate extension cache for each precision and relinks the wrappers
after SCons updates their static libraries. Build the variants
sequentially when using one source directory.

For a manually configured build, set `CASSIOPEE_PRECISION` explicitly and use a
distinct `ELSAPROD` and installation prefix for each variant. KCore must be
rebuilt and activated before compiling the other modules with the same mode.
The build rejects a KCore installation with an incompatible real size.

Never combine extensions of different precisions in one Python process or
install their wheels into the same Python prefix. Their binary interfaces are
different, even though their package names and versions are identical.

## Python arrays

Use the compiled type when passing numerical fields to native routines:

```python
import numpy
import KCore

print(KCore.REAL_DTYPE, KCore.REAL_SIZE)  # float32 / 4, or float64 / 8
coordinates = numpy.zeros((3, 100), dtype=KCore.E_NpyFloat)
```

These values describe the installed extension. Changing an environment variable
after importing it cannot change its precision. NumPy field arrays supplied to
the native array interface must match this dtype. CGNS readers convert real
data to the compiled precision; writers continue to honor the input node type.

Single precision has about seven significant decimal digits. Choose numerical
tolerances accordingly; FP64 reference results should not be compared bitwise
to FP32 results.

## Validation

After activating each installation, run:

```bash
python "$CASSIOPEE/Cassiopee/KCore/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/Converter/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/Generator/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/Compressor/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/Geom/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/Post/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/Intersector/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/OCC/test/precision.py" -v
python "$CASSIOPEE/Cassiopee/KCore/test/precision_build.py" -v
mpiexec -n 2 python "$CASSIOPEE/Cassiopee/XCore/test/precision_m2.py"
```

The core checks allocation dtype, alignment, ownership, OpenMP, and a Fortran
kernel linked from the installed library. The Converter checks exercise arrays,
expressions, pyTrees and CGNS interchange. Run the suites separately in each
mode; a successful import alone does not validate the precision.

The Generator suite requires Transform and the default TetGen/MMGS backends.
The Compressor suite checks both field types and the fixed-width codec metadata;
the MPI test exchanges a field through XCore between two ranks. These are
targeted checks, not a certification of every optional module or numerical
algorithm in the distribution.

The GNU/Linux onboarding validation built and installed all 19 component packages
in single precision, including CPlot and OCC with OpenCascade 7.8. The core,
Converter, Generator, Transform and Compressor were also built in double
precision. The runtime checks include a two-rank FP32 XCore field exchange,
analytic gradients and volumes, streamlines, STEP input and edge meshing.
Interactive graphics rendering was not exercised.
