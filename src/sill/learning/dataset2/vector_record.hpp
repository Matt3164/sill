#ifndef SILL_VECTOR_RECORD2_HPP
#define SILL_VECTOR_RECORD2_HPP

#include <armadillo>
#include <iostream>

#include <sill/macros_def.hpp>

namespace sill {

  template <typename T>
  struct vector_record2 {
    arma::Col<T> values;
    T weight;

    typedef T weight_type;

    vector_record2()
      : weight(0.0) { }

    explicit vector_record2(size_t n)
      : values(n, arma::fill::zeros), weight(0.0) { }
  };

  template <typename T>
  std::ostream& operator<<(std::ostream& out, const vector_record2<T>& r) {
    foreach(T x, r.values) {
      out << x << " "; // nan is the special "undefined" value
    }
    out << ": " << r.weight;
    return out;
  }
  
} // namespace sill

#include <sill/macros_undef.hpp>

#endif