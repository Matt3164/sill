
#include <iostream>

#include <sill/math/vector.hpp>
#include <sill/optimization/lbfgs.hpp>

struct obj_functor {
  double objective(sill::vec val) const {
    sill::vec tmpval(2,1.);
    return -5. + sill::inner_prod<double>(val - tmpval, val - tmpval);
  }
};

struct grad_functor {
  void gradient(sill::vec& grad, const sill::vec& val) const {
    sill::vec tmpval(2,1.);
    grad = 2. * (val - tmpval);
  }
};

int main(int argc, char* argv[]) {

  using namespace sill;

  size_t niter = 5;

  vec val(2, 0);
  lbfgs_parameters lbfgs_params;
  lbfgs_params.debug = 2;
  obj_functor obj;
  grad_functor grad;
  lbfgs<vec, obj_functor, grad_functor>
    lbfgs(obj, grad, val, lbfgs_params);
  std::cerr << "Iteration\tObjective\tChange\tx" << std::endl;
  for (size_t t(0); t < niter; ++t) {
    if (!lbfgs.step())
      break;
    std::cerr << lbfgs.iteration() << "\t" << lbfgs.objective() << "\t"
              << lbfgs.objective_change() << "\t" << lbfgs.x() << std::endl;
  }
  std::cerr << "Final values:\n"
            << lbfgs.iteration() << "\t" << lbfgs.objective() << "\t"
            << lbfgs.objective_change() << "\t" << lbfgs.x() << std::endl;
  return 0;

}
