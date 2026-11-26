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
#ifndef _DIST2WALLS_EIKONAL_EPS_H_
#define _DIST2WALLS_EIKONAL_EPS_H_
#include <cmath>
#include <algorithm>
#include "Def/DefTypes.h"

// Convergence criterion of the Eikonal iterations on a value p:
// absolute 1.e-6 in double precision (historical behaviour); in single
// precision 1.e-6 is below the resolution of float for p > 10, which makes the
// iterations never converge, so the criterion becomes relative to p.
#ifdef E_DOUBLEREAL
#define EIKONAL_CONV_EPS(p) (1.E-6)
#else
#define EIKONAL_CONV_EPS(p) (1.E-5 * std::max(E_Float(1.), std::fabs(p)))
#endif

#endif
