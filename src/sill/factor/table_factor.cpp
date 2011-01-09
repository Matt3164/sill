#include <boost/bind.hpp>

#include <sill/base/stl_util.hpp>
#include <sill/base/universe.hpp>
#include <sill/factor/table_factor.hpp>
#include <sill/serialization/serialize.hpp>
#include <sill/macros_def.hpp>

namespace sill {

  // Serialization
  //============================================================================
  void table_factor::save(oarchive& ar) const {
    ar << args << arg_seq << table_data << var_index;
  }

  void table_factor::load(iarchive& ar) {
    ar >> args >> arg_seq >> table_data >> var_index;
  }

  // Accessors
  //============================================================================
  
  void table_factor::swap(table_factor& f) {
    args.swap(f.args);
    var_index.swap(f.var_index);
    arg_seq.swap(f.arg_seq);
    
    dense_table<result_type> temp = table_data;
    table_data = f.table_data;
    f.table_data = temp;
  }


  finite_assignment_range table_factor::assignments() const {
    return std::make_pair(finite_assignment_iterator(arg_seq),
                          finite_assignment_iterator());
  }



  bool table_factor::operator==(const table_factor& other) const {
    if (arguments() == other.arguments()) {
      if (arg_seq == other.arg_seq) // can directly compare the tables
        return table() == other.table();
      else // revert to combine
        return !combine_find(*this, other, std::not_equal_to<result_type>());
    }
    else return false;
  }

  bool table_factor::operator<(const table_factor& other) const {
    if (this->arguments() < other.arguments()) return true;

    if (this->arguments() == other.arguments()) {
      // we need to always perform the combine, even if the argument sequences
      // are the same, in order to traverse the table in the right order
      boost::optional< std::pair<result_type,result_type> > values =
        combine_find(*this, other, std::not_equal_to<result_type>());
      return values && (values->first < values->second);
    }
    else return false;
  }


  table_factor
  table_factor::restrict(const finite_assignment& a) const {
    // NOTE: I did not merge this with the below restrict() since splitting
    //       them allows this version to be slightly more efficient.
    finite_domain retained;
    //more efficient set difference, the domain of the factor is
    //supposed to be small, but evidence size can be very large
    foreach(finite_variable* v, arguments()){
      if (a.find(v) == a.end())
        retained.insert(v);
    }

    //non of the variables of this factors are assigned, return a copy
    if(retained.size() == arguments().size())
      return *this;

    table_factor factor(retained, result_type());
    // TODO: only need to initialize the table's dimensions, not its elements

    factor.table_data.restrict(table(),
                           make_restrict_map(arg_seq, a),
                           make_dim_map(factor.arg_seq, var_index));
    return factor;
  }

  void
  table_factor::restrict(table_factor& f, const finite_assignment& a) const {
    finite_var_vector retained;
    // More efficient set difference: the domain of the factor is
    // supposed to be small, but evidence size can be very large.
    foreach(finite_variable* v, arg_seq) {
      if (a.find(v) == a.end())
        retained.push_back(v);
    }

    if (retained.size() == arg_seq.size()) {
      // None of the variables of this factors are assigned, so return a copy.
      if (f.arg_seq == retained) { // avoid reallocating result factor
        f.table_data = table_data;
      } else { // have to reallocate result factor
        f = *this;
      }
    } else {
      // Some variables were assigned.
      if (f.arg_seq != retained) {
        // TODO: only initialize the table's dimensions, not its elements
        f.initialize(retained, 0.);
        f.args.clear();
        f.args.insert(retained.begin(), retained.end());
      }
      f.table_data.restrict(table(),
                            make_restrict_map(arg_seq, a),
                            make_dim_map(f.arg_seq, var_index));
    }
  }

  void table_factor::
  restrict(table_factor& f, const finite_assignment& a,
           const finite_domain& a_vars, bool strict) const {
    finite_var_vector retained;
    // More efficient set difference: the domain of the factor is
    // supposed to be small, but evidence size can be very large.
    foreach(finite_variable* v, arg_seq) {
      if (a_vars.count(v) == 0)
        retained.push_back(v);
      else {
        if (a.find(v) == a.end()) {
          if (strict) {
            throw std::invalid_argument("table_factor::restrict(f,a,a_vars,strict) was given strict=true, but intersect(f.arguments(), a_vars) contained a variable which did not appear in keys(a).");
          }
          retained.push_back(v);
        }
      }
    }

    if (retained.size() == arg_seq.size()) {
      // None of the variables of this factors are assigned, so return a copy.
      if (f.arg_seq == retained) { // avoid reallocating result factor
        f.table_data = table_data;
      } else { // have to reallocate result factor
        f = *this;
      }
    } else {
      // Some variables were assigned.
      if (f.arg_seq != retained) {
        // TODO: only initialize the table's dimensions, not its elements
        f.initialize(retained, 0.);
        f.args.clear();
        f.args.insert(retained.begin(), retained.end());
      }
      f.table_data.restrict(table(),
                            make_restrict_map(arg_seq, a, a_vars),
                            make_dim_map(f.arg_seq, var_index));
    }
  }

  void
  table_factor::restrict(table_factor& f, const finite_record& r) const {
    finite_var_vector retained;
    // More efficient set difference: the domain of the factor is
    // supposed to be small, but evidence size can be very large.
    foreach(finite_variable* v, arg_seq) {
      if(!r.has_variable(v))
        retained.push_back(v);
    }

    if (retained.size() == arg_seq.size()) {
      // None of the variables of this factors are assigned, so return a copy.
      if (f.arg_seq == retained) { // avoid reallocating result factor
        f.table_data = table_data;
      } else { // have to reallocate result factor
        f = *this;
      }
    } else {
      // Some variables were assigned.
      if (f.arg_seq != retained) {
        // TODO: only initialize the table's dimensions, not its elements
        f.initialize(retained, 0.);
        f.args.clear();
        f.args.insert(retained.begin(), retained.end());
      }
      f.table_data.restrict(table(),
                            make_restrict_map(arg_seq, r),
                            make_dim_map(f.arg_seq, var_index));
    }
  }

  void table_factor::
  restrict(table_factor& f, const finite_record& r,
           const finite_domain& r_vars, bool strict) const {
    finite_var_vector retained;
    // More efficient set difference: the domain of the factor is
    // supposed to be small, but evidence size can be very large.
    foreach(finite_variable* v, arg_seq) {
      if (r_vars.count(v) == 0)
        retained.push_back(v);
      else {
        if (!r.has_variable(v)) {
          if (strict) {
            throw std::invalid_argument("table_factor::restrict(f,r,r_vars,strict) was given strict=true, but intersect(f.arguments(), r_vars) contained a variable which did not appear in keys(r).");
          }
          retained.push_back(v);
        }
      }
    }

    if (retained.size() == arg_seq.size()) {
      // None of the variables of this factors are assigned, so return a copy.
      if (f.arg_seq == retained) { // avoid reallocating result factor
        f.table_data = table_data;
      } else { // have to reallocate result factor
        f = *this;
      }
    } else {
      // Some variables were assigned.
      if (f.arg_seq != retained) {
        // TODO: only initialize the table's dimensions, not its elements
        f.initialize(retained, 0.);
        f.args.clear();
        f.args.insert(retained.begin(), retained.end());
      }
      f.table_data.restrict(table(),
                            make_restrict_map(arg_seq, r, r_vars),
                            make_dim_map(f.arg_seq, var_index));
    }
  }

  table_factor&
  table_factor::combine_in(const table_factor& y, op_type op) {
    switch(op) {
      case sum_op:
        return (*this) += y;
      case minus_op:
        return (*this) -= y;
      case product_op:
        return (*this) *= y;
      case divides_op:
        return (*this) /= y;
      case max_op:
        return this->max(y);
      case min_op:
        return this->min(y);
      case and_op:
        return this->logical_and(y);
      case or_op:
        return this->logical_or(y);
      default:
        assert(false);
    }
  }

  table_factor&
  table_factor::combine_in(const constant_factor& y, op_type op) {
    table_data.update(boost::bind(to_functor(op),  _1, y.value));
    return *this;
  }

  table_factor&
  table_factor::subst_args(const finite_var_map& var_map) {
    args = subst_vars(args, var_map);
    // Compute the new var_index of the arguments, so it matches up
    // with the current var_index.
    for (size_t i = 0; i < arg_seq.size(); ++i) {
      arg_seq[i] = safe_get(var_map, arg_seq[i]);
    }
    var_index = rekey(var_index, var_map);
    return *this;
  }

  void
  table_factor::marginal(table_factor& f, const finite_domain& retain) const {
    collapse(f, std::plus<result_type>(), 0, retain);
  }

  table_factor table_factor::conditional(const finite_domain& B) const {
    assert(includes(arguments(), B));
    table_factor cond(*this);
    table_factor PB(marginal(B));
    foreach(const finite_assignment& fa, assignments())
      cond(fa) /= PB(fa);
    return cond;
  }

  table_factor& table_factor::normalize() {
    double z = norm_constant();
    if (!is_positive_finite((double)z)) {
      std::cerr << "Unnormalizable: " << *this << std::endl;
      throw std::invalid_argument("factor is not normalizeable");
    }
    foreach(double& r, table_data) {
      r /= z;
    }
    return (*this);
  }
 
  // TODO (Stano): this should be cleaned up to not use assignment_iterator
  double table_factor::mutual_information(const finite_domain& fd1,
                                          const finite_domain& fd2) const {
    assert(set_disjoint(fd1, fd2));
    assert(includes(args, fd1) && includes(args, fd2));
    finite_assignment_iterator fa_it(set_union(fd1, fd2));
    finite_assignment_iterator fa_end;
    double mi = 0;
    if (args.size() > fd1.size() + fd2.size()) {
      table_factor fctr(marginal(set_union(fd1,fd2)));
      table_factor fctr1(fctr.marginal(fd1));
      table_factor fctr2(fctr.marginal(fd2));
      while (fa_it != fa_end) {
        const finite_assignment& fa = *fa_it;
        mi += fctr.v(fa) *
          (std::log(fctr.v(fa)) - fctr1.logv(fa) - fctr2.logv(fa));
        ++fa_it;
      }
    } else {
      table_factor fctr1(marginal(fd1));
      table_factor fctr2(marginal(fd2));
      while (fa_it != fa_end) {
        const finite_assignment& fa = *fa_it;
        mi += v(fa) * (logv(fa) - fctr1.logv(fa) - fctr2.logv(fa));
        ++fa_it;
      }
    }
    return mi;
  }

  double table_factor::bp_msg_derivative_ub(variable_type* x,
                                            variable_type* y) const {
    double result = 1;
    // get it in index coordinates
    size_t v = safe_get(var_index, x);
    size_t w = safe_get(var_index, y);
    foreach(const shape_type& a_b_g, table_data.indices()) {
      shape_type ap_b_g(a_b_g);
      foreach(const shape_type& ap_bp_gp, table_data.indices()) {
        shape_type a_bp_gp(ap_bp_gp);
        //in the notation of the paper,
        //f_a_b_g is alpha beta gamma
        //f_ap_bp_gp is alpha' beta' gamma'

        if(a_b_g[v] != ap_bp_gp[v] && a_b_g[w] != ap_bp_gp[w])
        {
          //f_ap_b_g is alpha' beta gamma
          ap_b_g[v] = ap_bp_gp[v];

          //f_a_bp_gp is alpha beta' gamma'
          a_bp_gp[v] = a_b_g[v];

          result = std::max(result,
			    double(table_data(a_b_g) *
				   table_data(ap_bp_gp) /
				   (table_data(ap_b_g) *
				    table_data(a_bp_gp))));
        }
      }
    }
    return tanh(std::log(result) * 0.25);
  }


  #ifndef SWIG // std::pair<variable, table_factor> not supported at the moment
  std::pair<finite_variable*, table_factor >
  table_factor::unroll(universe& u) const {
    size_t new_v_size = 1;
    foreach(finite_variable* v, arg_seq)
      new_v_size *= v->size();
    finite_variable* new_v = u.new_finite_variable(new_v_size);
    finite_var_vector new_args;
    new_args.push_back(new_v);
    table_factor newf
      (new_args, std::vector<result_type>(values().first, values().second));
    return std::make_pair(new_v, newf);
  }
  #endif

  table_factor
  table_factor::roll_up(const finite_var_vector& orig_arg_list) const {
    assert(args.size() == 1);
    finite_variable* arg = *(args.begin());
    size_t s = 1;
    foreach(finite_variable* v, orig_arg_list)
      s *= v->size();
    assert(s == arg->size());
    table_factor newf
      (orig_arg_list,
       std::vector<result_type>(values().first, values().second));
    return newf;
  }


  finite_assignment
  table_factor::assignment(const shape_type& index) const {
    finite_assignment a;
    assert(index.size() == arg_seq.size());
    for(size_t i = 0; i < index.size(); i++)
      a[arg_seq[i]] = index[i];
    return a;
  }

  // Private helper functions
  //==========================================================================

  void table_factor::initialize(
                          const forward_range<finite_variable*>& arguments,
                          result_type default_value) {
    using namespace boost;
// I replaced this line with a manual copy, which is much faster.
//    arg_seq.assign(boost::begin(arguments), boost::end(arguments));
    arg_seq.clear();
    forward_range<finite_variable*>::const_iterator arg_end(arguments.end());
    for (forward_range<finite_variable*>::const_iterator
           arg_it(arguments.begin());
         arg_it != arg_end; ++arg_it)
      arg_seq.push_back(*arg_it);
    var_index.clear();
    shape_type geometry(arg_seq.size());
    for (size_t i = 0; i < arg_seq.size(); ++i) {
      var_index[arg_seq[i]] = i;
      geometry[i] = arg_seq[i]->size();
    }
    // Allocate the table.
    table_data = dense_table<result_type>(geometry, default_value);
  }

  table_factor::shape_type
  table_factor::make_dim_map(const finite_var_vector& vars,
                                    const var_index_map& to_map) {
    // return make_vector(to_map.values(vars)); <-- slow
    dense_table<result_type>::shape_type map(vars.size());
    for(size_t i = 0; i < vars.size(); i++) {
      map[i] = safe_get(to_map, vars[i]);
    }
    return map;
  }

  table_factor::shape_type
  table_factor::make_restrict_map(const finite_var_vector& vars,
                                  const finite_assignment& a) {
    size_t retained = std::numeric_limits<size_t>::max();
    dense_table<result_type>::shape_type map(vars.size(), retained);
    for(size_t i = 0; i < vars.size(); i++) {
      finite_assignment::const_iterator it(a.find(vars[i]));
      if (it != a.end()) map[i] = it->second;
    }
    return map;
  }

  table_factor::shape_type
  table_factor::make_restrict_map(const finite_var_vector& vars,
                                  const finite_record& r) {
    size_t retained = std::numeric_limits<size_t>::max();
    dense_table<result_type>::shape_type map(vars.size(), retained);
    for(size_t i = 0; i < vars.size(); i++) {
      finite_record_iterator it(r.find(vars[i]));
      if (it != r.end())
        map[i] = it->second;
    }
    return map;
  }

  table_factor::shape_type
  table_factor::make_restrict_map(const finite_var_vector& vars,
                                  const finite_assignment& a,
                                  const finite_domain& a_vars) {
    size_t retained = std::numeric_limits<size_t>::max();
    dense_table<result_type>::shape_type map(vars.size(), retained);
    for(size_t i = 0; i < vars.size(); i++) {
      if (a_vars.count(vars[i]) != 0) {
        finite_assignment::const_iterator it(a.find(vars[i]));
        if (it != a.end())
          map[i] = it->second;
      }
    }
    return map;
  }

  table_factor::shape_type
  table_factor::make_restrict_map(const finite_var_vector& vars,
                                  const finite_record& r,
                                  const finite_domain& r_vars) {
    size_t retained = std::numeric_limits<size_t>::max();
    dense_table<result_type>::shape_type map(vars.size(), retained);
    for(size_t i = 0; i < vars.size(); i++) {
      if (r_vars.count(vars[i]) != 0) {
        finite_record_iterator it(r.find(vars[i]));
        if (it != r.end())
          map[i] = it->second;
      }
    }
    return map;
  }

  std::map<finite_variable*, size_t>
  table_factor::make_index_map(const finite_domain& vars) {
    std::map<finite_variable*, size_t> var_index;
    size_t i = 0;
    foreach(finite_variable* v, vars) var_index[v] = i++;
    return var_index;
  }

  // Free functions
  //============================================================================
  std::ostream& operator<<(std::ostream& out, const table_factor& f) {
    out << f.arg_list() << std::endl;
    out << f.table();
    return out;
  }

  // Combine and collapse operations
  //==========================================================================

  table_factor table_factor::collapse(op_type op, 
                                       const finite_domain& retained) const {
    switch(op) {
      case sum_op:
        return collapse(std::plus<result_type>(), 0.0, retained);
      case minus_op:
        /* not well defined */
        return collapse(std::minus<result_type>(), 0.0, retained);
      case product_op:
        return collapse(std::multiplies<result_type>(), 1.0, retained);
      case divides_op:
        /* not well defined */
        return collapse(safe_divides<result_type>(), 1.0, retained);
      case max_op:
        return collapse(sill::maximum<result_type>(), 
                        -std::numeric_limits<double>::infinity(), retained);
      case min_op:
        return collapse(sill::minimum<result_type>(), 
                        std::numeric_limits<double>::infinity(), retained);
      case and_op:
        return collapse(sill::logical_and<result_type>(), 1.0, retained);
      case or_op:
        return collapse(sill::logical_or<result_type>(), 0.0, retained);
      default:
        assert(false); /* Should never reach here */
        return *this;
    }
  }
  table_factor::result_type table_factor::collapse(op_type op) const {
    switch(op) {
      case sum_op:
        return collapse(std::plus<result_type>(), 0.0);
      case minus_op:
        /* not well defined */
        return collapse(std::minus<result_type>(), 0.0); 
      case product_op:
        return collapse(std::multiplies<result_type>(), 1.0);
      case divides_op:
        /* not well defined */
        return collapse(safe_divides<result_type>(), 1.0);
      case max_op:
        return collapse(sill::maximum<result_type>(), 
                         -std::numeric_limits<double>::infinity());
      case min_op:
        return collapse(sill::minimum<result_type>(), 
                         std::numeric_limits<double>::infinity());
      case and_op:
        return collapse(sill::logical_and<result_type>(), 1.0);
      case or_op:
        return collapse(sill::logical_or<result_type>(), 0.0);
      default:
        assert(false); /* Should never reach here */
        return 0.0;
    }
  }

  table_factor combine(const table_factor& x,
                              const table_factor& y,
                              op_type op) {
    switch(op) {
      case sum_op:
        return x + y;
      case minus_op:
        return x - y;
      case product_op:
        return x * y;
      case divides_op:
        return x / y;
      case max_op:
        return max(x, y);
      case min_op:
        return min(x, y);
      case and_op:
        return x && y;
      case or_op:
        return x || y;
      default:
        assert(false);
    }
  }


  double norm_1(const table_factor& x, const table_factor& y) {
    return table_factor::combine_collapse
      (x, y, abs_difference<table_factor::result_type>(), 
       std::plus<table_factor::result_type>(), 0.0);
  }

  double norm_inf(const table_factor& x, const table_factor& y) {
    return table_factor::combine_collapse
      (x, y, abs_difference<table_factor::result_type>(), 
       maximum<table_factor::result_type>(), 
       -std::numeric_limits<double>::infinity());
  }

  
  double norm_inf_log(const table_factor& x, const table_factor& y) {
    return table_factor::combine_collapse
      (x, y, 
       abs_difference_log<table_factor::result_type>(), 
       maximum<table_factor::result_type>(), 
       -std::numeric_limits<double>::infinity());
  }

  
  double norm_1_log(const table_factor& x, const table_factor& y) {
    return table_factor::combine_collapse
         (x, y, 
         abs_difference_log<table_factor::result_type>(), 
         std::plus<table_factor::result_type>(), 0.0);
  }


  
  table_factor weighted_update(const table_factor& f1,
                                      const table_factor& f2,
                                      double a) {
    return table_factor::combine(f1, f2, 
           weighted_plus<table_factor::result_type>(1-a, a));
  }

  
  table_factor pow(const table_factor& f, double a) {
    using std::pow;
    table_factor result(f);
    foreach(table_factor::result_type& x, result.table()) {
      x = pow(x, a);
    }
    return result;
  }

  finite_assignment arg_max(const table_factor& f) {
    table_factor::table_type::const_iterator it = max_element(f.values());
    return f.assignment(f.table().index(it));
  }

  finite_assignment arg_min(const table_factor& f) {
    table_factor::table_type::const_iterator it = min_element(f.values());
    return f.assignment(f.table().index(it));
  }
  // Operator Overloads
  // =====================================================================
  table_factor& table_factor::operator+=(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      std::plus<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, std::plus<table_factor::result_type>());
    }
    return *this;
  }
  
  table_factor& table_factor::operator-=(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      std::minus<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, std::minus<table_factor::result_type>());
    }
    return *this;
  }

  table_factor& table_factor::operator*=(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      std::multiplies<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, std::multiplies<table_factor::result_type>());
    }
    return *this;
  }

  table_factor& table_factor::operator/=(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                           safe_divides<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, safe_divides<table_factor::result_type>());
    }
    return *this;
  }

  table_factor& table_factor::logical_and(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      sill::logical_and<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, sill::logical_and<table_factor::result_type>());
    }
    return *this;
  }


  table_factor& table_factor::logical_or(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      sill::logical_or<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, sill::logical_or<table_factor::result_type>());
    }
    return *this;
  }
  
  table_factor& table_factor::max(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      sill::maximum<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, sill::maximum<table_factor::result_type>());
    }
    return *this;
  }

  table_factor& table_factor::min(const table_factor& y) { 
    if (includes(this->arguments(), y.arguments())) {
      // We can implement the combination efficiently.
      table_data.join_with(y.table(), make_dim_map(y.arg_seq, var_index),
                      sill::minimum<table_factor::result_type>());
    } else {
      // Revert to the standard implementation
      *this = combine(*this, y, sill::minimum<table_factor::result_type>());
    }
    return *this;
  }

  table_factor& table_factor::operator+=(double b) {
    combine_in(constant_factor(b), sum_op);
    return *this;
  }

  table_factor& table_factor::operator*=(double b) {
    combine_in(constant_factor(b), product_op);
    return *this;
  }

} // namespace sill

#include <sill/macros_undef.hpp>