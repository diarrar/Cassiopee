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

#include "compGeom.h"

using namespace K_FLD;
using namespace K_FUNC;
using namespace std;

#define dot(u,v) ( u[0]*v[0] + u[1]*v[1] + u[2]*v[2] )

//=============================================================================
E_Int K_COMPGEOM::intersectRayTriangle(E_Float* p1, E_Float* p2, E_Float* p3,
                                       E_Float* pR0, E_Float* pR1,
                                       E_Float* pi)
{
  // Computed in double: in single precision, a ray through a node or an edge
  // of the surface gives barycentric coordinates off by ~1e-7/h, beyond tol,
  // and every adjacent triangle rejects it (unchanged in double precision)
  double tol = 1.e-6;
  double small = 1.e-18;
    
  double u[3];
  double v[3];
  double n[3];
  double dir[3];
  double w[3];
  double w0[3];
  double pid[3];
  double a, b;
  
  // Aretes du triangle + normal au plan du triangle
  u[0] = double(p2[0]) - p1[0];
  u[1] = double(p2[1]) - p1[1];
  u[2] = double(p2[2]) - p1[2];
  v[0] = double(p3[0]) - p1[0];
  v[1] = double(p3[1]) - p1[1];
  v[2] = double(p3[2]) - p1[2];
  n[0] = u[1]*v[2]-u[2]*v[1];
  n[1] = u[2]*v[0]-u[0]*v[2];
  n[2] = u[0]*v[1]-u[1]*v[0];
  if (fEqualZero(n[0]*n[0] + n[1]*n[1] + n[2]*n[2], 1.e-28) == true)
    return -1;

  dir[0] = double(pR1[0]) - pR0[0];
  dir[1] = double(pR1[1]) - pR0[1]; 
  dir[2] = double(pR1[2]) - pR0[2];
  
  w0[0] = double(pR0[0]) - p1[0];
  w0[1] = double(pR0[1]) - p1[1];
  w0[2] = double(pR0[2]) - p1[2];
  
  a = -dot(n, w0);
  b = dot(n, dir);

  if (K_FUNC::E_abs(b) < small)
  {
    if (fEqualZero(a) == true) return 2;
    else return 0;
  }

  // Intersection
  double r, s, t;
  r = a / b;
  
  pid[0] = pR0[0] + r * dir[0];
  pid[1] = pR0[1] + r * dir[1];
  pid[2] = pR0[2] + r * dir[2];
  pi[0] = pid[0]; pi[1] = pid[1]; pi[2] = pid[2];
  
  // pi est dans le triangle?
  double uu, uv, vv, wu, wv, D;
  uu = dot(u,u);
  uv = dot(u,v);
  vv = dot(v,v);
  w[0] = pid[0] - p1[0];
  w[1] = pid[1] - p1[1];
  w[2] = pid[2] - p1[2];
  wu = dot(w,u);
  wv = dot(w,v);
  D = uv * uv - uu * vv;
  
  // Test les coordonnees parametriques
  s = (uv * wv - vv * wu) / D;
  if (s < -tol || s > 1.+tol) return 0;
  t = (uv * wu - uu * wv) / D;
  if (t < -tol || (s+t) > 1.+tol) return 0;
  
  return 1;  
}

