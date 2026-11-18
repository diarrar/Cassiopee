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
# include "CompGeom/compGeom.h"
# include "CompGeom/onedmapT.h"
# include <vector>

//=============================================================================
// map a 1D distribution over a profile
//=============================================================================
void K_COMPGEOM::onedmap(
  const E_Int ni,
  const E_Float* x, const E_Float* y, const E_Float* z,
  const E_Int no, const E_Float* d,
  E_Float* xo, E_Float* yo, E_Float* zo,
  E_Float* s, E_Float* dx, E_Float* dy, E_Float* dz
)
{
#ifdef E_DOUBLEREAL
  E_Float stota;

  // Parametrisation de la ligne entrante
  slope(ni, x, y, z, dx, dy, dz);

  paramFunc(ni, x, y, z, dx, dy, dz, stota, s);

  // Projection
  interp(ni, no, stota, s, d, x, y, z, dx, dy, dz, xo, yo, zo);
#else
  // single precision: the curvilinear abscissa is a cumulated sum over the
  // whole line, in float the two ends of a folded line (C-mesh wake) no
  // longer map to the same points. The mapping is computed in double.
  std::vector<double> xd(x, x+ni), yd(y, y+ni), zd(z, z+ni), dd(d, d+no);
  std::vector<double> sd(ni), dxd(ni), dyd(ni), dzd(ni);
  std::vector<double> xod(no), yod(no), zod(no);
  double stota;
  slopeT<double>(ni, &xd[0], &yd[0], &zd[0], &dxd[0], &dyd[0], &dzd[0]);
  paramFuncT<double>(ni, &xd[0], &yd[0], &zd[0], &dxd[0], &dyd[0], &dzd[0], stota, &sd[0]);
  interpT<double>(ni, no, stota, &sd[0], &dd[0], &xd[0], &yd[0], &zd[0],
                  &dxd[0], &dyd[0], &dzd[0], &xod[0], &yod[0], &zod[0]);
  for (E_Int i = 0; i < no; i++) { xo[i] = xod[i]; yo[i] = yod[i]; zo[i] = zod[i]; }
  for (E_Int i = 0; i < ni; i++) { s[i] = sd[i]; dx[i] = dxd[i]; dy[i] = dyd[i]; dz[i] = dzd[i]; }
#endif
}

//=============================================================================
void K_COMPGEOM::onedmapbar(
  const E_Int npts,
  const E_Float* x, const E_Float* y, const E_Float* z,
  const E_Int no, const E_Float* d,
  const E_Int net, const E_Int* cn1, const E_Int* cn2,
  const E_Int neto, E_Int* cn1o, E_Int* cn2o,
  E_Float* xo, E_Float* yo, E_Float* zo,
  E_Float* s, E_Float* dx, E_Float* dy, E_Float* dz
)
{
  E_Float stota;

  // Parametrisation de la ligne entrante
  slopebar(npts, net, cn1, cn2, x, y, z, dx, dy, dz);

  paramFuncBar(npts, net, cn1, cn2, x, y, z, dx, dy, dz, stota, s);

  // Projection
  interpbar(npts, no, stota, s, d, x, y, z, dx, dy, dz,
            xo, yo, zo, neto, cn1o, cn2o);
}
