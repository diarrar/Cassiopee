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

#ifndef __GENERATOR_PLASTER_H__
#define __GENERATOR_PLASTER_H__

#include "Nuga/include/DynArray.h"
#include "Nuga/include/defs.h"
#include "Nuga/include/DefContainers.h"

class Plaster
{
public:
  Plaster(void);
  ~Plaster(void);

  E_Int make(const K_FLD::FloatArray& pos, const K_FLD::IntArray& connectE2,
             K_FLD::FloatArray& posPlaster, E_Int& ni, nuga_real_t bump_factor = 0.);

private:

  void __cartesian(const nuga_real_t* minB, const nuga_real_t* maxB, E_Int ni, E_Int nj, K_FLD::FloatArray& cart);

  void __smooth(std::vector<nuga_real_t>& z, E_Int ni, nuga_real_t bump_factor, nuga_real_t tol = EPSILON);

  void __smooth_1(std::vector<nuga_real_t>& z, E_Int ni, nuga_real_t tol = EPSILON);

  void __smooth_2(std::vector<nuga_real_t>& z, E_Int ni, nuga_real_t tol = EPSILON);

#ifdef WIN32
#ifdef E_DEBUG
  void make_box(const nuga_real_t* minB, const nuga_real_t* maxB, K_FLD::FloatArray& pos, K_FLD::IntArray& connect);
  void drawBox(const K_FLD::FloatArray& pos, const K_FLD::IntArray& connect, const nuga_real_t* mB, const nuga_real_t* MB);
#endif
#endif

  void __initializePlaster(const K_FLD::FloatArray& plaster2D, E_Int ni, const K_FLD::FloatArray& pos2D,
                           const K_FLD::IntArray& connectE2/*, const std::vector<E_Int>& hard_nodes*/, const std::vector<nuga_real_t>& zE2,
                           const K_FLD::IntArray& connectT3, std::vector<nuga_real_t>& z, nuga_real_t bump_factor = 0.);

  void __blockSurroundingNodes(const K_FLD::FloatArray& plaster2D, E_Int ni, 
                               const NUGA::bool_vector_type& mask,
                               const K_FLD::FloatArray& pos2D,
                               const K_FLD::IntArray& connectE2, const std::vector<nuga_real_t>& zE2,
                               std::vector<nuga_real_t>& z, NUGA::int_set_type& onodes);
/*
  void __blockInsideNodes(const K_FLD::FloatArray& plaster2D, E_Int ni, 
                          const K_FLD::FloatArray& pos2D, const std::vector<E_Int>& hard_nodes,
                          const std::vector<nuga_real_t>& zE2, std::vector<nuga_real_t>& z);
*/
  ///
  void __bumpPlaster(const K_FLD::FloatArray& plaster2D, E_Int ni,
                     const NUGA::bool_vector_type& mask,
                     nuga_real_t bump_factor, const NUGA::int_set_type& onodes,
                     std::vector<nuga_real_t>& z);

  void __mask(const K_FLD::FloatArray& pos2D, const K_FLD::FloatArray& plaster2D,
              const K_FLD::IntArray& connectT3, NUGA::bool_vector_type& mask);

  bool __IsStrictlyInT3(const nuga_real_t* P, const nuga_real_t* P0, const nuga_real_t* P1, const nuga_real_t* P2);

  void __getPlasterBoundary(const NUGA::bool_vector_type& mask, E_Int ni, bool outside,
                            NUGA::int_set_type& nodes);

  void __blockNodes(const K_FLD::FloatArray& pos2D, const K_FLD::FloatArray& plaster2D,
                    const K_FLD::IntArray& connectE2, const std::vector<nuga_real_t>& zE2,
                    const NUGA::int_set_type& nodes, std::vector<nuga_real_t>& z);

  nuga_real_t __computeCharacteristicLength(const K_FLD::FloatArray& pos2D, const K_FLD::IntArray& connectE2);

#ifdef WIN32
#ifdef E_DEBUG
  static int _count;
#endif
#endif

};

#endif
