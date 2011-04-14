
#include <sill/factor/random/random.hpp>
#include <sill/factor/random/random_table_factor_functor.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  random_table_factor_functor::
  random_table_factor_functor(unsigned random_seed)
    : rng(random_seed) { }

  table_factor
  random_table_factor_functor::generate_marginal(const domain_type& X) {
    switch (params.factor_choice) {
    case parameters::RANDOM_RANGE:
      assert(params.lower_bound <= params.upper_bound);
      {
        table_factor f(random_range_discrete_factor<table_factor>
                       (X, rng, params.lower_bound, params.upper_bound));
        f.update(exponent<double>());
        return f;
      }
    case parameters::ASSOCIATIVE:
      assert(X.size() == 2);
      {
        finite_variable* v1 = *(X.begin());
        finite_variable* v2 = *(++(X.begin()));
        table_factor f(make_associative_factor(v1, v2, params.base_val));
        f.update(exponent<double>());
        return f;
      }
    case parameters::RANDOM_ASSOCIATIVE:
      assert(X.size() == 2);
      {
        return
          make_random_associative_factor(X, params.base_val, params.lower_bound,
                                         params.upper_bound, rng);
      }
    default:
      assert(false);
      return table_factor();
    }
  } // generate_marginal

  table_factor
  random_table_factor_functor::
  generate_conditional(const domain_type& Y, const domain_type& X) {
    table_factor f(generate_marginal(set_union(Y,X)));
    return f.conditional(X);
  }

  finite_variable*
  random_table_factor_functor::
  generate_variable(universe& u, const std::string& name) const {
    assert(params.arity != 0);
    return u.new_finite_variable(name, params.arity);
  }

  void
  random_table_factor_functor::seed(unsigned random_seed) {
    rng.seed(random_seed);
  }

} // namespace sill

#include <sill/macros_undef.hpp>