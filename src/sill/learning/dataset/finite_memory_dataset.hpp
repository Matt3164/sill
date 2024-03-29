#ifndef SILL_FINITE_MEMORY_DATASET_HPP
#define SILL_FINITE_MEMORY_DATASET_HPP

#include <sill/learning/dataset/finite_dataset.hpp>
#include <sill/learning/dataset/slice_view.hpp>
#include <sill/math/permutations.hpp>

#include <algorithm>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  // Forward declarations
  template <typename T> class hybrid_memory_dataset;

  /**
   * A dataset that stores observations for finite variables in memory.
   * Models Dataset, InsertableDataset, and SliceableDataset.
   *
   * \todo replace boost::shared_ptr with std::unique_ptr
   */
  class finite_memory_dataset : public finite_dataset, boost::noncopyable {
  public:
    // SliceableDataset concept typedefs
    typedef slice_view<finite_dataset> slice_view_type;

    // Bring the record(row) implementation up to this class
    using finite_dataset::record;
    
    //! Creates an uninitialized dataset
    finite_memory_dataset() { } 

    /**
     * Initializes the dataset with the given sequence of variables
     * and pre-allocates memory for the given number of rows.
     * It is an error to call initialize() more than once.
     */
    void initialize(const finite_var_vector& variables, size_t capacity = 1) {
      if (data) {
        throw std::logic_error("Attempt to call initialize() more than once.");
      }
      finite_dataset::initialize(variables);
      num_allocated = std::max(capacity, size_t(1));
      num_inserted = 0;
      num_cols = variables.size();
      data.reset(new size_t[num_allocated * num_cols]);
      weights.reset(new double[num_allocated]);
      col_ptr.resize(variables.size());
      for (size_t i = 0; i < variables.size(); ++i) {
        arg_index[variables[i]] = i;
        col_ptr[i] = data.get() + num_allocated * i;
      }
    }

    /**
     * Initializes the dataset with the given sequence of variables
     * and pre-allocates memory for the given number of rows.
     * It is an error to call initialize() more than once.
     */
    void initialize(const finite_domain& variables, size_t capacity = 1) {
      initialize(make_vector(variables), capacity);
    }

    size_t size() const {
      return num_inserted;
    }

    size_t capacity() const {
      return num_allocated;
    }

    void reserve(size_t new_capacity) {
      if (new_capacity > num_allocated) {
        reallocate(new_capacity); // private function
      }
    }

    finite_record record(size_t row, const finite_var_vector& vars) const {
      assert(row < num_inserted);
      finite_record result(vars, weights[row]);
      for (size_t i = 0; i < vars.size(); ++i) {
        result.values[i] = col_ptr[safe_get(arg_index, vars[i])][row];
      }
      return result;
    }

    //! Returns a view representing a contiguous range of rows
    slice_view<finite_dataset> subset(size_t begin, size_t end) {
      return slice_view<finite_dataset>(this, slice(begin, end));
    }

    //! Returns a view representing a contiguous range of rows
    slice_view<finite_dataset> subset(const slice& s) {
      return slice_view<finite_dataset>(this, s);
    }

    //! Returns a view of representing a union of row ranges
    slice_view<finite_dataset> subset(const std::vector<slice>& s) {
      return slice_view<finite_dataset>(this, s);
    }

    //! Inserts the values in this dataset's ordering.
    void insert(const finite_record& r) {
      check_initialized();
      insert(r.values, r.weight); // protected function
    }
 
    //! Inserts a new row from an assignment (all variables must be present).
    void insert(const finite_assignment& a, double weight = 1.0) {
      check_initialized();
      std::vector<size_t> values;
      values.reserve(num_cols);
      foreach(finite_variable* v, args) {
        values.push_back(safe_get(a, v));
      }
      insert(values, weight); // protected function
    }

    //! Inserts the given number of rows with unit weights and "undefined" values.
    void insert(size_t nrows) {
      check_initialized();
      std::vector<size_t> values(args.size(), -1);
      for (size_t i = 0; i < nrows; ++i) {
        insert(values, 1.0); // protected function
      }
    }

    //! Randomly permutes the rows
    template <typename RandomNumberGenerator>
    void shuffle(RandomNumberGenerator& rng) {
      permute(randperm(num_inserted, rng));
    }

    //! Swaps this dataset with the other
    void swap(finite_memory_dataset& ds) {
      finite_dataset::swap(ds);
      std::swap(arg_index, ds.arg_index);
      std::swap(data,      ds.data);
      std::swap(weights,   ds.weights);
      std::swap(col_ptr,   ds.col_ptr);
      std::swap(num_allocated, ds.num_allocated);
      std::swap(num_inserted,  ds.num_inserted);
      std::swap(num_cols,      ds.num_cols);
    }

    // Protected functions
    //========================================================================
  protected:
    //! Throws an exception if the dataset is not initialized
    void check_initialized() const {
      if (!data) {
        throw std::logic_error("The dataset is not initialized!");
      }
    }

    //! The common implementation of the insert() function
    void insert(const std::vector<size_t>& values, double weight) {
      assert(num_inserted <= num_allocated);
      if (num_inserted == num_allocated) {
        reallocate(2 * num_allocated);
      }

      assert(values.size() == num_cols);
      for (size_t i = 0; i < num_cols; ++i) {
        col_ptr[i][num_inserted] = values[i];
      }
      weights[num_inserted] = weight;
      ++num_inserted;
    }

    //! Reorders the rows according the given permutation
    void permute(const std::vector<size_t>& permutation) {
      assert(permutation.size() == num_inserted);
      finite_memory_dataset ds;
      ds.initialize(args, num_inserted);
      std::vector<size_t> values(num_cols);
      for (size_t row = 0; row < num_inserted; ++row) {
        size_t prow = permutation[row];
        for (size_t col = 0; col < num_cols; ++col) {
          values[col] = col_ptr[col][prow];
        }
        ds.insert(values, weights[prow]);
      }
      swap(ds);
    }

    aux_data* init(const finite_var_vector& args,
                   iterator_state_type& state) const {
      check_initialized();
      state.elems.resize(args.size());
      for (size_t i = 0; i < args.size(); ++i) {
        state.elems[i] = col_ptr[safe_get(arg_index, args[i])];
      }
      state.weights = weights.get();
      state.e_step.assign(args.size(), 1);
      state.w_step = 1;
      return NULL;
      // hybrid_memory_dataset depends on this being NULL
      // if this ever changes, so should hybrid_memory_dataset
    }
    
    void advance(ptrdiff_t diff,
                 iterator_state_type& state,
                 aux_data* data) const {
      for (size_t i = 0; i < state.elems.size(); ++i) {
        state.elems[i] += diff; // step is always one for finite_memory_dataset
      }
      state.weights += diff;
    }
    
    size_t load(size_t n,
                iterator_state_type& state,
                aux_data* data) const {
      return std::min(n, size_t(weights.get() + num_inserted - state.weights));
    }

    void save(iterator_state_type& state, aux_data* data) { }

    void print(std::ostream& out) const {
      out << "finite_memory_dataset(N=" << size() << ", args=" << args << ")";
    }

    // friends
    template <typename T> friend class hybrid_memory_dataset;

    // Private data members
    //========================================================================
  private:
    // increases the storage capacity to new_capacity and copies the data
    void reallocate(size_t new_capacity) {
      // allocate the new data
      size_t* new_data = new size_t[new_capacity * num_cols];
      double* new_weights = new double[new_capacity];
      std::vector<size_t*> new_col_ptr(args.size());
      for (size_t i = 0; i < args.size(); ++i) {
        new_col_ptr[i] = new_data + new_capacity * i;
      }

      // copy the elements and weights to the new locations
      for (size_t i = 0; i < args.size(); ++i) {
        std::copy(col_ptr[i], col_ptr[i] + num_inserted, new_col_ptr[i]);
      }
      std::copy(weights.get(), weights.get() + num_inserted, new_weights);

      // swap the old and the new data
      data.reset(new_data);
      weights.reset(new_weights);
      col_ptr.swap(new_col_ptr);
      num_allocated = new_capacity;
    }

    // finite_var_vector args;  // moved to the base class
    std::map<finite_variable*, size_t> arg_index; // the index of each var
    boost::shared_ptr<size_t[]> data;    // the data storage
    boost::shared_ptr<double[]> weights; // the weights storage
    std::vector<size_t*> col_ptr;        // pointers to the elements
    size_t num_allocated;                // the number of allocated rows
    size_t num_inserted;                 // the number of inserted rows
    size_t num_cols;                     // the number of columns

  }; // class finite_memory_dataset

} // namespace sill

#include <sill/macros_undef.hpp>

#endif
