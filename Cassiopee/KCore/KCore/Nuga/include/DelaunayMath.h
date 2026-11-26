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


#ifndef __LINEAR_DELAUNAY_MATH_H__
#define __LINEAR_DELAUNAY_MATH_H__

#include "Nuga/include/defs.h"
#include "Nuga/include/DynArray.h"

namespace K_LINEAR
{
  class DelaunayMath
  {
  public:
    static void eigen_vectors 
      (nuga_real_t a00, nuga_real_t a11, nuga_real_t a10, nuga_real_t& lambda0, nuga_real_t& lambda1, nuga_real_t* v0, nuga_real_t* v1);

    static void eigen_values 
      (nuga_real_t a00, nuga_real_t a11, nuga_real_t a10, nuga_real_t& lambda0, nuga_real_t& lambda1);

    static void simultaneous_reduction
      (const K_FLD::FloatArray& M1, const K_FLD::FloatArray& M2,
       nuga_real_t* V1, nuga_real_t* V2);

    static void intersect(const K_FLD::FloatArray& Mi, const K_FLD::FloatArray& M2, K_FLD::FloatArray& I);

    static void resoLin(nuga_real_t a11, nuga_real_t a12, nuga_real_t a21, nuga_real_t a22,
                        nuga_real_t d1, nuga_real_t d2, nuga_real_t&x, nuga_real_t&y);
  };
}

#endif
