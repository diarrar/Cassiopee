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

# include <math.h>
# include <stdio.h>
# include "CompGeom/compGeom.h"
# include "CompGeom/onedmapT.h"

//****************************************************************************
// Compute the parametrization
//    O.P JACQUOTTE  1987
//    F.MONTIGNY-RANNOU  1991
//****************************************************************************
void K_COMPGEOM::paramFunc(
  const E_Int m,
  const E_Float* x0, const E_Float* y0, const E_Float* z0,
  const E_Float* dx, const E_Float* dy, const E_Float* dz,
  E_Float& stota, E_Float* s0
)
{
  paramFuncT<E_Float>(m, x0, y0, z0, dx, dy, dz, stota, s0);
}

//=============================================================================
// _IN
// npts: dimension of input line
// x0, y0, z0: input line
// net: number of BAR elements
// cn1, cn2: BAR connectivity (first and second vertices)
// _OUT
// dx, dy, dz, s0: output arrays
//=============================================================================
void K_COMPGEOM::paramFuncBar(
  const E_Int npts, const E_Int net,
  const E_Int* cn1, const E_Int* cn2,
  const E_Float* x0, const E_Float* y0, const E_Float* z0,
  const E_Float* dx, const E_Float* dy, const E_Float* dz,
  E_Float& stota, E_Float* s0
)
{
  E_Int k, kp;
  E_Float delx, dely, delz, qx, qy, qz, q2, f, g, e, ds;
  E_Int ind0 = cn1[0] - 1;
  s0[ind0] = 0.0;

  for (E_Int et = 0; et < net; et++)
  {
    k = cn1[et] - 1;
    kp = cn2[et] - 1;
    delx = x0[kp] - x0[k];
    dely = y0[kp] - y0[k];
    delz = z0[kp] - z0[k];
    qx = dx[kp] + dx[k];
    qy = dy[kp] + dy[k];
    qz = dz[kp] + dz[k];
    q2 = qx*qx + qy*qy + qz*qz;
    g = delx*delx + dely*dely + delz*delz;
    f = delx*qx + dely*qy + delz*qz;
    e = 8.0 - 0.5*q2;
    ds = (3.0/e) * (sqrt(f*f + 2.0*g*e) - f);
    s0[kp] = s0[k] + ds;
  }

  stota = s0[npts-1];
  E_Float stotai = 1.0 / stota;
  for (E_Int k = 1; k < npts; k++) s0[k] *= stotai;
}
