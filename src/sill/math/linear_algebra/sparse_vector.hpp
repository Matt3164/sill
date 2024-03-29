
#ifndef _SILL_SPARSE_VECTOR_HPP_
#define _SILL_SPARSE_VECTOR_HPP_

#include <sill/math/linear_algebra/armadillo.hpp>
#include <sill/math/linear_algebra/sparse_vector_view.hpp>
#include <sill/math/statistics.hpp>

namespace sill {

  // Forward declarations
  template <typename T, typename SizeType> class sparse_vector_view;
  /*
  template <typename T> class vector;
  template <typename T> void vector<T>::save(oarchive& ar) const;
  template <typename T> void vector<T>::load(iarchive& ar);
  */

  /**
   * Sparse host/device vector class.
   *
   * @tparam T        Type of data element (e.g., float).
   * @tparam SizeType    Type of index (e.g., arma::u32).
   */
  template <typename T, typename SizeType = arma::u32>
  class sparse_vector
    : public vector_base<T,SizeType> {

    // Public types
    //==========================================================================
  public:

    typedef vector_base<T,SizeType> base;

    typedef T                 value_type;
    typedef SizeType          size_type;
    typedef const T*          const_iterator;
    typedef T*                iterator;
    typedef const size_type*  const_index_iterator;
    typedef size_type*        index_iterator;

    typedef uvec              index_vector_type;

    // Constructors
    //==========================================================================

    //! Constructs an empty vector.
    sparse_vector()
      : base(0), sorted_(true) { }

    //! Constructs a vector with n elements but NO non-zero elements.
    explicit sparse_vector(size_type n)
      : base(n), indices_((size_type)0), values_((size_type)0), sorted_(true) {
    }

    //! Constructs a vector with n elements and k non-zero elements,
    //! but does not initialize them.
    //! WARNING: You must initialize the values before calling other methods
    //!          (else those methods might fail due to duplicate indices).
    sparse_vector(size_type n, size_type k)
      : base(n), indices_(k), values_(k), sorted_(false) {
      assert(k <= n);
    }

    /**
     * Constructs a vector with the given non-zero elements.
     * @param n         Size of vector.
     * @param indices_  Indices of non-zeros elements.
     * @param values_   Corresponding values of non-zero elements.
     */
    template <typename SizeTypeVecType, typename ValueVecType>
    sparse_vector(size_type n, const SizeTypeVecType& indices_,
                  const ValueVecType& values_)
      : base(n),
        indices_(arma::conv_to<arma::Col<size_type> >::from(indices_)),
        values_(arma::conv_to<arma::Col<value_type> >::from(values_)),
        sorted_(false) {
      sort_indices();
    }

    //! Constructor from a view of another type.
    template <typename OtherT, typename OtherSizeType>
    sparse_vector(const sparse_vector_view<OtherT, OtherSizeType>& other)
      : base(other),
        indices_(other.num_non_zeros()), values_(other.num_non_zeros()),
        sorted_(false) {
      for (size_type i = 0; i < other.num_non_zeros(); ++i) {
        indices_[i] = static_cast<size_type>(other.index(i));
        values_[i] = static_cast<value_type>(other.value(i));
      }
      sort_indices();
    }

    //! Constructor from a dense vector.
    template <typename OtherT>
    sparse_vector(const arma::Col<OtherT>& other)
      : base(other.size()),
        indices_(sequence(0, other.size())), values_(other), sorted_(true) {
    }

    /**
     * Reset this vector to have the given non-zero elements.
     * @param n         Size of vector.
     * @param indices_  Indices of non-zeros elements.
     * @param values_   Corresponding values of non-zero elements.
     */
    template <typename SizeTypeVecType, typename ValueVecType>
    void reset(size_type n, const SizeTypeVecType& indices_,
               const ValueVecType& values_) {
      n_ = n;
      this->indices_ = arma::conv_to<arma::Col<size_type> >::from(indices_);
      this->values_ = arma::conv_to<arma::Col<value_type> >::from(values_);
      sorted_ = false;
      sort_indices();
    }

    // Serialization
    //==========================================================================

    void save(oarchive& ar) const {
      base::save(ar);
      ar << indices_ << values_ << sorted_;
    }

    void load(iarchive& ar) {
      base::load(ar);
      ar >> indices_ >> values_ >> sorted_;
    }

    // Getters and setters: dimensions
    //==========================================================================

    using base::length;
    using base::size;

    //! Number of non-zero elements.
    size_type num_non_zeros() const {
      return indices_.size();
    }

    //! Resizes for n elements and k non-zeros but does not initialize the
    //! elements.
    //! NOTE: This does NOT currently retain old values.
    void resize(size_type n, size_type k) {
      this->n_ = n;
      indices_.set_size(k);
      values_.set_size(k);
      sorted_ = false;
    }

    //! Sets the size of the vector to n, with no non-zeros.
    //! Note: This clears all data from this vector.
    void set_size(size_type n) {
      this->n_ = n;
      indices_.set_size(0);
      values_.set_size(0);
      sorted_ = true;
    }

    // Getters and setters: values
    //==========================================================================

    //! Return element i.
    //! NOTE: If you call this method many times, you should call sort_indices()
    //!       beforehand.
    value_type operator()(size_type i) const {
      assert(i < size());
      if (sorted_) {
        size_type k = find_index(i);
        if (k != (size_type)(-1))
          return values_[k];
      } else {
        for (size_type k = 0; k < indices_.size(); ++k) {
          if (indices_[k] == i)
            return values_[k];
        }
      }
      return 0;
    }

    //! Return a mutable reference to element i.
    //! WARNING: This is SLOW if element i used to be zero.
    value_type& operator()(size_type i) {
      assert(i < size());
      if (sorted_) {
        size_type k = find_index(i);
        if (k != (size_type)(-1))
          return values_[k];
      } else {
        for (size_type k = 0; k < indices_.size(); ++k) {
          if (indices_[k] == i)
            return values_[k];
        }
      }
      // Insert new non-zero element.
      sorted_ = false;
      size_type oldsize = indices_.size();
      indices_.reshape(oldsize+1, 1);
      values_.reshape(oldsize+1, 1);
      indices_[oldsize] = i;
      values_[oldsize] = 0;
      return values_[oldsize];
    }

    //! Return element i.
    //! NOTE: If you call this method many times, you should call sort_indices()
    //!       beforehand.
    value_type operator[](size_type i) const {
      return this->operator()(i);
    }

    //! Return a mutable reference to element i.
    //! WARNING: This is SLOW if element i used to be zero.
    value_type& operator[](size_type i) {
      return this->operator()(i);
    }

    //! Returns a sparse subvector.
    sparse_vector operator()(const index_vector_type& ind) const {
      std::vector<size_type> subinds;
      std::vector<value_type> subvals;
      for (size_type k = 0; k < ind.size(); ++k) {
        if (this->operator()(ind[k]) != 0) {
          subinds.push_back(k);
          subvals.push_back(this->operator()(ind[k]));
        }
      }
      return sparse_vector(ind.size(), subinds, subvals);
    }

    //! Return the index for the i^th non-zero element.
    //! WARNING: This is not bound-checked.
    size_type index(size_type i) const { return indices_[i]; }

    //! Return a mutable reference to the index for the i^th non-zero element.
    //! WARNING: This is not bound-checked.
    size_type& index(size_type i) {
      sorted_ = false;
      return indices_[i];
    }

    //! Return the value for the i^th non-zero element.
    //! WARNING: This is not bound-checked.
    value_type value(size_type i) const { return values_[i]; }

    //! Return a mutable reference to the value for the i^th non-zero element.
    //! WARNING: This is not bound-checked.
    value_type& value(size_type i) { return values_[i]; }

    //! Indices of non-zeros.
    const arma::Col<size_type>& indices() const { return indices_; }

    //! Indices of non-zeros.
    arma::Col<size_type>& indices() { return indices_; }

    //! Values of non-zeros.
    const arma::Col<value_type>& values() const { return values_; }

    //! Values of non-zeros.
    arma::Col<value_type>& values() { return values_; }

    //! Get a const iterator to the beginning of the indices.
    const_index_iterator begin_indices() const { return indices_.begin(); }

    //! Get a const iterator to the end of the indices.
    const_index_iterator end_indices() const { return indices_.end(); }

    //! Get a const iterator to the beginning of the values.
    const_iterator begin_values() const { return values_.begin(); }

    //! Get a const iterator to the end of the values.
    const_iterator end_values() const { return values_.end(); }

    //! Indicates whether the indices are sorted.
    bool sorted() const { return sorted_; }

    //! Indicates whether the indices are sorted.
    //! WARNING: Modify this bit with caution!
    bool& sorted_mutable() { return sorted_; }

    // Utilities
    //==========================================================================

    //! Sorts indices in increasing order for faster accesses.
    //! This operation is sometimes called silently by other methods.
    void sort_indices() const {
      assert(indices_.size() == values_.size());
      if (sorted_)
        return;
      index_vector_type
        sorted_ind(sorted_indices<arma::Col<size_type>,index_vector_type>
                   (indices_));
      indices_ = indices_(sorted_ind);
      values_ = values_(sorted_ind);
      // Check validity.
      if (indices_.size() != 0) {
        if (indices_[indices_.size()-1] >= n_) {
          std::cerr << "sparse_vector given non-zero index "
                    << indices_[indices_.size()-1]
                    << " too large for vector of size " << n_ << std::endl;
          assert(false);
        }
        for (size_type k = 1; k < indices_.size(); ++k) {
          if (indices_[k-1] == indices_[k]) {
            std::cerr << "sparse_vector given duplicate non-zero indices "
                      << indices_[k] << std::endl;
            assert(false);
          }
        }
      }
      sorted_ = true;
    }

    //! Swap this vector with another efficiently.
    //! NOTE: This swaps pointers, so any iterators will be invalidated.
    void swap(sparse_vector& other) {
      indices_.swap(other.indices_);
      values_.swap(other.values_);
      std::swap(sorted_, other.sorted_);
    }

    //! Print to the given output stream.
    void print(std::ostream& out) const {
      out << "[n=" << size() << "; ";
      for (size_type i(0); i < num_non_zeros(); ++i)
        out << index(i) << "(" << value(i) << ") ";
      out << "]";
    }

    // Comparisons
    //==========================================================================

    //! Comparison
    bool operator==(const sparse_vector& other) const {
      if (size() != other.size())
        return false;
      sort_indices();
      other.sort_indices();
      return (equal(indices_, other.indices_) &&
              equal(values_, other.values_));
    }

    //! Comparison
    bool operator!=(const sparse_vector& other) const {
      return !operator==(other);
    }

    //! If one vector is shorter, it is the lesser value.
    //! Otherwise, this uses a lexigraphical comparison.
    //! @todo Make this more efficient!
    bool operator<(const sparse_vector& other) const {
      if (size() != other.size()) {
        return (size() < other.size());
      } else {
        sort_indices();
        other.sort_indices();
        size_type myk = 0;
        size_type otherk = 0;
        while (myk < num_non_zeros() && otherk < other.num_non_zeros()) {
          if (index(myk) < other.index(otherk)) {
            if (value(myk) != 0)
              return (value(myk) < 0);
            ++myk;
          } else if (index(myk) > other.index(otherk)) {
            if (other.value(otherk) != 0)
              return (0 < other.value(otherk));
            ++otherk;
          } else {
            if (value(myk) != other.value(otherk))
              return (value(myk) < other.value(otherk));
            ++myk;
            ++otherk;              
          }
        }
        return false;
      }
    }

    // Operations
    //==========================================================================

    //! Multiplication with a scalar.
    sparse_vector& operator*=(T val) {
      values_ *= val;
      return *this;
    }

    //! Set to all zeros.
    void zeros() {
      indices_.set_size(0);
      values_.set_size(0);
    }

    //! Resize to length n, and set to all zeros.
    void zeros(size_type n) {
      this->n_ = n;
      indices_.set_size(0);
      values_.set_size(0);
      sorted_ = true;
    }

    // Methods to support optimization routines
    //==========================================================================

    //! Element-wise multiplication with another vector of the same size.
    //! @todo Get rid of non-zeros which become 0.
    sparse_vector& elem_mult(const sparse_vector& other) {
      assert(size() == other.size());
      sort_indices();
      other.sort_indices();
      // TO DO: MAKE THIS RUN IN TIME O(n) instead of O(n log n).
      for (size_type k = 0; k < indices_.size(); ++k)
        values_[k] *= other[indices_[k]];
      return *this;
    }

    //! Returns the L1 norm.
    T L1norm() const {
      return norm(values_,1);
    }

    //! Returns the L2 norm.
    T L2norm() const {
      return norm(values_,2);
    }

    // Protected data and methods
    //==========================================================================
  protected:

    using base::n_;

    //! Indices of non-zeros.
    mutable arma::Col<size_type> indices_;

    //! Values of non-zeros.
    mutable arma::Col<value_type> values_;

    //! Indicates if the indices are sorted in increasing order.
    mutable bool sorted_;

    //! Find index i via binary search; this assumes sorted_ == true.
    //! @return  SizeType of i in indices_, or (size_type)(-1) if not found.
    size_type find_index(size_type i) const {
      size_type lower = 0;
      size_type upper = indices_.size();
      while (lower < upper) {
        size_type mid = (lower + upper) / 2;
        if (indices_[mid] < i) {
          lower = mid + 1;
        } else if (indices_[mid] > i) {
          upper = mid;
        } else {
          return mid;
        }
      }
      return (size_type)(-1);
    }

  }; // class sparse_vector

  template <typename T, typename SizeType>
  std::ostream&
  operator<<(std::ostream& out, const sparse_vector<T,SizeType>& vec) {
    vec.print(out);
    return out;
  }

}; // namespace sill

#endif // #ifndef _SILL_SPARSE_VECTOR_HPP_
