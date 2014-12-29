#ifndef SILL_GRADIENT_DESCENT_HPP
#define SILL_GRADIENT_DESCENT_HPP

#include <sill/math/constants.hpp>
#include <sill/optimization/concepts.hpp>
#include <sill/optimization/gradient_method/gradient_method.hpp>
#include <sill/optimization/line_search/line_search.hpp>

#include <boost/shared_ptr.hpp>

namespace sill {

  /**
   * A class the performs gradient descent to minimize an objective.
   * At each iteration, we perform line search along the negative
   * gradient direction.
   *
   * \ingroup optimization_gradient
   *
   * \tparam Vec a type that satisfies the OptimizationVector concept
   */
  template <typename Vec>
  class gradient_descent : public gradient_method<Vec> {
  public:
    typedef typename Vec::value_type real_type;
    typedef line_search_result<real_type> result_type;
    typedef boost::function<real_type(const Vec&)> objective_fn;
    typedef boost::function<const Vec&(const Vec&)> gradient_fn;

    struct param_type {
      /**
       * We declare convergence if the difference between the previous
       * and the new objective value is less than this threshold.
       */
      real_type convergence;

      explicit param_type(real_type convergence = 1e-6)
        : convergence(convergence) { }

      friend std::ostream& operator<<(std::ostream& out, const param_type& p) {
        out << p.convergence;
        return out;
      }
    };
    
    /**
     * Creates a gradient_descent object using the given line search algorithm
     * and convergence parameters. The line_search object becomes owned by the
     * gradient_descent and will be deleted upon destruction.
     */
    explicit gradient_descent(line_search<Vec>* search,
                              const param_type& params = param_type())
      : search_(search),
        params_(params),
        value_(nan()),
        converged_(false) { }

    void reset(const objective_fn& objective,
               const gradient_fn& gradient,
               const Vec& init) {
      search_->reset(objective, gradient);
      gradient_ = gradient;
      x_ = init;
      value_ = nan();
      converged_ = false;
    }

    result_type iterate() {
      dir_ = -gradient_(x_);
      result_type result = search_->step(x_, dir_);
      x_ += result.step * dir_;
      converged_ = (value_ - result.value) < params_.convergence;
      value_ = result.value;
      return result;
    }

    bool converged() const {
      return converged_;
    }

    const Vec& solution() const {
      return x_;
    }

    void print(std::ostream& out) const {
      out << "gradient_descent(" << params_ << ")";
    }

  private:
    //! The line search algorithm
    boost::shared_ptr<line_search<Vec> > search_;

    //! The convergence parameters
    param_type params_;

    //! The gradient function
    gradient_fn gradient_;

    //! Current solution
    Vec x_;

    //! Last descent direction (-gradient)
    Vec dir_;

    //! Last objective value
    real_type value_;

    //! True if the (last) iteration has converged
    bool converged_;

  }; // class gradient_descent

} // namespace sill

#endif