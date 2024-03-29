#ifndef SILL_CANONICAL_TABLE_HPP
#define SILL_CANONICAL_TABLE_HPP

#include <map>
#include <algorithm>
#include <iosfwd>
#include <sstream>

#include <boost/bind.hpp>

#include <sill/base/finite_assignment.hpp>
#include <sill/base/finite_assignment_iterator.hpp>
#include <sill/math/logarithmic.hpp>
#include <sill/datastructure/dense_table.hpp>
#include <sill/global.hpp>
#include <sill/factor/factor.hpp>
#include <sill/factor/table_factor.hpp>

#include <sill/functional.hpp>
#include <sill/math/is_finite.hpp>
#include <sill/range/algorithm.hpp>
#include <sill/range/forward_range.hpp>
#include <sill/serialization/serialize.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  /**
   * A table factor represents a function of a set of finite variables.
   *
   * \ingroup factor_types
   * \see Factor
   */
  class canonical_table : public factor {
 
    // Public type declarations
    //==========================================================================
  public:
    //! implements Factor::result_type
    typedef logarithmic<double> result_type;

    typedef dense_table<result_type> table_type;

    //! implements Factor::domain_type
    typedef finite_domain domain_type;

    //! implements Factor::variable_type
    typedef finite_variable variable_type;

    //! implements Factor::assignment_type
    typedef finite_assignment assignment_type;

    //! implements Factor::record_type
    typedef finite_record_old record_type;

    // Private data members
    //==========================================================================
  private:
    //! The type of shape / index of the underlying table
    typedef table_type::index_type index_type;

    //! The type that maps variables to table indices
    typedef std::map<finite_variable*, size_t> var_index_map;

    //! The arguments of this factor.
    finite_domain args;

    //! A mapping from the arguments' identifiers to \f$[1, ..., k)\f$.
    var_index_map var_index;

    //! A mapping from the dimensions to the arguments.
    finite_var_vector arg_seq;

    /**
     * The table used to store the factor's values.  The dimensions of
     * this table are ordered such that variable v corresponds to
     * dimension var_index[i].
     */
    table_type table_data;


    // Private helper functions
    //==========================================================================
    /**
     * Initializes this table factor to have the supplied collection of
     * arguments and to have a constant value.
     */
    void initialize(const forward_range<finite_variable*>& arguments,
                    result_type default_value);

    //! Fills in the local table coordinates according to the assignment
    void get_shape_from_assignment( const finite_assignment& a,
                                    index_type& s) const{
      for(size_t i = 0; i<arg_seq.size(); i++){
        finite_assignment::const_iterator
              var_i_value_iterator = a.find(arg_seq[i]);
        assert(var_i_value_iterator != a.end());
        s[i] = var_i_value_iterator->second;
      }
    }

    //! Creates an object that maps indices of one table to another
    static index_type make_dim_map(const finite_var_vector& vars,
                                   const var_index_map& to_map);

    //! Creates an object that maps indices of a table to fixed values
    static index_type make_restrict_map(const finite_var_vector& vars,
                                        const finite_assignment& a);

    //! Creates an object that maps indices of a set to 0..(n-1)
    static var_index_map make_index_map(const finite_domain& vars);
    // Constructors and conversion operators
    //==========================================================================
  public:

    //! Serialize members
    void save(oarchive & ar) const;

    //! Deserialize members
    void load(iarchive & ar);

    //! Default constructor for a factor with no arguments, i.e.,  a constant.
    explicit canonical_table(double default_value = 0.0) {
      initialize(this->arguments(), result_type(default_value));
    }

    explicit canonical_table(result_type default_value) {
      initialize(this->arguments(), default_value);
    }

    //! Creates a factor with the specified arguments. The table
    //! geometry will respect the specified order of arguments.
    canonical_table(const forward_range<finite_variable*>& arguments,
                     result_type default_value)
      : args(boost::begin(arguments), boost::end(arguments)) {
      initialize(arguments, default_value);
    }

    explicit canonical_table(const finite_var_vector& arguments,
                          result_type default_value = 0.0)
      : args(arguments.begin(), arguments.end()) {
      initialize(arguments, default_value);
    }

    canonical_table(const finite_var_vector& arguments,
                     const std::vector<result_type>& values)
      : args(arguments.begin(), arguments.end()) {
      initialize(arguments, 0);
      assert(table().size() == values.size());
      sill::copy(values, table_data.begin());
    }

    canonical_table(const canonical_table& factor) {
      (*this) = factor;
    }


    //! Conversion from a table_factor.
    explicit canonical_table(const table_factor& f)
      : args(f.arguments()), arg_seq(f.arg_vector()),
        table_data(f.table().shape()) {
      var_index.clear();
      for (size_t i = 0; i < arg_seq.size(); ++i)
        var_index[arg_seq[i]] = i;
      sill::copy(f.table(), table_data.begin());
    }

//     //! Conversion to a constant factor. The argument set of this factor
//     //! must be empty (otherwise, an assertion violation is thrown).
//     operator constant_factor() const {
//       assert(this->arguments().empty());
//       return constant_factor(*table_data.begin());
//     }

//     //! Conversion to human-readable representation
//     operator std::string() const {
//       std::ostringstream out; out << *this; return out.str();
//     }

    //! Exchanges the content of two factors
    void swap(canonical_table& f);

    // Accessors and comparison operators
    //==========================================================================

    //! Returns the argument set of this factor
    const finite_domain& arguments() const {
      return args;
    }

    //! Returns the underlying table // warning! this is not very safe!
    const table_type& table() const { 
      return table_data;
    }

    //! Returns the underlying table
    table_type& table()  {
      return table_data;
    }

    //! Returns the total number of elements (zero or non-zero) of the factor
    size_t size() const {
      return table_data.size();
    }

    //! Returns the arguments of the factor in the natural order
    const finite_var_vector& arg_vector() const {
      return arg_seq;
    }

    //! Returns the values of the factor in a linear order
    std::pair<table_type::const_iterator,
              table_type::const_iterator>
    values() const {
      return table_data.elements();
    }

    //! Returns the values of the factor in a linear order
    std::pair<table_type::iterator,
              table_type::iterator>
    values() {
      return table_data.elements();
    }
    
    /**
     * Returns the range over the set of assignments.
     * The order of the assignments is determined by the order of the variables
     * in arg_vector(), counting from lowest to highest with the first variable
     * being the most significant digit.
     */
    finite_assignment_range assignments() const;
    
    result_type operator()(const finite_assignment& a) const {
      return v(a);
    }
    result_type operator()(size_t i) const {
      return v(i);
    }
    result_type operator()(size_t i, size_t j) const {
      return v(i, j);
    }

    result_type& operator()(const finite_assignment& a) {
      return v(a);
    }
    result_type& operator()(size_t i) {
      return v(i);
    }
    result_type& operator()(size_t i, size_t j) {
      return v(i, j);
    }

    // Getters
    // -------------------------------------------------------------------
    //! Returns the value associated with a given assignment of variables
    result_type v(const finite_assignment& a) const {
      index_type index(arg_seq.size());
      get_shape_from_assignment(a,index);
      return table_data(index);
    }

    result_type& v(const finite_assignment& a) {
      index_type index(arg_seq.size());
      get_shape_from_assignment(a,index);
      return table_data(index);
    }

    //! Returns a mutable reference to the value associated with an assignment
    double logv(const finite_assignment& a) const {
      return std::log(v(a));
    }
    //! direct indexing for 2 arguments
    result_type v(size_t i, size_t j) const {
      assert(arguments().size()==2);
      index_type index(2);
      index[0] = i;
      index[1] = j;
      return table_data(index);
    }
    
    result_type& v(size_t i, size_t j) {
      assert(arguments().size()==2);
      index_type index(2);
      index[0] = i;
      index[1] = j;
      return table_data(index);
    }

    //! direct indexing for 2 arguments
    double logv(size_t i, size_t j) const {
      return std::log(v(i, j));
    }
 
     //! direct indexing for 1 argument
    result_type v(size_t i) const {
      assert(arguments().size()==1);
      index_type index(1);
      index[0] = i;
      return table_data(index);
    }
    
    //! direct indexing for 1 argument
    result_type& v(size_t i) {
      assert(arguments().size()==1);
      index_type index(1);
      index[0] = i;
      return table_data(index);
    }

    //! direct indexing for 1 argument
    double logv(size_t i) const {
      return std::log(v(i));
    }



    // Setters
    // -------------------------------------------------------------------
    //! Returns the value associated with a given assignment of variables
    void set_v(const finite_assignment& a, result_type v) {
      index_type index(arg_seq.size());
      get_shape_from_assignment(a,index);
      table_data(index) = v;
    }

    //! Returns a mutable reference to the value associated with an assignment
    void set_logv(const finite_assignment& a, double v) {
      set_v(a, logarithmic<double>(v, log_tag()));
    }
    //! direct indexing for 2 arguments
    void set_v(size_t i, size_t j, result_type v) {
      assert(arguments().size()==2);
      index_type index(2);
      index[0] = i;
      index[1] = j;
      table_data(index) = v;
    }
    //! direct indexing for 2 arguments
    void set_logv(size_t i, size_t j, double v) {
      set_v(i, j, logarithmic<double>(v, log_tag()));
    }
 
     //! direct indexing for 1 argument
    void set_v(size_t i, result_type v) {
      assert(arguments().size()==1);
      index_type index(1);
      index[0] = i;
      table_data(index) = v;
    }

    //! direct indexing for 1 argument
    void set_logv(size_t i, double v) {
      set_v(i, logarithmic<double>(v, log_tag()));
    }

    //! Returns true if the two factors have the same argument sets and values
    bool operator==(const canonical_table& other) const;

    //! Assignment operator
    canonical_table& operator=(const canonical_table& other) {
      args = other.args;
      var_index = other.var_index;
      arg_seq = other.arg_seq;
      table_data = other.table_data;
      return *this;
    }

    //! Returns true if the two factors do not have the same arguments or values
    bool operator!=(const canonical_table& other) const {
      return !(*this == other);
    }

//     //! Returns true if *this precedes other in the lexicographical ordering
//     //! The lexicographical ordering first compares the arguments and then
//     //! the values in the natural order of the arguments.
//     bool operator<(const canonical_table& other) const;

    /**
     * Applies the supplied functor to all values of the factor.  Note
     * that if the factor is represented using a sparse table, this
     * functor will not be called for all assignments.
     * \todo Do we need this function?
     */
    template <typename UnaryFunction>
    void apply(UnaryFunction f) {
      table_data.apply(f);
    }

    // Factor operations
    //==========================================================================
    /**
     * Collapses the table into a smaller table with fewer arguments
     * using the collapse_op operation to combine the values of the remaining
     * assignment values.
     *
     * For instance if A has arguments (i,j,k)
     * B = A.collapse({i}, plus, 0) 
     * will compute
     * B(i) = sum_{j,k} A(i,j,k)
     */
    template <typename AggOp>
    canonical_table collapse(AggOp agg_op, 
                              result_type initialvalue,
                              const finite_domain& retained) const {
      // If the retained arguments contain the arguments of this factor,
      // we can simply return a copy
      if (includes(retained, arguments())) return *this;
  
      // Initialize the table with the initial value
      finite_domain newargs = set_intersect(arguments(), retained);
      canonical_table factor(newargs, initialvalue);
  
      factor.table_data.aggregate(table(),
                             make_dim_map(factor.arg_seq, var_index),
                             agg_op);
      return factor;
    }

    /**
     * Collapses the table into a smaller table with fewer arguments
     * using the collapse_op operation to combine the values of the remaining
     * assignment values.
     *
     * For instance if A has arguments (i,j,k)
     * B = A.collapse({i}, plus, 0) 
     * will compute
     * B(i) = sum_{j,k} A(i,j,k)
     *
     * Unlike the above version, this stores the result in the factor f
     * and avoids reallocation if possible.
     */
    template <typename AggOp>
    void collapse(AggOp agg_op, result_type initialvalue,
                  const finite_domain& retained, canonical_table& f) const {
      finite_var_vector newargs;
      foreach(finite_variable* v, arg_seq)
        if (retained.count(v) != 0)
          newargs.push_back(v);
      if (newargs.size() == arg_seq.size()) {
        // The retained arguments contain the arguments of this factor, so
        // we can simply return a copy.
        if (f.arg_seq == arg_seq)
          f.table_data = table_data;
        else
          f = *this;
      } else {
        if (f.arg_seq != newargs) {
          // Initialize the table with the initial value
          f.initialize(newargs, initialvalue);
          f.args.clear();
          f.args.insert(newargs.begin(), newargs.end());
        } else {
          f.table_data.update(make_constant(initialvalue));
        }
        f.table_data.aggregate(table(),
                               make_dim_map(f.arg_seq, var_index),
                               agg_op);
      }
    }

    /**
     * A complete collapse() to a single value
     * using the agg_op operation to combine all values in the table
     *
     * For instance if A has arguments (i,j,k)
     * B = A.collapse(plus, 0) 
     * will compute
     * B = sum_{i,j,k} A(i,j,k)
     *
     * This is equivalent to passing an empty 'retained' argument
     */
    template <typename AggOp>
    result_type collapse(AggOp agg_op, 
                         result_type initialvalue) const {
      return table_data.aggregate(agg_op, initialvalue);
    }
  
    //! implements Factor::restrict
    canonical_table restrict(const finite_assignment& a) const;

    //! implements Factor::subst_args
    //! \todo Strengthen the requirement on var_map
    canonical_table& subst_args(const finite_var_map& var_map);

    //! implements DistributionFactor::marginal
    canonical_table marginal(const finite_domain& retain) const {
      return collapse(std::plus<result_type>(), 0, retain);
    }

    //! Computes marginal, storing result in factor f.
    //! If f is pre-allocated, this avoids reallocation.
    void marginal(canonical_table& f, const finite_domain& retain) const;

    //! implements DistributionFactor::is_normalizable
    bool is_normalizable() const {
      return is_positive_finite(norm_constant());
    }

    //! Returns the normalization constant
    double norm_constant() const {
      return table_data.aggregate(std::plus<double>(), 0.0);
    }

    //! Normalizes the factor in-place
    canonical_table& normalize();

    //! Computes the maximum for each assignment to the given variables
    canonical_table maximum(const finite_domain& retain) const {
      return collapse(sill::maximum<result_type>(), 
                      result_type(0.0),
                      retain);
    }

    //! Computes the minimum for each assignment to the given variables
    canonical_table minimum(const finite_domain& retain) const {
      return collapse(sill::minimum<result_type>(), 
                      result_type(std::numeric_limits<double>::infinity()),
                      retain);
    }

    //! Returns the maximum value in the factor
    result_type maximum() const {
      return collapse(sill::maximum<result_type>(), result_type(0.0));
    }

    //! Returns the maximum value in the factor
    result_type minimum() const {
      return collapse(sill::minimum<result_type>(), 
                      result_type(std::numeric_limits<double>::infinity()));
    }

    /**
     * implements DistributionFactor::entropy
     *
     * \todo We can avoid the temporary in this function by calling
     * defining some kind of map_collapse function.
     */
    double entropy() const {
      return collapse(std::plus<result_type>(), result_type(0.0));
    }

    /**
     * Computes the Kullback-Liebler divergence from this table factor
     * to the supplied table factor:
     *
     * \f[
     *   \sum_{{\bf x}} p({\bf x}) \log \frac{p({\bf x})}{q({\bf x})}
     * \f]
     *
     * where this factor is \f$p\f$ and the supplied factor is \f$q\f$.
     * Ordinarily this function is used when this factor and the
     * supplied factor are normalized; but this function does not
     * explicitly normalize either factor.
     *
     * @param f the factor to which the KL divergence is computed
     * @return  the KL divergence (in natural logarithmic units)
     */
    double relative_entropy(const canonical_table& f) const {
      assert(this->arguments() == f.arguments());
      double res = combine_collapse(*this, f, kld_operator<result_type>(), 
                                    std::plus<result_type>(), 0.0);
      if (res < 0) res = 0;
      return res;
    }

    double js_divergence(const canonical_table& f) const {
      assert(this->arguments() == f.arguments());
      canonical_table m = combine(*this, f, std::plus<result_type>());
      foreach(result_type& r, m.table_data) {
        r /= 2.0;
      }
      double kl1 = relative_entropy(m);
      double kl2 = f.relative_entropy(m);
      double res= (kl1+kl2)/2.0;
      return res;
      
    }
    double cross_entropy(const canonical_table& f) const {
      assert(this->arguments() == f.arguments());
      return combine_collapse(*this, f, cross_entropy_operator<double>(), 
                            std::plus<double>(), 0.0);
    }
    //! Computes the mutual information between two sets of variables
    //! in this factor's arguments. The sets of f1, f2 must be discombinet.
    double mutual_information(const finite_domain& fd1,
                              const finite_domain& fd2) const;

    //! mooij and kappen upper bound on message derivative between variables
    //! x and y
    //! \todo Stano: can we document this function more? Does it belong here?
    double bp_msg_derivative_ub(variable_type* x, variable_type* y) const;

    /**
     * Unrolls the factor to be over a single variable new_v (created within
     * the given universe).
     * If the factor is over A,B,C (the sequence given by arg_vector()), then
     * this returns a new factor only over variable new_v, with domain of size
     * A.size() * B.size() * C.size().  The new factor's values match those
     * in the original factor, with the values of A,B,C defining the index into
     * the new factor, with A being the highest-order bit: e.g. if the domains
     * are of size 2, new_factor(2^2 * a + 2^1 * b + 2^0 * c) = old_factor(a,b,c).
     *
     * @param  u  universe in which to create the new variable
     * @return pair: newly created variable, new table factor
     * @see create_indicator_factor, roll_up
     */
    std::pair<finite_variable*, canonical_table > unroll(universe& u) const;

    /**
     * Rolls up a factor (which was unrolled to be over a single variable)
     * to restore the factor to its original state over the set of
     * orig_arg_list. This undoes the effects of unroll().
     *
     * @param  orig_arg_list vector of variables this factor was originally
     *                       over
     * @return the restored factor
     * @see unroll
     * \todo Change this so that orig_vars is a general range--but figure out
     *       how to assert that the range is ordered (and isn't just a set).
     */
    canonical_table
    roll_up(const finite_var_vector& orig_arg_list) const;

    // Combine and collapse operations
    //==========================================================================
    
    //! Combines the two factors
    template <typename CombineOp>
    static canonical_table
    combine(const canonical_table& x, const canonical_table& y, CombineOp op) {
      finite_domain arguments = set_union(x.arguments(), y.arguments());

      canonical_table factor(arguments, result_type());
      factor.table_data.join(x.table(), y.table(),
                        make_dim_map(x.arg_seq, factor.var_index),
                        make_dim_map(y.arg_seq, factor.var_index),
                        op);
      //! \todo optimize when x or y have 0 dimensions
      return factor;
    }

    

    //! Combines two factors and collapse
    template <typename CombineOp, typename AggOp>
    static double combine_collapse(const canonical_table& x, const canonical_table& y,
                                 CombineOp combine_op, AggOp agg_op, 
                                 result_type initialvalue) {
      concept_assert((BinaryFunction<CombineOp,result_type,result_type,result_type>));
      concept_assert((BinaryFunction<AggOp,result_type,result_type,result_type>));
      var_index_map var_index;
      if (x.arguments() == y.arguments())
        var_index = x.var_index;
      else
        var_index = make_index_map(set_union(x.arguments(), y.arguments()));
  
      // Perform the computation using a combine-aggregation.
      return table_type::join_aggregate(x.table(), y.table(),
                                  make_dim_map(x.arg_seq, var_index),
                                  make_dim_map(y.arg_seq, var_index),
                                  combine_op, agg_op, initialvalue);
    }


    /**
     * Performs a combine and finds the first pair of values that satisfy
     * the given predicate
     * The items are traversed in the natural order given by the union
     * of x's and y's arguments.
     */
    template <typename Pred>
    static boost::optional< std::pair<result_type, result_type> >
    combine_find(const canonical_table& x, const canonical_table& y, Pred predicate) {
      concept_assert((BinaryPredicate<Pred, result_type, result_type>));
      var_index_map var_index = make_index_map(set_union(x.arguments(), y.arguments()));
      return dense_table<result_type>::join_find(x.table(), y.table(),
                            make_dim_map(x.arg_seq, var_index),
                            make_dim_map(y.arg_seq, var_index),
                            predicate);

    }

    //! Converts table index (subscripts) to an assignment
    finite_assignment assignment(const index_type& index) const;

  // Operator Overloads 
  //============================================================================

    /** Elementwise addition of two table factors. 
     *  i.e. 
     *  A += B
     *  will perform the operation A(i,j,k...) += B(i,j,k,...)
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& operator+=(const canonical_table& y);
    
    /** Elementwise subtraction of two table factors. 
     *  i.e. 
     *  A -= B
     *  will perform the operation A(i,j,k...) -= B(i,j,k,...)
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& operator-=(const canonical_table& y);


    /** Elementwise multiplication of two table factors. 
     *  i.e. 
     *  A *= B
     *  will perform the operation A(i,j,k...) *= B(i,j,k,...)
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& operator*=(const canonical_table& y);
     
    /** Elementwise division of two table factors. 
     *  i.e. 
     *  A /= B
     *  will perform the operation A(i,j,k...) /= B(i,j,k,...)
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& operator/=(const canonical_table& y);
  
      /** Elementwise logical AND of two table factors. 
     *  i.e. 
     *  A.logical_and(B)
     *  will perform the operation A(i,j,k...) = A(i,j,k...) && B(i,j,k,...)
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& logical_and(const canonical_table& y);
  
      /** Elementwise logical OR of two table factors. 
     *  i.e. 
     *  A.logical_or(B)
     *  will perform the operation A(i,j,k...) = A(i,j,k...) || B(i,j,k,...)
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& logical_or(const canonical_table& y);
    /** Elementwise maximum of two table factors. 
     *  i.e. 
     *  A.elementwise_max(B)
     *  will perform the operation A(i,j,k...) = max(A(i,j,k,...), B(i,j,k,...))
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& max(const canonical_table& y);

    /** Elementwise minimum of two table factors. 
     *  i.e. 
     *  A.elementwise_min(B)
     *  will perform the operation A(i,j,k...) = min(A(i,j,k,...), B(i,j,k,...))
     *  The resulting canonical_table will have arguments union(arg(A), arg(B))
     */
    canonical_table& min(const canonical_table& y);

  }; // class canonical_table



// Free functions
//============================================================================
  //! Writes a human-readable representation of the table factor
  //! \relates canonical_table
  std::ostream& operator<<(std::ostream& out, const canonical_table& f);

  //! Returns the L1 distance between two factors
  
  double norm_1(const canonical_table& x, const canonical_table& y);

  //! Returns the L-infinity distance between two factors
  
  double norm_inf(const canonical_table& x, const canonical_table& y);

  //! Returns the L-infinity distance between two factors in log space
  
  double norm_inf_log(const canonical_table& x,
          const canonical_table& y);

  //! Returns the L1 distance between two factors in log space
  
  double norm_1_log(const canonical_table& x,
          const canonical_table& y);


  //! Returns \f$(1-a)f_1 + a f_2\f$
  
  canonical_table weighted_update(const canonical_table& f1,
                                      const canonical_table& f2,
                                      double a);

  //! Returns \f$f^a\f$
  
  canonical_table pow(const canonical_table& f, double a);

  //! Returns an assignment that achieves the maximum value
  finite_assignment arg_max(const canonical_table& f);

  //! Returns an assignment that achieves the minimum value
  finite_assignment arg_min(const canonical_table& f);

  /**
   * Constructs a dense table factor filled by the given value vector.
   *
   * @param arguments the arguments of this factor
   * @param values  vector of values for this factor, ordered:
   *                e.g. if var_vec = <x1, x2> and x1,x2 are binary and
   *                val_vec = <1,2,3,4>, then this sets table<x1=0,x2=0> = 1,
   *                table<x1=1,x2=0> = 2, table<x1=0,x2=1> = 3,
   *                table<x1=1,x2=1> = 4.
   * \relates canonical_table
   */
  template <typename Range>
  canonical_table make_dense_canonical_table(const finite_var_vector& arguments,
                                 const Range& values) {
    canonical_table factor(arguments, 0);
    assert(values.size() == factor.size());
    sill::copy(values, boost::begin(factor.values()));
    return factor;
  }

  // Operator Overloads
  //============================================================================

  /** Elementwise addition of two table factors. 
   *   X = A + B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) + B(i,j,k,...)
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table operator+(const canonical_table& x, const canonical_table& y) {
    return canonical_table::combine(x, y, std::plus<canonical_table::result_type>());
  }
  
  /** Elementwise subtraction of two table factors. 
   *   X = A - B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) - B(i,j,k,...)
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table operator-(const canonical_table& x, const canonical_table& y) {
    return canonical_table::combine(x, y, std::minus<canonical_table::result_type>());
  }
  
  /** Elementwise multiplication of two table factors. 
   *   X = A * B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) * B(i,j,k,...)
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table operator*(const canonical_table& x, const canonical_table& y) {
    return canonical_table::combine(x, y, 
                              std::multiplies<canonical_table::result_type>());
  }
  
  /** Elementwise division of two table factors. 
   *   X = A / B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) / B(i,j,k,...)
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table operator/(const canonical_table& x, const canonical_table& y) {
    return canonical_table::combine(x, y, 
                              safe_divides<canonical_table::result_type>());
  }
  
  /** Elementwise logical AND of two table factors. 
   *   X = A && B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) && B(i,j,k,...)
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table operator&&(const canonical_table& x, const canonical_table& y) {
    return canonical_table::combine(x, y, 
                              logical_and<canonical_table::result_type>());
  }
  
  /** Elementwise logical OR of two table factors. 
   *   X = A || B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) || B(i,j,k,...)
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table operator||(const canonical_table& x, const canonical_table& y) {
    return canonical_table::combine(x, y, 
                              logical_or<canonical_table::result_type>());
  }

  /** Elementwise max of two table factors. 
   *   X = elmentwise_max(A, B)
   *   will perform the operation X(i,j,k...) = max(A(i,j,k...), B(i,j,k,...))
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table max(const canonical_table& x, 
                              const canonical_table& y) {
    return canonical_table::combine(x, y, maximum<canonical_table::result_type>());
  }
  
  /** Elementwise min of two table factors. 
   *   X = elmentwise_min(A, B)
   *   will perform the operation X(i,j,k...) = min(A(i,j,k...), B(i,j,k,...))
   *   The resulting canonical_table will have arguments union(arg(A), arg(B))
   */
  inline canonical_table min(const canonical_table& x, 
                              const canonical_table& y) {
    return canonical_table::combine(x, y, minimum<canonical_table::result_type>());
  }

} // namespace sill


#include <sill/macros_undef.hpp>


#endif // #ifndef SILL_LOG_TABLE_FACTOR_HPP
