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

#ifndef KDTREE_H_
#define KDTREE_H_

#include "Nuga/include/DynArray.h"
#include "Nuga/include/defs.h"
#include "Nuga/include/ArrayAccessor.h"
#include <functional>
#include <algorithm>
#include <vector>

namespace K_SEARCH
{
///
template <typename ArrayType = K_FLD::FloatArray>
class KdTree {

  public: /** Typedefs */

    typedef KdTree                            self_type;
    typedef K_FLD::ArrayAccessor<ArrayType>   coord_access_type;
    typedef E_Int                             size_type;
    typedef nuga_real_t                           value_type;
    typedef K_FLD::IntArray                   tree_array_type;
    typedef const value_type*                 const_coord_iterator;
    typedef tree_array_type::iterator         tree_iterator;
    typedef tree_array_type::const_iterator   const_tree_iterator;

  public: /** Constructors and Destructor */

    /// Build a tree and inserts all the nodes of the input coordinate array pos.
    KdTree(const coord_access_type& posAcc, nuga_real_t tolerance=EPSILON, bool use_omp=false);

    /// Builds a tree and inserts only the valid nodes from begin to end.
    KdTree(const coord_access_type& posAcc, 
           std::vector<size_type> indices/*passed by value to preserve input*/,
	   nuga_real_t tolerance=EPSILON, bool use_omp=false);

    /// Destructor.
    ~KdTree(){};

    void build(std::vector<size_type>* indices=nullptr, nuga_real_t tolerance=EPSILON);

    void clear();
    
  public: /*Accessors */
    
    E_Int nb_nodes(){ return _tree_sz; }

  public: /** Insertion methods */

    /** Insert a node in the tree. */
    void insert(size_type N);

  public: /** Query methods */

    /// Returns a close node to a given point. It is not necessarily the closest one.
    size_type getClose(const nuga_real_t* pt) const;
    size_type getClose(const nuga_real_t* pt, nuga_real_t& d2) const;

    /// Returns a close node to node N. It is not necessarily the closest one.
    size_type getClose(size_type N) const;
    size_type getClose(size_type N, nuga_real_t& d2) const;
    size_type getClosest(E_Int n, const nuga_real_t& guessed_d2, nuga_real_t& dist2) const;

    /// Returns the global closest node.
    size_type getClosest(const nuga_real_t* pt) const;
    size_type getClosest(const nuga_real_t *pt, nuga_real_t& d2) const;
    size_type getClosest(const nuga_real_t* point, const nuga_real_t& guessed_d2, nuga_real_t& d2) const;
    // same queries with a point and distances in another real type (E_Float when it is float)
    template <typename U, NUGA_OTHER_REAL(U)>
    size_type getClosest(const U* pt) const
    { nuga_real_t P[3]; for (E_Int k = 0; k < _dim; ++k) P[k] = pt[k]; return getClosest(P); }
    template <typename U, NUGA_OTHER_REAL(U)>
    size_type getClosest(const U* pt, U& d2) const
    { nuga_real_t P[3], D; for (E_Int k = 0; k < _dim; ++k) P[k] = pt[k]; size_type r = getClosest(P, D); d2 = (U)D; return r; }
    template <typename U, NUGA_OTHER_REAL(U)>
    size_type getClosest(const U* pt, const U& guessed_d2, U& d2) const
    { nuga_real_t P[3], D; for (E_Int k = 0; k < _dim; ++k) P[k] = pt[k]; size_type r = getClosest(P, (nuga_real_t)guessed_d2, D); d2 = (U)D; return r; }
    template <typename U, NUGA_OTHER_REAL(U)>
    size_type getClosest(E_Int n, const U& guessed_d2, U& d2) const
    { nuga_real_t D; size_type r = getClosest(n, (nuga_real_t)guessed_d2, D); d2 = (U)D; return r; }
    template <typename U, NUGA_OTHER_REAL(U)>
    size_type getClosest(size_type N, U& d2) const
    { nuga_real_t D; size_type r = getClosest(N, D); d2 = (U)D; return r; }

    /// Returns the global closest node to node of index N.
    size_type getClosest(E_Int N) const;
    size_type getClosest(size_type N, nuga_real_t& d2) const;

    /// Returns all the nodes in the input box by appending the vector 'out'.
    /** Warning: out is not cleared upon entry.*/
    void getInBox(const nuga_real_t* minB, const nuga_real_t* maxB, std::vector<size_type>& out) const;
    template <typename U, NUGA_OTHER_REAL(U)>
    void getInBox(const U* minB, const U* maxB, std::vector<size_type>& out) const
    { nuga_real_t m[3], M[3]; for (E_Int k = 0; k < _dim; ++k) { m[k] = minB[k]; M[k] = maxB[k]; } getInBox(m, M, out); }

    /// Returns all the nodes in the input sphere centered on C by appending the vector 'out'.
    /** Warning: out is not cleared upon entry.*/
    void getInSphere(const nuga_real_t* C, nuga_real_t radius, std::vector<size_type>& out) const;
    void getInSphere(const nuga_real_t* C, nuga_real_t radius, std::vector<size_type>& out, std::vector<nuga_real_t>& d2) const;
    template <typename U, NUGA_OTHER_REAL(U)>
    void getInSphere(const U* C, U radius, std::vector<size_type>& out) const
    { nuga_real_t P[3]; for (E_Int k = 0; k < _dim; ++k) P[k] = C[k]; getInSphere(P, (nuga_real_t)radius, out); }
    template <typename U, NUGA_OTHER_REAL(U)>
    void getInSphere(const U* C, U radius, std::vector<size_type>& out, std::vector<U>& d2) const
    { nuga_real_t P[3]; for (E_Int k = 0; k < _dim; ++k) P[k] = C[k]; std::vector<nuga_real_t> D; getInSphere(P, (nuga_real_t)radius, out, D); d2.assign(D.begin(), D.end()); }
    /// Returns all the nodes in the input sphere centered on node N by appending the vector 'out'.
    /** Warning: out is not cleared upon entry.*/
    void getInSphere(E_Int N, nuga_real_t radius, std::vector<size_type>& out) const;
    void getInSphere(E_Int N, nuga_real_t radius, std::vector<size_type>& out, std::vector<nuga_real_t>& d2) const;
    template <typename U, NUGA_OTHER_REAL(U)>
    void getInSphere(E_Int N, U radius, std::vector<size_type>& out, std::vector<U>& d2) const
    { std::vector<nuga_real_t> D; getInSphere(N, (nuga_real_t)radius, out, D); d2.assign(D.begin(), D.end()); }

  private:
    /// Underneath method to insert a node in an existing tree (assuming that _tree has the right size).
    void __insert(size_type N);

    /// Underneath algorithm for creating a tree with a set of nodes (balanced construction).
    template <typename InputIterator>
    E_Int __insert(InputIterator begin, InputIterator end, size_type depth);
    template <typename InputIterator>
    void __insert_omp(InputIterator begin, InputIterator end, size_type depth);
    template <typename InputIterator>
    E_Int __set_tree(InputIterator begin, InputIterator end);

    /// Underneath algorithm for the getClosest method.
    void __seek_closest(const nuga_real_t *pt, size_type cur_col, size_type axis, nuga_real_t& d2, size_type& m) const;
    
    /// Underneath algorithm for the getClosest method (closest to n).
    void __seek_closest(size_type n, const nuga_real_t *Xn, size_type cur_col, size_type axis, nuga_real_t& d2, size_type& m) const ;
    
    /// Underneath algorithm for the getClose method.
    void __getClosest_through_path(const nuga_real_t* pt, size_type & m, nuga_real_t& d2) const;
    
    /// Underneath algorithm for the getClose method (close to node).
    void __getClosest_through_path(size_type n, const nuga_real_t *Xn, size_type & m, nuga_real_t& d2) const;
    
    /// Underneath algorithm for the getInBox method.
    void __getInBox(size_type ci, size_type axis, const nuga_real_t* mBox, const nuga_real_t* MBox, std::vector<size_type>& out) const;

  private:

    /// accessor on coordinates
    const coord_access_type& _posAcc;

    /// kd tree
    tree_array_type	_tree;

    /// current tree size
    size_type       _tree_sz;

    /// dimension (nb of coordinates)
    size_type       _dim;

    /// tolerance
    nuga_real_t         _tolerance;

    /// to extract the coordinates.
    mutable nuga_real_t _Xn[3], _mB[3], _MB[3];
    
}; // End class KdTree

} // end namespace

#include "Nuga/include/KdTree.cxx"

#endif /* KDTREE_H_ */
