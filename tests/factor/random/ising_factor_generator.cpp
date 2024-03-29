#define BOOST_TEST_MODULE ising_factor_generator
#include <boost/test/unit_test.hpp>

#include <sill/base/universe.hpp>
#include <sill/factor/random/ising_factor_generator.hpp>

#include <boost/random/mersenne_twister.hpp>

#include <sill/macros_def.hpp>

using namespace sill;

size_t nsamples = 100;
const double lower = -0.7;
const double upper = +0.5;
const double exp_lower = std::exp(lower);
const double exp_upper = std::exp(upper);

BOOST_AUTO_TEST_CASE(test_all) {
  universe u;
  finite_variable* x = u.new_finite_variable(2);
  finite_variable* y = u.new_finite_variable(2);
  finite_domain xs = make_domain(x);
  finite_domain ys = make_domain(y);
  finite_domain xy = make_domain(x, y);

  boost::mt19937 rng;
  ising_factor_generator gen(lower, upper);

  // check the node marginals
  for (size_t i = 0; i < nsamples; ++i) {
    table_factor f = gen(xs, rng);
    BOOST_CHECK_CLOSE(f(1), 1.0/f(0), 1e-8);
    BOOST_CHECK(f(1) >= exp_lower && f(1) <= exp_upper);
  }

  // check the pairwise marginals
  for (size_t i = 0; i < nsamples; ++i) {
    table_factor f = gen(xy, rng);
    double val = f(0, 0);
    BOOST_CHECK(val >= exp_lower && val <= exp_upper);
    foreach(finite_assignment a, assignments(xy)) {
      if (a[x] == a[y]) {
        BOOST_CHECK_CLOSE(f(a), val, 1e-8);
      } else {
        BOOST_CHECK_CLOSE(f(a), 1.0/val, 1e-8);
      }
    }
  }
}
