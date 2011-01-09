
#ifndef SILL_TABLE_FACTOR_HPP
#define SILL_TABLE_FACTOR_HPP

#include <map>
#include <algorithm>
#include <iosfwd>
#include <sstream>

#include <boost/random/uniform_real.hpp>

#include <sill/base/finite_assignment.hpp>
#include <sill/base/finite_assignment_iterator.hpp>
#include <sill/datastructure/dense_table.hpp>
#include <sill/global.hpp>
#include <sill/factor/constant_factor.hpp>
#include <sill/factor/factor.hpp>
#include <sill/functional.hpp>
#include <sill/learning/dataset/finite_record.hpp>
#include <sill/math/is_finite.hpp>
#include <sill/range/algorithm.hpp>
#include <sill/range/forward_range.hpp>
#include <sill/base/finite_variable.hpp>
#include <sill/serialization/serialize.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  /**
   * A table factor represents a function of a set of finite variables.
   *
   * @tparam Table A type that implements the sill::Table concept.
   *
   * \ingroup factor_types
   * \see Factor
   */
  class table_factor : public factor {
    //    concept_assert((sill::Table));

    // Public type declarations
    //==========================================================================
  public:
    //! implements Factor::result_type
    typedef double result_type;

    typedef dense_table<result_type> table_type;

    //! implements Factor::domain_type
    typedef finite_domain domain_type;

    //! implements Factor::variable_type
    typedef finite_variable variable_type;

    //! implements Factor::assignment_type
    typedef finite_assignment assignment_type;

    //! implements Factor::record_type
    typedef finite_record record_type;

    //! implements Factor::collapse_type
    typedef table_factor collapse_type;

    // Private data members
    //==========================================================================
  private:
    //! The type of shape / index of the underlying table
    typedef table_type::shape_type shape_type;

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
                                    shape_type& s) const{
      for(size_t i = 0; i<arg_seq.size(); i++){
        finite_assignment::const_iterator
              var_i_value_iterator = a.find(arg_seq[i]);
        assert(var_i_value_iterator != a.end());
        s[i] = var_i_value_iterator->second;
      }
    }

    //! Fills in the local table coordinates according to the assignment
    void get_shape_from_assignment( const finite_record& r,
                                    shape_type& s) const{
      for(size_t i = 0; i < arg_seq.size(); i++) {
        s[i] = r.finite(arg_seq[i]);
      }
    }

    //! Fills in the assignment according to the local table coordinates
    void get_assignment_from_shape(const shape_type& s,
                                   finite_assignment& a) const {
      a.clear();
      assert(s.size() == arg_seq.size());
      for(size_t i(0); i < s.size(); i++) {
        a[arg_seq[i]] = s[i];
      }
    }

    //! Creates an object that maps indices of one table to another
    static shape_type make_dim_map(const finite_var_vector& vars,
                                   const var_index_map& to_map);

    //! Creates an object that maps indices of a table to fixed values
    static shape_type make_restrict_map(const finite_var_vector& vars,
                                        const finite_assignment& a);

    //! Creates an object that maps indices of a table to fixed values
    static shape_type make_restrict_map(const finite_var_vector& vars,
                                        const finite_record& r);

    //! Creates an object that maps indices of a table to fixed values,
    //! but limits assignment a to include only variables in a_vars.
    static shape_type make_restrict_map(const finite_var_vector& vars,
                                        const finite_assignment& a,
                                        const finite_domain& a_vars);

    //! Creates an object that maps indices of a table to fixed values,
    //! but limits record r to include only variables in r_vars.
    static shape_type make_restrict_map(const finite_var_vector& vars,
                                        const finite_record& r,
                                        const finite_domain& r_vars);

    //! Creates an object that maps indices of a set to 0..(n-1)
    static var_index_map make_index_map(const finite_domain& vars);

    // Constructors and conversion operators
    //==========================================================================
  public:

    //! Serialize members
    void save(oarchive & ar) const;

    //! Deserialize members
    void load(iarchive & ar);

    //! Default constructor for a factor with no arguments, i.e., a constant.
    explicit table_factor(result_type default_value = 0.0) {
      initialize(this->arguments(), default_value);
    }

    //! Creates a factor with the specified arguments. The table
    //! geometry will respect the specified order of arguments.
    table_factor(const forward_range<finite_variable*>& arguments,
                 result_type default_value)
      : args(boost::begin(arguments), boost::end(arguments)) {
      initialize(arguments, default_value);
    }

    ~table_factor() { }
    //! SWIG constructor
    explicit table_factor(const finite_var_vector& arguments,
                          result_type default_value = 0.0)
      : args(arguments.begin(), arguments.end()) {
      initialize(arguments, default_value);
    }

    //! SWIG constructor
    table_factor(const finite_var_vector& arguments,
                 const std::vector<result_type>& values)
      : args(arguments.begin(), arguments.end()) {
      initialize(arguments, 0);
      assert(table().size() == values.size());
      sill::copy(values, table_data.begin());
    }

    table_factor(const table_factor& factor) {
      (*this) = factor;
    }

    //! Conversion from a constant_factor
    explicit table_factor(const constant_factor& factor) {
      initialize(arguments(), factor.value);
    }

    //! Conversion to a constant factor. The argument set of this factor
    //! must be empty (otherwise, an assertion violation is thrown).
    operator constant_factor() const {
      assert(this->arguments().empty());
      return constant_factor(*table_data.begin());
    }

    //! Conversion to human-readable representation
    operator std::string() const {
      std::ostringstream out; out << *this; return out.str();
    }

    //! Exchanges the content of two factors
    void swap(table_factor& f);

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
    const finite_var_vector& arg_list() const {
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
     * in arg_list(), counting from lowest to highest with the first variable
     * being the most significant digit.
     */
    finite_assignment_range assignments() const;
    
    result_type operator()(const finite_assignment& a) const {
      return v(a);
    }
    result_type operator()(const finite_record& r) const {
      return v(r);
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
    result_type& operator()(const finite_record& r) {
      return v(r);
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
      shape_type index(arg_seq.size());
      get_shape_from_assignment(a,index);
      return table_data(index);
    }

    //! Returns the value associated with a given assignment of variables
    result_type& v(const finite_assignment& a) {
      shape_type index(arg_seq.size());
      get_shape_from_assignment(a,index);
      return table_data(index);
    }

    //! Returns the value associated with a given assignment of variables
    result_type v(const finite_record& r) const {
      shape_type index(arg_seq.size());
      get_shape_from_assignment(r,index);
      return table_data(index);
    }

    //! Returns the value associated with a given assignment of variables
    result_type& v(const finite_record& r) {
      shape_type index(arg_seq.size());
      get_shape_from_assignment(r,index);
      return table_data(index);
    }

    //! Returns the log of the value associated with an assignment.
    double logv(const finite_assignment& a) const {
      return std::log(v(a));
    }

    //! Returns the log of the value associated with an assignment.
    double logv(const finite_record& r) const {
      return std::log(v(r));
    }

    //! direct indexing for 2 arguments
    result_type v(size_t i, size_t j) const {
      assert(arguments().size()==2);
      shape_type index(2);
      index[0] = i;
      index[1] = j;
      return table_data(index);
    }
    
    //! direct indexing for 2 arguments
    result_type& v(size_t i, size_t j) {
      assert(arguments().size()==2);
      shape_type index(2);
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
      shape_type index(1);
      index[0] = i;
      return table_data(index);
    }

    //! direct indexing for 1 argument
    result_type& v(size_t i) {
      assert(arguments().size()==1);
      shape_type index(1);
      index[0] = i;
      return table_data(index);
    }
    
    //! direct indexing for 1 argument
    double logv(size_t i) const {
      return std::log(v(i));
    }



    // Setters
    // -------------------------------------------------------------------

    //! Sets the value associated with a given assignment of variables
    void set_v(const finite_assignment& a, result_type v) {
      shape_type index(arg_seq.size());
      get_shape_from_assignment(a,index);
      table_data(index) = v;
    }

    //! Sets the value associated with a given assignment of variables
    //! to be exp(v).
    void set_logv(const finite_assignment& a, double v) {
      set_v(a, std::exp(v));
    }

    //! direct indexing for 2 arguments
    void set_v(size_t i, size_t j, result_type v) {
      assert(arguments().size()==2);
      shape_type index(2);
      index[0] = i;
      index[1] = j;
      table_data(index) = v;
    }

    //! direct indexing for 2 arguments
    void set_logv(size_t i, size_t j, double v) {
      set_v(i, j, std::exp(v));
    }
 
     //! direct indexing for 1 argument
    void set_v(size_t i, result_type v) {
      assert(arguments().size()==1);
      shape_type index(1);
      index[0] = i;
      table_data(index) = v;
    }

    //! direct indexing for 1 argument
    void set_logv(size_t i, double v) {
      set_v(i, std::exp(v));
    }


    //! Returns true if the two factors have the same argument sets and values
    bool operator==(const table_factor& other) const;

    //! Assignment operator
    table_factor& operator=(const table_factor& other) {
      args = other.args;
      var_index = other.var_index;
      arg_seq = other.arg_seq;
      table_data = other.table_data;
      return *this;
    }

    //! Assigns the given value to all elements in this factor.
    table_factor& operator=(double val) {
      table_data.update(make_constant(val));
      return *this;
    }

    //! Returns true if the two factors do not have the same arguments or values
    bool operator!=(const table_factor& other) const {
      return !(*this == other);
    }

    //! Returns true if *this precedes other in the lexicographical ordering
    //! The lexicographical ordering first compares the arguments and then
    //! the values in the natural order of the arguments.
    bool operator<(const table_factor& other) const;

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

    /**
     * Updates all values using the supplied functor.
     * Note that if the factor is represented using a sparse table, this
     * functor will not be called for all assignments.
     */
    template <typename UnaryFunction>
    void update(UnaryFunction f) {
      table_data.update(f);
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
    table_factor collapse(AggOp agg_op, 
                          double initialvalue,
                          const finite_domain& retained) const {
      // If the retained arguments contain the arguments of this factor,
      // we can simply return a copy
      if (includes(retained, arguments())) return *this;
  
      // Initialize the table with the initial value
      finite_domain newargs = set_intersect(arguments(), retained);
      table_factor factor(newargs, initialvalue);
  
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
    void collapse(table_factor& f, AggOp agg_op, double initialvalue,
                  const finite_domain& retained) const {
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
    double collapse(AggOp agg_op, 
                    double initialvalue) const {
      return table_data.aggregate(agg_op, initialvalue);
    }

    /** An overload of collapse() which takes in an op_type instead 
      * of a functor
      */
    table_factor collapse(op_type op, const finite_domain& retained) const;
    
    
    /** An overload of collapse() which takes in an op_type instead 
      * of a functor
      */
    result_type collapse(op_type op) const;
    
    //! implements Factor::restrict
    table_factor restrict(const finite_assignment& a) const;

    //! Restrict which stores the result in the given factor f.
    //! This avoids reallocation if f has been pre-allocated.
    void restrict(table_factor& f, const finite_assignment& a) const;

    /**
     * Restrict which stores the result in the given factor f.
     * This avoids reallocation if f has been pre-allocated.
     * @param a_vars  Only restrict away arguments of this factor which
     *                appear in both keys(a) and a_vars.
     * @param strict  Require that all variables which are in
     *                intersect(f.arguments(), a_vars) appear in keys(a).
     *                (default = false)
     */
    void restrict(table_factor& f, const finite_assignment& a,
                  const finite_domain& a_vars, bool strict = false) const;

    //! Restrict which stores the result in the given factor f.
    //! This avoids reallocation if f has been pre-allocated.
    void restrict(table_factor& f, const finite_record& r) const;

    /**
     * Restrict which stores the result in the given factor f.
     * This avoids reallocation if f has been pre-allocated.
     * @param r_vars  Only restrict away arguments of this factor which
     *                appear in both keys(r) and r_vars.
     * @param strict  Require that all variables which are in
     *                intersect(f.arguments(), r_vars) appear in keys(r).
     *                (default = false)
     */
    void restrict(table_factor& f, const finite_record& r,
                  const finite_domain& r_vars, bool strict = false) const;

    //! implements Factor::combine_in
    table_factor& combine_in(const table_factor& y, op_type op);

    //! combines a constant factor into this factor
    table_factor& combine_in(const constant_factor& y, op_type op);

    //! implements Factor::subst_args
    //! \todo Strengthen the requirement on var_map
    table_factor& subst_args(const finite_var_map& var_map);

    //! implements DistributionFactor::marginal
    table_factor marginal(const finite_domain& retain) const {
      return collapse(std::plus<result_type>(), 0, retain);
    }

    //! Computes marginal, storing result in factor f.
    //! If f is pre-allocated, this avoids reallocation.
    void marginal(table_factor& f, const finite_domain& retain) const;

    //! If this factor represents P(A,B), then this returns P(A|B).
    //! @todo Make this more efficient.
    table_factor conditional(const finite_domain& B) const;

    //! implements DistributionFactor::is_normalizable
    bool is_normalizable() const {
      return is_positive_finite(norm_constant());
    }

    //! Returns the normalization constant
    double norm_constant() const {
      return table_data.aggregate(std::plus<double>(), 0);
    }

    //! Normalizes the factor in-place
    table_factor& normalize();

    //! Computes the maximum for each assignment to the given variables
    table_factor maximum(const finite_domain& retain) const {
      return collapse(sill::maximum<result_type>(), 
                       -std::numeric_limits<double>::infinity(),
                       retain);
    }

    //! Computes the minimum for each assignment to the given variables
    table_factor minimum(const finite_domain& retain) const {
      return collapse(sill::minimum<result_type>(), 
                       std::numeric_limits<double>::infinity(),
                       retain);
    }

    //! Returns the maximum value in the factor
    result_type maximum() const {
      return table_data.aggregate(sill::maximum<result_type>(), 
                                  -std::numeric_limits<double>::infinity());
    }

    //! Returns the maximum value in the factor
    result_type minimum() const {
      return table_data.aggregate(sill::minimum<result_type>(), 
                                  std::numeric_limits<double>::infinity());
    }

    //! Returns a sample from the factor, which is assumed to be normalized
    //! to be a distribution P(arguments).
    template <typename RandomNumberGenerator>
    finite_assignment sample(RandomNumberGenerator& rng) const {
      double r(boost::uniform_real<double>(0,1)(rng));
      foreach(const shape_type& s, table_data.indices()) {
        if (r < table_data(s)) {
          finite_assignment a;
          get_assignment_from_shape(s, a);
          return a;
        } else {
          r -= table_data(s);
        }
      }
      // This should not really happen, so just return whatever.
      finite_assignment a;
      foreach(finite_variable* v, arg_seq) {
        a[v] = v->size() - 1;
      }
      return a;
    }

    /**
     * implements DistributionFactor::entropy (using the given base)
     *
     * \todo We can avoid the temporary in this function by calling
     * defining some kind of map_collapse function.
     */
    double entropy(double base) const {
      table_type tmp(table_data);
      tmp.update(entropy_operator<result_type>(base));
      return tmp.aggregate(std::plus<result_type>(), 0.0);
    }

    /**
     * implements DistributionFactor::entropy (base e)
     *
     * \todo We can avoid the temporary in this function by calling
     * defining some kind of map_collapse function.
     */
    double entropy() const {
      return entropy(std::exp(1.));
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
    double relative_entropy(const table_factor& f) const {
      assert(this->arguments() == f.arguments());
      double res = combine_collapse(*this, f, 
                                    kld_operator<double>(), 
                                    std::plus<double>(), 0.0);
      if (res < 0) res = 0;
      return res;
    }

    double js_divergence(const table_factor& f) const {
      assert(this->arguments() == f.arguments());
      table_factor m = combine(*this, f, std::plus<result_type>());
      foreach(result_type& r, m.table_data) {
        r /= 2.0;
      }
      double kl1 = relative_entropy(m);
      double kl2 = f.relative_entropy(m);
      double res= (kl1+kl2)/2.0;
      return res;
      
    }
    double cross_entropy(const table_factor& f) const {
      assert(this->arguments() == f.arguments());
      return combine_collapse(*this, f, cross_entropy_operator<result_type>(), 
                            std::plus<result_type>(), 0.0);
    }
    //! Computes the mutual information between two sets of variables
    //! in this factor's arguments. The sets of f1, f2 must be discombinet.
    double mutual_information(const finite_domain& fd1,
                              const finite_domain& fd2) const;

    //! mooij and kappen upper bound on message derivative between variables
    //! x and y
    //! \todo Stano: can we document this function more? Does it belong here?
    double bp_msg_derivative_ub(variable_type* x, variable_type* y) const;

  #ifndef SWIG // std::pair<variable, table_factor> not supported at the moment
    /**
     * Unrolls the factor to be over a single variable new_v (created within
     * the given universe).
     * If the factor is over A,B,C (the sequence given by arg_list()), then
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
    std::pair<finite_variable*, table_factor > unroll(universe& u) const;
  #endif

    /**
     * Rolls up a factor (which was unrolled to be over a single variable)
     * to restore the factor to its original state over the set of
     * orig_arg_list. This undoes the effects of unroll().
     *
     * @param  orig_arg_list vector of variables this factor was originally
     *                       over
     * @return the restored factor
     * @see unroll
     */
    table_factor
    roll_up(const finite_var_vector& orig_arg_list) const;

    // Combine and collapse operations
    //==========================================================================
    
    //! Combines the two factors
    template <typename CombineOp>
    static table_factor
    combine(const table_factor& x, const table_factor& y, CombineOp op) {
      finite_domain arguments = set_union(x.arguments(), y.arguments());

      table_factor factor(arguments, result_type());
      factor.table_data.join(x.table(), y.table(),
                        make_dim_map(x.arg_seq, factor.var_index),
                        make_dim_map(y.arg_seq, factor.var_index),
                        op);
      //! \todo optimize when x or y have 0 dimensions
      return factor;
    }

    

    //! Combines two factors and collapse
    template <typename CombineOp, typename AggOp>
    static double combine_collapse(const table_factor& x, const table_factor& y,
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
    combine_find(const table_factor& x, const table_factor& y, Pred predicate) {
      concept_assert((BinaryPredicate<Pred, result_type, result_type>));
      var_index_map var_index = make_index_map(set_union(x.arguments(), y.arguments()));
      return dense_table<result_type>::join_find(x.table(), y.table(),
                            make_dim_map(x.arg_seq, var_index),
                            make_dim_map(y.arg_seq, var_index),
                            predicate);

    }

    //! Converts table index (subscripts) to an assignment
    finite_assignment assignment(const shape_type& index) const;

  // Operator Overloads 
  //============================================================================

    /** Elementwise addition of two table factors. 
     *  i.e. 
     *  A += B
     *  will perform the operation A(i,j,k...) += B(i,j,k,...)
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& operator+=(const table_factor& y);
    
    /** Elementwise subtraction of two table factors. 
     *  i.e. 
     *  A -= B
     *  will perform the operation A(i,j,k...) -= B(i,j,k,...)
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& operator-=(const table_factor& y);


    /** Elementwise multiplication of two table factors. 
     *  i.e. 
     *  A *= B
     *  will perform the operation A(i,j,k...) *= B(i,j,k,...)
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& operator*=(const table_factor& y);
     
    /** Elementwise division of two table factors. 
     *  i.e. 
     *  A /= B
     *  will perform the operation A(i,j,k...) /= B(i,j,k,...)
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& operator/=(const table_factor& y);
  
      /** Elementwise logical AND of two table factors. 
     *  i.e. 
     *  A.logical_and(B)
     *  will perform the operation A(i,j,k...) = A(i,j,k...) && B(i,j,k,...)
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& logical_and(const table_factor& y);
  
      /** Elementwise logical OR of two table factors. 
     *  i.e. 
     *  A.logical_or(B)
     *  will perform the operation A(i,j,k...) = A(i,j,k...) || B(i,j,k,...)
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& logical_or(const table_factor& y);
    /** Elementwise maximum of two table factors. 
     *  i.e. 
     *  A.elementwise_max(B)
     *  will perform the operation A(i,j,k...) = max(A(i,j,k,...), B(i,j,k,...))
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& max(const table_factor& y);

    /** Elementwise minimum of two table factors. 
     *  i.e. 
     *  A.elementwise_min(B)
     *  will perform the operation A(i,j,k...) = min(A(i,j,k,...), B(i,j,k,...))
     *  The resulting table_factor will have arguments union(arg(A), arg(B))
     */
    table_factor& min(const table_factor& y);

    /** Add a constant to all values in the table factor.
     *  i.e. 
     *  A += b
     *  will perform the operation A(i,j,k...) += b
     */
    table_factor& operator+=(double b);

    /**
     * Multiply all values in the table factor by a constant.
     */
    table_factor& operator*=(double b);

  }; // class table_factor



// Free functions
//============================================================================
  //! Writes a human-readable representation of the table factor
  //! \relates table_factor
  std::ostream& operator<<(std::ostream& out, const table_factor& f);

  //! Combines two table factors
  //! \relates table_factor
  table_factor combine(const table_factor& x,
                       const table_factor& y,
                       op_type op);

  //! Returns the L1 distance between two factors
  double norm_1(const table_factor& x, const table_factor& y);

  //! Returns the L-infinity distance between two factors
  double norm_inf(const table_factor& x, const table_factor& y);

  //! Returns the L-infinity distance between two factors in log space
  double norm_inf_log(const table_factor& x,
                      const table_factor& y);

  //! Returns the L1 distance between two factors in log space
  double norm_1_log(const table_factor& x,
                    const table_factor& y);


  //! Returns \f$(1-a)f_1 + a f_2\f$
  table_factor weighted_update(const table_factor& f1,
                               const table_factor& f2,
                               double a);

  //! Returns \f$f^a\f$
  table_factor pow(const table_factor& f, double a);

  //! Returns an assignment that achieves the maximum value
  finite_assignment arg_max(const table_factor& f);

  //! Returns an assignment that achieves the minimum value
  finite_assignment arg_min(const table_factor& f);

  /**
   * Constructs a dense table factor filled by the given value vector.
   *
   * @param arguments the arguments of this factor
   * @param values  vector of values for this factor, ordered:
   *                e.g. if var_vec = <x1, x2> and x1,x2 are binary and
   *                val_vec = <1,2,3,4>, then this sets table<x1=0,x2=0> = 1,
   *                table<x1=1,x2=0> = 2, table<x1=0,x2=1> = 3,
   *                table<x1=1,x2=1> = 4.
   * \relates table_factor
   */
  template <typename Range>
  table_factor make_dense_table_factor(const finite_var_vector& arguments,
                                       const Range& values) {
    table_factor factor(arguments, 0);
    assert(values.size() == factor.size());
    sill::copy(values, boost::begin(factor.values()));
    return factor;
  }

// Operator Overloads
//============================================================================

  /** Elementwise addition of two table factors. 
   *   X = A + B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) + B(i,j,k,...)
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor operator+(const table_factor& x, const table_factor& y) {
    return table_factor::combine(x, y, std::plus<table_factor::result_type>());
  }
  
  /** Elementwise subtraction of two table factors. 
   *   X = A - B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) - B(i,j,k,...)
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor operator-(const table_factor& x, const table_factor& y) {
    return table_factor::combine(x, y, std::minus<table_factor::result_type>());
  }
  
  /** Elementwise multiplication of two table factors. 
   *   X = A * B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) * B(i,j,k,...)
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor operator*(const table_factor& x, const table_factor& y) {
    return table_factor::combine(x, y, std::multiplies<double>());
  }
  
  /** Elementwise division of two table factors. 
   *   X = A / B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) / B(i,j,k,...)
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor operator/(const table_factor& x, const table_factor& y) {
    return table_factor::combine(x, y, safe_divides<double>());
  }
  
  /** Elementwise logical AND of two table factors. 
   *   X = A && B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) && B(i,j,k,...)
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor operator&&(const table_factor& x, const table_factor& y) {
    return table_factor::combine(x, y, logical_and<double>());
  }
  
  /** Elementwise logical OR of two table factors. 
   *   X = A || B
   *   will perform the operation X(i,j,k...) = A(i,j,k...) || B(i,j,k,...)
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor operator||(const table_factor& x, const table_factor& y) {
    return table_factor::combine(x, y, logical_or<double>());
  }

  /** Elementwise max of two table factors. 
   *   X = elmentwise_max(A, B)
   *   will perform the operation X(i,j,k...) = max(A(i,j,k...), B(i,j,k,...))
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor max(const table_factor& x, 
                              const table_factor& y) {
    return table_factor::combine(x, y, maximum<double>());
  }
  
  /** Elementwise min of two table factors. 
   *   X = elmentwise_min(A, B)
   *   will perform the operation X(i,j,k...) = min(A(i,j,k...), B(i,j,k,...))
   *   The resulting table_factor will have arguments union(arg(A), arg(B))
   */
  inline table_factor min(const table_factor& x, 
                              const table_factor& y) {
    return table_factor::combine(x, y, minimum<double>());
  }

  /**
   * Multiplication of all elements in a table factor by a constant.
   */
  inline table_factor operator*(const table_factor& x, double b) {
    table_factor y(x);
    return y.combine_in(constant_factor(b), product_op);
  }

  //typedef table_factor tablef;
} // namespace sill

#include <sill/macros_undef.hpp>

#include <sill/factor/operations.hpp>

#endif // #ifndef SILL_TABLE_FACTOR_HPP