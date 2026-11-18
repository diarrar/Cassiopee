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


#ifndef __NUGA_DEFS_H__
#define __NUGA_DEFS_H__

// Nuga (geometric kernel: Delaunay, booleans, trees...) computes in double
// whatever the compiled precision of nuga_real_t. Conversions happen at the
// interface with Cassiopee arrays (FldArrayF <-> K_FLD::FloatArray).
typedef double nuga_real_t;
#include <type_traits>
// enable a converting overload only for a real type U different from nuga_real_t
#define NUGA_OTHER_REAL(U) typename std::enable_if<std::is_floating_point<U>::value && !std::is_same<U, nuga_real_t>::value, int>::type = 0

#define ZERO_M       1.e-15
#define EPSILON      1.e-12

#define Vector_t     std::vector

#define SIGN(a) ((a < -ZERO_M) ? -1 : ((a > ZERO_M) ? 1 : 0))  

// types
#include "Def/DefTypes.h"
#include "Def/DefCplusPlusConst.h"

#ifdef E_DOUBLEINT
  #define IDX_NONE 9223372036854775807
#else
  #define IDX_NONE 2147483647
#endif

namespace NUGA
{
static const nuga_real_t FLOAT_MAX = K_CONST::E_MAX_FLOAT;
// Nuga computes in double: its constants must not come from K_CONST, whose
// E_Float values are rounded to float in single precision (pi would then be
// 3.14159274 in the angle tests). Same values as K_CONST in double precision.
static const nuga_real_t SQRT3 = 1.73205080756887729353;

static const nuga_real_t PI   = 3.14159265358979323846;
static const nuga_real_t PI_2 = 1.57079632679489661923;
static const nuga_real_t PI_4 = 0.78539816339744830962;
}

#endif
