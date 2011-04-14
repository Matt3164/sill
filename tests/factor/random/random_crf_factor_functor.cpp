
/**
 * \file random_crf_factor_functor.cpp
 *       Test of random_crf_factor_functor.
 */

#include <sill/base/universe.hpp>
#include <sill/factor/random/random_canonical_gaussian_functor.hpp>
#include <sill/factor/random/random_crf_factor_functor.hpp>

#include <sill/macros_def.hpp>

using namespace sill;

int main(int argc, char** argv) {

  unsigned random_seed = time(NULL);

  boost::mt11213b rng(random_seed);
  boost::uniform_int<int> unif_int(0, std::numeric_limits<int>::max());
  universe u;
  vector_variable* Y = u.new_vector_variable(1);
  vector_variable* X = u.new_vector_variable(1);

  random_crf_factor_functor rmgf(unif_int(rng));

  std::cout << "Test: random_crf_factor_functor\n"
            << "---------------------------------------------" << std::endl;
  crf_factor P_Y(rmgf.generate_marginal(Y));
  std::cout << "Generated crf_factor P(Y):\n"
            << P_Y << std::endl;
  crf_factor P_YX(rmgf.generate_marginal(make_domain(Y,X)));
  std::cout << "Generated crf_factor P(Y,X):\n"
            << P_YX << std::endl;
  crf_factor P_Y_given_X(rmgf.generate_conditional(Y, X));
  std::cout << "Generated crf_factor P(Y|X):\n"
            << P_Y_given_X << std::endl;

} // main

#include <sill/macros_undef.hpp>