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
// 1D mapping of a distribution over a line (slope, parametrization, cubic
// Hermite interpolation), templated on the real type: onedmap computes in
// double whatever the compiled precision, the E_Float functions of
// compGeom.h are instantiated on E_Float (unchanged in double precision).
#ifndef _KCORE_COMPGEOM_ONEDMAPT_H_
#define _KCORE_COMPGEOM_ONEDMAPT_H_
#include <math.h>
#include "Def/DefTypes.h"

namespace K_COMPGEOM
{
template <typename R>
inline R valcubT(R a, R b, R c, R d, R t)
{
  R onemt = 1.0 - t;
  R t2 = t * t;
  R onemt2 = onemt * onemt;
  return (a * (1.0 + 2.0 * t) + c * t) * onemt2 +
         (b * (3.0 - 2.0 * t) + d * (t - 1.0)) * t2;
}

template <typename R>
void slopeT(const E_Int m,
  const R* x0, const R* y0, const R* z0,
  R* dx, R* dy, R* dz
)
{
  // 2 pts case
  if (m == 2)
  {
    R dxp = x0[1] - x0[0];
    R dyp = y0[1] - y0[0];
    R dzp = z0[1] - z0[0];
    R den = 1.0 / sqrt(dxp*dxp + dyp*dyp + dzp*dzp);
    dx[0] = dxp * den;
    dy[0] = dyp * den;
    dz[0] = dzp * den;
    dx[1] = dx[0];
    dy[1] = dy[0];
    dz[1] = dz[0];
    return;
  }

  // first vertex
  R dxm = x0[1] - x0[0];
  R dxp = x0[2] - x0[1];
  R dym = y0[1] - y0[0];
  R dyp = y0[2] - y0[1];
  R dzm = z0[1] - z0[0];  
  R dzp = z0[2] - z0[1];
  R d2m = dxm*dxm + dym*dym + dzm*dzm;
  R d2p = dxp*dxp + dyp*dyp + dzp*dzp;
  R dm = sqrt(d2m);
  R dp = sqrt(d2p);
  R ambda = (1.0 + dp/dm) * (1.0 + dp/dm);
  R cxm = ambda*dxm - dxm - dxp;
  R cym = ambda*dym - dym - dyp;
  R czm = ambda*dzm - dzm - dzp;
  R denm = 1.0 / sqrt(cxm*cxm + cym*cym + czm*czm);
  dx[0] = cxm * denm;
  dy[0] = cym * denm;
  dz[0] = czm * denm;
 
  // last vertex
  E_Int m1 = m-1;
  dxm = x0[m1-1] - x0[m1-2];
  dxp = x0[m1] - x0[m1-1];
  dym = y0[m1-1] - y0[m1-2];
  dyp = y0[m1] - y0[m1-1];
  dzm = z0[m1-1] - z0[m1-2];
  dzp = z0[m1] - z0[m1-1];
  d2m = dxm*dxm + dym*dym + dzm*dzm;
  d2p = dxp*dxp + dyp*dyp + dzp*dzp;
  dm = sqrt(d2m);
  dp = sqrt(d2p);
  ambda = (1.0 + dm/dp) * (1.0 + dm/dp);
  R cxp = ambda*dxp - dxm - dxp;
  R cyp = ambda*dyp - dym - dyp;
  R czp = ambda*dzp - dzm - dzp;
  R denp = 1.0 / sqrt(cxp*cxp + cyp*cyp + czp*czp);
  dx[m1] = cxp * denp;
  dy[m1] = cyp * denp;
  dz[m1] = czp * denp;

  // other points
  for (E_Int k = 1; k < m-1; k++)
  {
    E_Int kp = k+1;
    E_Int km = k-1;
    dxm = x0[k] - x0[km];
    dxp = x0[kp] - x0[k];
    dym = y0[k] - y0[km];
    dyp = y0[kp] - y0[k];
    dzm = z0[k] - z0[km];
    dzp = z0[kp] - z0[k];
    d2m = dxm*dxm + dym*dym + dzm*dzm;
    d2p = dxp*dxp + dyp*dyp + dzp*dzp;
    ambda = d2p / (d2m + d2p);
    R cx = (1.0 - ambda)*dxm + ambda*dxp;
    R cy = (1.0 - ambda)*dym + ambda*dyp;
    R cz = (1.0 - ambda)*dzm + ambda*dzp;
    R den = 1.0 / sqrt(cx*cx + cy*cy + cz*cz);
    dx[k] = cx * den;
    dy[k] = cy * den;
    dz[k] = cz * den;
  }
}

template <typename R>
void paramFuncT(const E_Int m,
  const R* x0, const R* y0, const R* z0,
  const R* dx, const R* dy, const R* dz,
  R& stota, R* s0
)
{
  E_Int kp;
  R delx, dely, delz, qx, qy, qz, q2, f, g, e, ds;
  
  s0[0] = 0.0;
  // ds is only used as a temporary in the loop, no need to initialize here

  // degenerated edge (commented out in original)
  /*
  R sma = 1000.0 * small;
  R x1m = fabs(x0[0] - x0[m-1]);
  R y1m = fabs(y0[0] - y0[m-1]);
  R z1m = fabs(z0[0] - z0[m-1]);
  if (x1m <= sma && y1m <= sma && z1m <= sma)
  {
    for (E_Int k = 1; k < m; k++)
    {
      s0[k] = sma;
    }
    stota = s0[m-1];
    return;
  }
  */

  for (E_Int k = 0; k < m-1; k++)
  {
    kp = k+1;
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

  stota = s0[m-1];
  R stotai = 1.0 / stota;
  for (E_Int k = 0; k < m-1; k++) s0[k+1] *= stotai;
}

template <typename R>
void interpT(const E_Int im0, const E_Int im, const R stota,
  const R* s0, const R* s,
  const R* tabx0, const R* taby0, const R* tabz0,
  const R* dx0, const R* dy0, const R* dz0,
  R* tabx, R* taby, R* tabz
)
{
  tabx[0] = tabx0[0];
  taby[0] = taby0[0];
  tabz[0] = tabz0[0];
  tabx[im-1] = tabx0[im0-1];
  taby[im-1] = taby0[im0-1];
  tabz[im-1] = tabz0[im0-1];

  if (im == 2) return;
  else
  {
    E_Int k;
    R t, coef, d1, d2;

    for (E_Int i = 1; i < im-1; i++)
    {
      for (k = 0; k < im0-1; k++)
      {
        if (s[i] <= s0[k+1]) break;
      }
      if (k >= im0-1) k = im0-2;
      t = (s[i] - s0[k]) / (s0[k+1] - s0[k]);
      coef = stota * (s0[k+1] - s0[k]);
      d1 = dx0[k] * coef;
      d2 = dx0[k+1] * coef;
      tabx[i] = valcubT<R>(tabx0[k], tabx0[k+1], d1, d2, t);
      d1 = dy0[k] * coef;
      d2 = dy0[k+1] * coef;
      taby[i] = valcubT<R>(taby0[k], taby0[k+1], d1, d2, t);
      d1 = dz0[k] * coef;
      d2 = dz0[k+1] * coef;
      tabz[i] = valcubT<R>(tabz0[k], tabz0[k+1], d1, d2, t);
    }
  }
}

}
#endif
