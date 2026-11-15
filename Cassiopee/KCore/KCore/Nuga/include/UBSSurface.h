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

#ifndef __UBS_SURFACE_H__
#define __UBS_SURFACE_H__

#include "Nuga/include/DynArray.h"
#include <vector>

class UBSSurface
{
public:
  /// Builder
  static UBSSurface* buildUBSSurface(const K_FLD::FloatArray& pos, E_Int nj);

  /// Destructor.
  ~UBSSurface(void);

  /// Computes the surface point P for the input (u,v) parameters.
  void point(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Computes the first U-derivative at P(u,v) on the surface.
  void DU1(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Computes the second U-derivative at P(u,v) on the surface.
  void DU2(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Computes the first V-derivative at P(u,v) on the surface.
  void DV1(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Computes the second U-derivative at P(u,v) on the surface.
  void DV2(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Computes the first crossed UV-derivative at P(u,v) on the surface.
  void DUV(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Compute all derivatives in one go
  void DAUV1(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;
  void DAUV2(nuga_real_t u, nuga_real_t v, nuga_real_t* P) const;

  /// Gets the Umax.
  inline nuga_real_t getUmax() const { return _Umax;}

  /// Gets the Vmax.
  inline nuga_real_t getVmax() const { return _Vmax;}

  /// Triangulate using the spline value at knots.
  void triangulate(K_FLD::FloatArray& pos, K_FLD::IntArray& connectT3);
  
  /// Checks whether input parameters are in the bounds for this surface
  inline bool in_bounds(nuga_real_t u, nuga_real_t v) const
  {
    if (u < _Umin) return false;
    if (u > _Umax) return false;
    if (v < _Vmin) return false;
    if (v > _Vmax) return false;
    return true;
  }

private:

  /// Constructor with an input array of control points.
  UBSSurface(const K_FLD::FloatArray& pos, E_Int nj);

  /// pointer-to-function type for the form functions defined below.
  typedef nuga_real_t (*pBaseFunc)(nuga_real_t) ;

  /// Form Functions for a cubic spline.
  static inline nuga_real_t __N0(nuga_real_t t) { return ((1. - t) * (1. - t) * (1. - t)) / 6.; }
  static inline nuga_real_t __N1(nuga_real_t t) { return (4. - (6. * t * t) + (3. * t* t * t)) / 6.; }
  static inline nuga_real_t __N2(nuga_real_t t) { return (1. + (3. * t) + (3. * t * t) - (3. * t * t * t)) / 6.; }
  static inline nuga_real_t __N3(nuga_real_t t) { return (t * t * t) / 6.; }

  /// First derivative of the form Functions for a cubic spline.
  static inline nuga_real_t __DN0(nuga_real_t t) { return -0.5 * ((1. - t) * (1. - t)); }
  static inline nuga_real_t __DN1(nuga_real_t t) { return ( - (2. * t) + (1.5 * t * t)); }
  static inline nuga_real_t __DN2(nuga_real_t t) { return (0.5 + t - (1.5 * t * t)); }
  static inline nuga_real_t __DN3(nuga_real_t t) { return 0.5 * t * t; }

  /// Second derivative of the form Functions for a cubic spline.
  static inline nuga_real_t __D2N0(nuga_real_t t) { return (1. - t); }
  static inline nuga_real_t __D2N1(nuga_real_t t) { return ( -2. + (3. * t)); }
  static inline nuga_real_t __D2N2(nuga_real_t t) { return (1. - (3. * t)); }
  static inline nuga_real_t __D2N3(nuga_real_t t) { return t; }

  /// Core function used to evaluate a surface point or any of its derivatives.
  void __eval (nuga_real_t u, const std::vector<pBaseFunc>& FU, nuga_real_t v,
                          const std::vector<pBaseFunc>& FV, nuga_real_t* P) const;

  /// Set the control points.
  void __setControlPoints(const K_FLD::FloatArray& pos, E_Int nj);

private:public://fixme
  const K_FLD::FloatArray& _pos;
  K_FLD::IntArray          _ctrlPts;

  nuga_real_t _Umin;
  nuga_real_t _Umax;
  nuga_real_t _Vmin;
  nuga_real_t _Vmax;

  std::vector<pBaseFunc> _base;
  std::vector<pBaseFunc> _baseD1;
  std::vector<pBaseFunc> _baseD2;

};

#endif
