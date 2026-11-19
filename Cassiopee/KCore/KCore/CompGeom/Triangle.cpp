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

# include <stdio.h>
# include "CompGeom/compGeom.h"
# include <math.h>

using namespace std;
using namespace K_FLD;
using namespace K_FUNC;

//=============================================================================
K_COMPGEOM::Triangle::Triangle(E_Float coefa, E_Float coefb, 
                               E_Float coefc, E_Float coefd,
                               E_Int indA, E_Int indB, E_Int indC, 
                               FldArrayF& coord)
{ 
  _indA = indA; 
  _indB = indB;
  _indC = indC;
  compCircumCircle(coefa, coefb, coefc, coefd, coord); 
}

//=============================================================================
K_COMPGEOM::Triangle::~Triangle()
{
}
//=============================================================================
/*Return true if the point coord(ind,) is in the circumcircle of the triangle*/
//=============================================================================
E_Bool K_COMPGEOM::Triangle::isPointInCircumCircle(
  E_Int ind, FldArrayF& coord, E_Float tol)
{ 
  E_Float* xp = coord.begin(1);
  E_Float* yp = coord.begin(2);
  E_Float* zp = coord.begin(3);
  double dx = xp[ind] - _coordCC[0];
  double dy = yp[ind] - _coordCC[1];
  double dz = zp[ind] - _coordCC[2];

  double d = dx*dx + dy*dy + dz*dz;
  
  if ( d <= _radius2 + tol ) return true;
  else return false;
}

//=============================================================================
/* Computes the circum circle of the triangle */
//=============================================================================
void K_COMPGEOM::Triangle::compCircumCircle(E_Float coefa, E_Float coefb, 
                                            E_Float coefc, E_Float coefd,
                                            FldArrayF& coord)
{  
  double invdet;
  E_Float* xp = coord.begin(1);
  E_Float* yp = coord.begin(2);
  E_Float* zp = coord.begin(3);
  double x1 = xp[_indA];
  double y1 = yp[_indA];
  double z1 = zp[_indA];
  double x2 = xp[_indB];
  double y2 = yp[_indB];
  double z2 = zp[_indB];
  double x3 = xp[_indC]; 
  double y3 = yp[_indC]; 
  double z3 = zp[_indC];

  double norm1 = x1*x1 + y1*y1 + z1*z1;
  double n12 = (x2*x2 + y2*y2 + z2*z2) - norm1;
  double n13 = (x3*x3 + y3*y3 + z3*z3) - norm1;

  double x12 = 2. * (x2 - x1);
  double x13 = 2. * (x3 - x1);

  double y12 = 2. * (y2 - y1);
  double y13 = 2. * (y3 - y1);

  double z12 = 2. * (z2 - z1);
  double z13 = 2. * (z3 - z1);

  double det = (coefa * y12 * z13 + coefb * z12 * x13 + coefc * x12 * y13)
              - (coefa * y13 * z12 + coefb * z13 * x12 + coefc * x13 * y12);

  if (fEqualZero(det) != true)
  {   
    invdet = 1. / det;
  
    double det1 = 
      (-coefd * y12 * z13 + coefb * z12 * n13 + coefc * n12 * y13) 
      - (-coefd * y13 * z12 + coefb * z13 * n12 + coefc * n13 * y12);

    double det2 = 
      (coefa * n12 * z13 - coefd * z12 * x13 + coefc * x12 * n13)
      - (coefa * n13 * z12 - coefd * x12 * z13 + coefc * x13 * n12);
   
    double det3 = 
      (coefa * y12 * n13 + coefb * n12 * x13 - coefd * x12 * y13) 
      - (coefa * y13 * n12 + coefb * n13 * x12 - coefd * x13 * y12);

    _coordCC[0] = det1 * invdet;
    _coordCC[1] = det2 * invdet;
    _coordCC[2] = det3 * invdet;

    // square radius 
    _radius2 = (x1-_coordCC[0])*(x1-_coordCC[0]) + 
      (y1-_coordCC[1])*(y1-_coordCC[1]) +
      (z1-_coordCC[2])*(z1-_coordCC[2]);

    double distCC =
      _coordCC[0] * _coordCC[0] +
      _coordCC[1] * _coordCC[1] +
      _coordCC[2] * _coordCC[2]; 
    
    _distmax = _radius2 + distCC + 2*sqrt(distCC*_radius2);
  }
  else
  {
    //printf(" det = %12.15e\n", det);
    //printf("Warning...det = 0 in circumcenter computation.\n");
    //printf("ind=%d %d %d\n", _indA, _indB, _indC);
    //printf("x1= %12.15e %12.15e %12.15e\n",x1,y1,z1);
    //printf("x2= %12.15e %12.15e %12.15e\n",x2,y2,z2);
    //printf("x3= %12.15e %12.15e %12.15e\n",x3,y3,z3);
    _coordCC[0] = x1;
    _coordCC[1] = y1;
    _coordCC[2] = z1;
    _radius2 = 0.;
    _distmax = 0.;
  }
}
// ======================= KCore/CompGeom/Triangle.cpp ====================
