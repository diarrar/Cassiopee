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

#ifndef __MASKGEN_H__
#define __MASKGEN_H__

//#include "Nuga/include/ArrayAccessor.h"
#include "Nuga/include/ArrayWriter.h"
#include <vector>
#define Vector_t std::vector
#include "Nuga/include/BbTree.h"
#include "Nuga/include/KdTree.h"
#include "Nuga/include/Tetrahedron.h"
#include "Nuga/include/EltAlgo.h"

#ifdef DEBUG_MASK
#include "IO/io.h"
#include <sstream>
#include "meshIO/chrono.h"
#endif

#define MASKED 0
#define VISIBLE 1

namespace K_CONNECTOR
{
  class maskGen
  {
    public:
      
      enum eType {TRI/*, QUAD, PG*/, TH4/*, PH*/};
      
      typedef K_FLD::ArrayAccessor<K_FLD::FldArrayF> ACoord_t;
      typedef K_FLD::ArrayAccessor<K_FLD::FldArrayI> AConnec_t;
      
      maskGen(const K_FLD::FldArrayF& coord,  E_Int px, E_Int py, E_Int pz, const K_FLD::FldArrayI& conn, eType ELT, nuga_real_t tolerance = E_EPSILON)
              :_coordT4(coord, px, py, pz), _connT4(new K_FLD::ArrayAccessor<K_FLD::FldArrayI>(conn, -1)),//fld indices start at 1
               _tolerance(tolerance), _tree(0), _reoriented(false), _ELType(ELT), _kdtree(0){__init();}

      ///    
      ~maskGen();
      
      E_Int blank(const K_FLD::FldArrayF& coord, E_Int px, E_Int py, E_Int pz, K_FLD::FldArrayI& isBlanked, E_Int cellnval=0, bool overwrite=false);
     
      ///
      inline E_Int nb_points() const { return _coordT4.size();}
      ///
      inline E_Int nb_elts() const { return (_connT4 ? _connT4->size() : 0);}
      
      //Accessors to pass the info to a Collider
      K_SEARCH::BbTree3D* getTree(){return _tree;}
      const ACoord_t& coords(){return _coordT4;}
      AConnec_t& connect(){return *_connT4;}
      nuga_real_t tolerance(){return _tolerance;}
            
    private:
      
      ///
      template <typename ELT_t>
      void __blank(const ACoord_t& coord, Vector_t<E_Int>& isBlanked);
      ///
      template <typename ELT_t>
      int is_inside(const nuga_real_t* pt, Vector_t<E_Int>& caught_boxes);
      
      ///
      void __init();
      ///
      template <typename ELT_t>
      bool __reorient();
      ///
      void __create_boxes(Vector_t<K_SEARCH::BBox3D*>& boxes);
      
#ifdef DEBUG_MASK
      public:
        static bool dbg_switch;
#endif
       
    private:
      ACoord_t _coordT4;
      AConnec_t* _connT4;
      nuga_real_t _tolerance;
      Vector_t<K_SEARCH::BBox3D*> _boxes;
      K_SEARCH::BbTree3D         *_tree;
      bool _reoriented;
      eType _ELType;
      // Tri specific
      K_SEARCH::KdTree<K_FLD::FldArrayF>* _kdtree;
      Vector_t<E_Int> _ancestors;
      K_FLD::FloatArray _isoG, _normals;
  };
  

  /// Specialization : Tet soup : reorient individually each tet
  template <> inline
  bool maskGen::__reorient<K_MESH::Tetrahedron>()
  {    
    Vector_t<size_t> elts_to_reorient;
    size_t nb_elts(_connT4->size());
    const E_Int NB_NODES = _connT4->stride();
    nuga_real_t Ni[3], Nj[3], Nk[3], Nl[3];
    Vector_t<E_Int> t4(NB_NODES);
    
    for (size_t i = 0; i < nb_elts; ++i)
    {
      _connT4->getEntry(i, &t4[0]);
      
      _coordT4.getEntry(t4[0], Ni);
      _coordT4.getEntry(t4[1], Nj);
      _coordT4.getEntry(t4[2], Nk);
      _coordT4.getEntry(t4[3], Nl);
      
      E_Float fNi[3] = {(E_Float)Ni[0], (E_Float)Ni[1], (E_Float)Ni[2]}, fNj[3] = {(E_Float)Nj[0], (E_Float)Nj[1], (E_Float)Nj[2]};
      E_Float fNk[3] = {(E_Float)Nk[0], (E_Float)Nk[1], (E_Float)Nk[2]}, fNl[3] = {(E_Float)Nl[0], (E_Float)Nl[1], (E_Float)Nl[2]};
      if (K_FUNC::zzdet4(fNi, fNj, fNk, fNl) < -E_EPSILON) //must be reoriented
        elts_to_reorient.push_back(i);
    }
    
    size_t s = elts_to_reorient.size();
    if (s == 0) return false;
    
    //need to create a copy of the input connectivity
    typedef AConnec_t::array_type acon_t;
    acon_t* new_connect = new acon_t(_connT4->array());
    K_FLD::ArrayWriter<acon_t> writer(*new_connect, _connT4->posX(0), _connT4->posX(1), _connT4->posX(2));
    //permut the first and second nodes
    for (size_t i = 0; i < s; ++i)
      std::swap(writer.getVal(elts_to_reorient[i], 0), writer.getVal(elts_to_reorient[i], 1));
    
    //reassign the new array to the accessor
    E_Int shift = _connT4->shift();
    delete _connT4;
    _connT4 = new K_FLD::ArrayAccessor<acon_t>(*new_connect, shift);
    
    return true;
  }
  
  ///
  template <> inline
  int maskGen::is_inside<K_MESH::Tetrahedron>(const nuga_real_t* pt, Vector_t<E_Int>& boxes)
  {
    bool ret = false;
    boxes.clear();
    nuga_real_t mB[]={pt[0]-E_EPSILON, pt[1]-E_EPSILON, pt[2]-E_EPSILON};//fixme : necessary ?
    nuga_real_t MB[]={pt[0]+E_EPSILON, pt[1]+E_EPSILON, pt[2]+E_EPSILON};
    
    _tree->getOverlappingBoxes(mB, MB, boxes);
    size_t sz = boxes.size();
    
#ifdef DEBUG_MASK
    
if (dbg_switch)
  std::cout << "number of caught boxes for [" << pt[0] << "," << pt[1] << "," << pt[2] << "] :" << sz << std::endl;
    
static int count = 0;
if (dbg_switch && sz)
{
  std::ostringstream fname;
  fname << "caught_" << count++ << ".mesh";
  K_FLD::IntArray connect;
  E_Int t4[4];
  for (size_t i = 0; i < sz; ++i)
  {
    _connT4->getEntry(boxes[i], t4);
    connect.pushBack(t4, t4+4);
  }
  K_FLD::FloatArray c(_coordT4.array());
  
  std::cout << connect << std::endl;
  
  c.pushBack(pt, pt+3);//a way to add the test point
  t4[0]=connect(0,0);
  t4[1]=connect(1,0);
  t4[2]=connect(2,0);
  t4[3]=c.cols()-1;
  connect.pushBack(t4, t4+4);
  K_CONVERTER::DynArrayIO::write(fname.str().c_str(), c, connect, "TETRA"); 
}
#endif
    
    E_Int t4[4];
    nuga_real_t Ni[3], Nj[3], Nk[3], Nl[3];
    
    for (size_t i = 0; (i < sz) && !ret; ++i)
    {
      _connT4->getEntry(boxes[i], t4);
      _coordT4.getEntry(t4[0], Ni);
      _coordT4.getEntry(t4[1], Nj);
      _coordT4.getEntry(t4[2], Nk);
      _coordT4.getEntry(t4[3], Nl);
     
      ret = K_MESH::Tetrahedron::is_inside(Ni, Nj, Nk, Nl, pt);
    }
    
    return ret;
  }
  
  ///
  template <> inline
  int maskGen::is_inside<K_MESH::Triangle>(const nuga_real_t* pt, Vector_t<E_Int>& candidates)
  {
    E_Int Ti[3], tx;
    // Vertex contact accounts for mesh precision. Ray predicates use Nuga's double precision.
    const nuga_real_t vertexTol2 = E_EPSILON * E_EPSILON;
    const nuga_real_t rayTol = EPSILON;
    nuga_real_t d2;
    
    // Ray tracing from pt using closest node
    E_Int N = _kdtree->getClosest(pt, d2);
    
    if (d2 < vertexTol2)
      return true; //lying on the mask
    
    nuga_real_t* A = _isoG.col(_ancestors[N]);
             
    candidates.clear();
    _tree->getIntersectingBoxes(pt, A, candidates, rayTol, true);//segment intersection
    
    size_t bx_sz=candidates.size();
    
#ifdef DEBUG_MASK
    /*assert (!candidates.empty());
    K_FLD::FloatArray crd;
    crd.pushBack(pt,pt+3);
    crd.pushBack(A, A+3);
    K_FLD::IntArray cnt(2,1,0); cnt(1,0)=1;
    MIO::write("ray.mesh", crd, cnt, "BAR");
    {
      E_Int Tii[3];
      K_FLD::IntArray cnt2;
      for (size_t i=0; i < bx_sz; ++i)
      {
        _connT4->getEntry(candidates[i], Tii);
        cnt2.pushBack(Tii, Tii+3);
      }
      const K_FLD::FloatArray crd2(_coordT4.array());
      MIO::write("cands.mesh", crd2, cnt2, "TRI");
    }*/
#endif
    
    nuga_real_t Q0[3], Q1[3], Q2[3], u0(K_CONST::E_MAX_FLOAT), u1, mindp(K_CONST::E_MAX_FLOAT), ptA[3];
    E_Bool overlap, tol_is_abs(1), coincident;
    size_t nb_visibles(0);
    
    NUGA::diff<3>(A, pt, ptA);
    NUGA::normalize<3>(ptA);
                
    for (size_t b=0; b < bx_sz; ++b)
    {     
      _connT4->getEntry(candidates[b], Ti);
      _coordT4.getEntry(Ti[0], Q0);
      _coordT4.getEntry(Ti[1], Q1);
      _coordT4.getEntry(Ti[2], Q2);
            
      // in order to prevent numerical errors, do a special treatment for the triangle containing A (when the line is not coincident with the plane).
      if ( (_ancestors[N] == candidates[b]) && (::fabs(NUGA::dot<3>(_normals.col(candidates[b]), ptA)) > rayTol) ) //prevent numerical errors due to isoG computation that does not fall into the plane exactly.
        u0 = 1.;
      else
      { 
        if (!K_MESH::Triangle::intersectv2<3>(Q0, Q1, Q2, pt, A, rayTol, tol_is_abs,  u0, u1, tx, overlap, coincident))
          continue;
      
        if (IS_ZERO(u0, rayTol)) // lying on the mask surface
          return 1;
        
        if (overlap || coincident) //ambiguous : overlap (but pt is not inside - otherwise u0=0. -) or coincident : touching a triangle's border
          continue;
      }
      // u0 has necessarily a value here.
      if (u0 < mindp)
      {
        nb_visibles=1;
        candidates[0]=candidates[b]; //overwrite candidates to have real candidates between rank 0 and (nb_visibles-1)
        mindp=u0;
      }
      else if (IS_ZERO(u0-mindp, rayTol))
        candidates[nb_visibles++]=candidates[b];
    }
    
    if (nb_visibles == 0)
      return -1;//ambiguous
    
    _connT4->getEntry(candidates[0], Ti);
    _coordT4.getEntry(Ti[0], Q0);
    _coordT4.getEntry(Ti[1], Q1);
    _coordT4.getEntry(Ti[2], Q2);
    
    bool orient0= (NUGA::dot<3>(_normals.col(candidates[0]), ptA) > 0.);
      
    if (nb_visibles > 1)
    {
      //check if all canddidates giev the same orientation
      for (size_t i=1; i < nb_visibles; ++i)
      {
        _connT4->getEntry(candidates[i], Ti);
        _coordT4.getEntry(Ti[0], Q0);
        _coordT4.getEntry(Ti[1], Q1);
        _coordT4.getEntry(Ti[2], Q2);
      
        if ((NUGA::dot<3>(_normals.col(candidates[i]), ptA) > 0.) != orient0)
          return -1;//ambiguous
      }
    }
      
    return orient0;
  }
   
}
#endif
