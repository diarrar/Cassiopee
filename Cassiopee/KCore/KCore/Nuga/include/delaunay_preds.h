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


#ifndef __DELAUNAY_PREDICATES_H__
#define __DELAUNAY_PREDICATES_H__

#include "Nuga/include/Triangle.h"
#include "Metric.h"

namespace DELAUNAY
{

/**
   Predicate that returns true if the input edge is hard.
  */
struct HardEdgeCriterion // : public std::unary_function <const K_MESH::NO_Edge&, bool>
{

  explicit HardEdgeCriterion(const NUGA::non_oriented_edge_set_type& hard_edges)
    :_hard_edges(hard_edges){}

  inline bool operator() (const K_MESH::NO_Edge& Ei) const
  {
      return (_hard_edges.find(Ei) != _hard_edges.end());
  }

  const NUGA::non_oriented_edge_set_type& _hard_edges;
};

///
template <typename T>
struct no_predicate // : public std::unary_function <T, bool>
{
  inline bool operator() (const T& ) const
  {
    return false;
  }
};


/**
   Predicate that returns true if the input node is in the K ball.
   */

template <typename T>
struct DelaunayCriterion // : public std::unary_function <E_Int, bool>
{
  explicit DelaunayCriterion(const K_FLD::FloatArray& pos, const K_FLD::IntArray& connect)
    :_pos(pos), _connect(connect), _m(0){}

  inline void setPoint(const nuga_real_t* pt, const T& m){_pt = pt;_m = &m;}

  inline bool operator() (E_Int K) const
  {
    nuga_real_t              R2, C[2];

    _t.circumDiskCenter(_pos, _connect.col(K), R2, C, *_m);

    return (R2 > 0.) ? (distance2(C, _pt) < R2 * (1.0 + 1.e-10)) : false;//fixme : tol
  }
 
  inline nuga_real_t distance2(const nuga_real_t* P0, const nuga_real_t* P1) const
  {
    nuga_real_t u = P1[0] - P0[0];
    nuga_real_t v = P1[1] - P0[1];
    return ((*_m)[0] * u * u) + (2. * ((*_m)[1] * u * v) +  ((*_m)[2] * v * v));
  }

  const K_FLD::FloatArray& _pos;
  const K_FLD::IntArray&   _connect;
  const nuga_real_t*           _pt;
  const T*                 _m;
  K_MESH::Triangle         _t;
};

//iso specialization
template <> inline
nuga_real_t
DelaunayCriterion<nuga_real_t>::distance2(const nuga_real_t* P0, const nuga_real_t* P1) const
{
  nuga_real_t u = P1[0] - P0[0];
  nuga_real_t v = P1[1] - P0[1];
  return (u * u) + (v * v); 
}

}

#endif
