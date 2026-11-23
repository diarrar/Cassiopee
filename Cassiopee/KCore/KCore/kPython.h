// To avoid pb when python is compiled with gcc and we compile with icc
#ifdef __INTEL_COMPILER
//#define __attribute__(x)
#endif
#include <math.h> // because of _hypot in python.h
#include "Python.h"
#if PY_VERSION_HEX >= 0x03000000
#define PyString_FromString PyBytes_FromString
#define PyString_AsString   PyBytes_AsString
#define PyString_Check PyBytes_Check
#define PyInt_Check PyLong_Check
#define PyInt_AsLong PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong
#endif

// True for a python float or a numpy floating scalar (to be used where numpy
// arrayobject.h is included). numpy.float64 derives from float but
// numpy.float32 does not: values computed from float32 arrays reach the
// modules as numpy.float32 scalars (numpy >= 2 keeps float32 in mixed scalar
// arithmetic, NEP 50). Read them with PyFloat_AsDouble.
#define K_PYFLOAT_CHECK(o) (PyFloat_Check(o) || PyArray_IsScalar(o, Floating))
