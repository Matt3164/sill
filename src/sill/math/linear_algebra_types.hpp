
#ifndef _SILL_LINEAR_ALGEBRA_TYPES_HPP_
#define _SILL_LINEAR_ALGEBRA_TYPES_HPP_

#include <sill/math/sparse_linear_algebra/sparse_linear_algebra.hpp>

/**
 * \file linear_algebra_types.hpp  Dense/sparse linear algebra specifications.
 *
 * This file contains structs which are passed to methods as template
 * parameters in order to specify what vector/matrix classes should be used.
 * SILL provides dense and sparse versions, but the user may implement and
 * pass in others.
 *
 * STANDARD: Classes which take a linear algebra specifier as a template
 *           parameter (or have one hard-coded) should typedef the specifier
 *           as "la_type" as a standard name for other classes to use.
 */

namespace sill {

  //! Dense linear algebra specification.
  template <typename T = double, typename SizeType = size_t>
  struct dense_linear_algebra {

    typedef vector<T>  vector_type;
    typedef matrix<T>  matrix_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::size_type  size_type;

    typedef vector<T>  dense_vector_type;
    typedef matrix<T>  dense_matrix_type;

    typedef ivec  index_vector_type;

  };

  //! Dense linear algebra specification.
  template <typename T = double, typename SizeType = size_t>
  struct sparse_linear_algebra {

    typedef sparse_vector<T,SizeType>  vector_type;
    typedef csc_matrix<T,SizeType>     matrix_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::size_type  size_type;

    typedef vector<T>  dense_vector_type;
    typedef matrix<T>  dense_matrix_type;

    typedef ivec  index_vector_type;

  };

} // namespace sill

#endif // #ifndef _SILL_LINEAR_ALGEBRA_TYPES_HPP_