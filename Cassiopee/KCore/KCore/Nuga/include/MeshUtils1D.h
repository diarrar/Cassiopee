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


#ifndef __DELAUNAY_MESH_UTILS_1D_H__
#define __DELAUNAY_MESH_UTILS_1D_H__

#include "Nuga/include/defs.h"
#include "Nuga/include/DynArray.h"
#include "Nuga/include/maths.hxx"
#include <vector>

namespace DELAUNAY
{


class MeshUtils1D
{
public:
  MeshUtils1D(void);
  ~MeshUtils1D(void);

  template <short DIM>
  static void mesh_line (const nuga_real_t* P0, const nuga_real_t* P1, E_Int N,
    K_FLD::FloatArray& pos, K_FLD::IntArray& connect);

  template <short DIM>
  static void mesh_arc (nuga_real_t* C, nuga_real_t* axis, nuga_real_t* P0, nuga_real_t alpha, E_Int N,
    K_FLD::FloatArray& pos, K_FLD::IntArray& connect);

  static void
    compute_iso_metric(const K_FLD::FloatArray& pos, const K_FLD::IntArray& connect,
    const std::vector<E_Int>& hard_nodes,
    std::vector<nuga_real_t> & metric, nuga_real_t &hmax, nuga_real_t& hmin);
};

template <short DIM>
void
MeshUtils1D::mesh_line
(const nuga_real_t* P0, const nuga_real_t* P1, E_Int N,
 K_FLD::FloatArray& pos, K_FLD::IntArray& connect)
{
  nuga_real_t E[DIM];
  nuga_real_t Pi[DIM];
  E_Int   Ei[2];

  NUGA::diff<DIM>(P1, P0, E);
  nuga_real_t L = NUGA::normalize<DIM>(E);
  nuga_real_t ti = L/N;

  pos.pushBack(P0, P0+DIM);
  E_Int I = pos.cols()-1;

  for (E_Int i = 0; i < N; ++i)
  {
    for (E_Int j = 0; j < DIM; ++j)
      Pi[j] = pos(j, I) + ti * E[j];

    pos.pushBack(Pi, Pi+DIM);
    Ei[0] = ++I-1;
    Ei[1] = I;
    connect.pushBack(Ei, Ei+2);
  }
}

template <short DIM>
void
MeshUtils1D::mesh_arc (nuga_real_t* C, nuga_real_t* axis, nuga_real_t* P0, nuga_real_t alpha, E_Int N,
                       K_FLD::FloatArray& pos, K_FLD::IntArray& connect)
{
  nuga_real_t R = sqrt(NUGA::sqrDistance(C, P0, DIM));
  nuga_real_t ai = alpha*R/N;
  nuga_real_t Pj[DIM], CPi[DIM], PiPj[DIM];
  const nuga_real_t* Pi;
  E_Int Ei[2], I;

  pos.pushBack(P0, P0+DIM);
  I = pos.cols()-1;

  NUGA::normalize<3>(axis);

  for (E_Int i = 0; i < N; ++i)
  {
    Pi = pos.col(I);
    NUGA::diff<DIM>(Pi, C, CPi);
    NUGA::crossProduct<3>(axis, CPi, PiPj);
    NUGA::normalize<3>(PiPj);

    for (E_Int j = 0; j < DIM; ++j)
      Pj[j] = *(Pi+j) + ai * PiPj[j];

    pos.pushBack(Pj, Pj+DIM);

    Ei[0] = ++I-1;
    Ei[1] = I;

    connect.pushBack(Ei, Ei+2);
  }
}

}


#endif

