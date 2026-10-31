/*    
    Copyright 2013-2026 ONERA.

    This file is part of Cassiopee.

    Cassiopee is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cassiopee is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cassiopee.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "kcore.h"
#include <stdlib.h>

//=============================================================================
// Threaded memcpy
//==============================================================================
void K_KCORE::memcpy__(E_Int* a, E_Int* b, E_Int s)
{
  if (s < 100) memcpy(a, b, s*sizeof(E_Int));
  else
  {
#pragma omp parallel for default (shared)
    for (E_Int i = 0; i < s; i++) a[i] = b[i];
  }
}
void K_KCORE::memcpy__(E_Float* a, E_Float* b, E_Int s)
{
  if (s < 100) memcpy(a, b, s*sizeof(E_Float));
  else
  {
#pragma omp parallel for default (shared)
    for (E_Int i = 0; i < s; i++) a[i] = b[i];
  }
}

//=============================================================================
// Allocate an aligned, Fortran-contiguous array of the compiled real type.
//=============================================================================
PyObject* K_KCORE::empty(PyObject* self, PyObject* args)
{
  PyObject* shape; E_Int align;
  if (!PYPARSETUPLE_(args, O_ I_, &shape, &align)) return NULL;
  IMPORTNUMPY;
  if (align < 0 || (align != 0 && (align & (align-1)) != 0))
  {
    PyErr_SetString(PyExc_ValueError, "empty: alignment must be zero or a power of two.");
    return NULL;
  }
  npy_intp alignment = align > E_Int(sizeof(E_Float)) ? align : sizeof(E_Float);
  npy_intp dims[NPY_MAXDIMS];
  int nd = PyTuple_Check(shape) ? int(PyTuple_Size(shape)) : 1;
  if (nd > NPY_MAXDIMS)
  {
    PyErr_SetString(PyExc_ValueError, "empty: too many dimensions.");
    return NULL;
  }
  npy_intp bytes = sizeof(E_Float);
  for (int i = 0; i < nd; ++i)
  {
    PyObject* item = PyTuple_Check(shape) ? PyTuple_GetItem(shape, i) : shape;
    dims[i] = PyLong_AsSsize_t(item);
    if (PyErr_Occurred()) return NULL;
    if (dims[i] < 0)
    {
      PyErr_SetString(PyExc_ValueError, "empty: negative dimensions are not allowed.");
      return NULL;
    }
    if (dims[i] != 0 && bytes > (NPY_MAX_INTP-alignment)/dims[i])
      return PyErr_NoMemory();
    bytes *= dims[i];
  }
  npy_intp capacity = bytes + alignment;
  PyObject* owner = PyArray_SimpleNew(1, &capacity, NPY_UBYTE);
  if (owner == NULL) return NULL;
  uintptr_t address = reinterpret_cast<uintptr_t>(PyArray_DATA((PyArrayObject*)owner));
  void* data = reinterpret_cast<void*>((address+alignment-1) & ~(uintptr_t(alignment)-1));
  PyObject* arr = PyArray_NewFromDescr(&PyArray_Type, PyArray_DescrFromType(E_NPY_FLOAT),
                                      nd, dims, NULL, data, NPY_ARRAY_F_CONTIGUOUS |
                                      NPY_ARRAY_ALIGNED | NPY_ARRAY_WRITEABLE, NULL);
  if (arr == NULL) { Py_DECREF(owner); return NULL; }
  if (PyArray_SetBaseObject((PyArrayObject*)arr, owner) < 0)
  { Py_DECREF(arr); return NULL; }
  return arr;
}
