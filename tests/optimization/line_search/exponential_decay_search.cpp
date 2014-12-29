#define BOOST_TEST_MODULE exponential_decay_search
#include <boost/test/unit_test.hpp>

#include <sill/optimization/line_search/exponential_decay_search.hpp>

#include <boost/bind.hpp>

#include "../quadratic_objective.hpp"

using namespace sill;
typedef line_search_result<double> result_type;

template class exponential_decay_search<vec_type>;

BOOST_AUTO_TEST_CASE(test_exponential_decay_search) {
  quadratic_objective objective("5 4", "1 0; 0 1");
  exponential_decay_search_parameters<double> params(0.5, 0.1);
  exponential_decay_search<vec_type> search(params);
  search.reset(boost::bind(&quadratic_objective::value, &objective, _1), NULL);
  
  result_type r1 = search.step("1 2", "1 0");
  BOOST_CHECK_CLOSE(r1.step, 0.5, 1e-6);
  BOOST_CHECK_CLOSE(r1.value, objective.value("1.5 2"), 1e-6);
  
  result_type r2 = search.step("4 3", "1 1");
  BOOST_CHECK_CLOSE(r2.step, 0.05, 1e-6);
  BOOST_CHECK_CLOSE(r2.value, objective.value("4.05 3.05"), 1e-6);
}