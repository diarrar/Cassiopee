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

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

// ==========================================================================
#ifndef _KCORE_DEF_DEFTYPES_H_
#define _KCORE_DEF_DEFTYPES_H_

// Real precision: double (default) or float when E_SINGLEREAL is set
// by compilation option (ELSAPROD suffix _r4)
#ifndef E_SINGLEREAL
  #define E_DOUBLEREAL
#endif
//#define E_DOUBLEINT // set by compilation option

// ==========================================================================
///+ Basic types

// Essai pour trouver si long long existe (8 octets)
#ifdef LLONG_MAX
  #define E_LONG long long
  //#define E_LONG int64_t
#else
  #define E_LONG long
#endif

#ifdef E_DOUBLEREAL
  typedef double E_Float;
  #ifdef E_MPI
    #define E_PCM_FLOAT MPI_DOUBLE
  #else
    #define E_PCM_FLOAT sizeof(E_Float)
  #endif
  #define E_NPY_FLOAT NPY_DOUBLE
  #define E_MAXFLOAT 1.797693134862315e308
  typedef int64_t E_FloatAsInt; // integer stored in an E_Float slot (same size)
  // largest integer N such that all integers of [0,N] are exact in an E_Float
  #define E_MAXFLOATINT 9007199254740992.
#else
  typedef float E_Float;
  #ifdef E_MPI
    #define E_PCM_FLOAT MPI_FLOAT
  #else
    #define E_PCM_FLOAT sizeof(E_Float)
  #endif
  #define E_NPY_FLOAT NPY_FLOAT32
  #define E_MAXFLOAT 3.4028235e38
  typedef int32_t E_FloatAsInt; // integer stored in an E_Float slot (same size)
  // largest integer N such that all integers of [0,N] are exact in an E_Float
  // (indices stored in real fields are exact up to 2^24 points)
  #define E_MAXFLOATINT 16777216.
#endif

// Int (int or long long)
#ifdef E_DOUBLEINT
  typedef int64_t E_Int;
  typedef int E_Bool;
  typedef int E_Boolean; //#OBSOLETE
  #ifdef E_MPI
    #define E_PCM_INT MPI_LONG
  #else
    #define E_PCM_INT sizeof(E_Int)
  #endif
  #define E_NPY_INT NPY_INT64 
  #define E_IDX_NONE 9223372036854775807
  #define E_MAXINT 9223372036854775807 // Max for signed int64
#else
  typedef int E_Int;
  typedef int E_Bool;
  typedef int E_Boolean; //#OBSOLETE
  #ifdef E_MPI
    #define E_PCM_INT MPI_INT
  #else
    #define E_PCM_INT sizeof(E_Int)
  #endif
  #define E_NPY_INT NPY_INT
  #define E_IDX_NONE 2147483647
  #define E_MAXINT 2147483647 // Max for signed int32
#endif

///-

#ifdef E_DOUBLEREAL
  #define E_EPSILON 1.e-12
  // geometric coincidence tolerance given in double: unchanged
  #define E_REALTOL(x) (x)
#else
  #define E_EPSILON 1.e-6
  // geometric coincidence tolerance given in double: floored at the resolution of float
  #define E_REALTOL(x) (((x) < 1.e-6) ? 1.e-6 : (x))
#endif

///-

#endif
// ===== Def/DefTypes.h === Last line ===
