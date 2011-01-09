#ifndef SILL_TABLE_CRF_FACTOR_HPP
#define SILL_TABLE_CRF_FACTOR_HPP

#include <sill/factor/concepts.hpp>
#include <sill/factor/learnable_crf_factor.hpp>
#include <sill/optimization/table_factor_opt_vector.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  /**
   * CRF factor based on table_factor.  This supports finite Y,X only.
   *
   * This satisfies the LearnableCRFfactor concept.
   *
   * Regularization info:
   *  - Method (regularization_type::regularization)
   *     - 0: none
   *     - 2: L2
   *         (default)
   *  - Lambdas (regularization_type::lambdas)
   *     - Strength of regularization (in [0,1]), applied to all values in
   *       this factor.
   *
   * \ingroup factor
   * @author Joseph Bradley
   */
  class table_crf_factor
    : public learnable_crf_factor<finite_variable, table_factor,
                                  table_factor_opt_vector, 1> {

    // Public types
    // =========================================================================
  public:

    //! Base class
    typedef
    learnable_crf_factor<finite_variable,table_factor,table_factor_opt_vector,1>
    base;

    //! Parameters used for learn_crf_factor methods.
    struct parameters {

      /**
       * Regularization parameters used for learn_crf_factor().
       * This is interpreted as an amount of smoothing to add to counts.
       * E.g., 1/ntrain = pretend we saw 1 extra training example with each
       * possible assignment.
       *  (default = regularization_type defaults)
       */
      regularization_type reg;

      parameters() { }

      bool valid() const {
        return true;
      }

    }; // struct parameters

    // Protected data
    // =========================================================================
  protected:

    table_factor_opt_vector f;

    //! If true, the data is stored in log-space.
    bool log_space_;

    //! Temporary used to avoid reallocation for conditioning.
    mutable table_factor conditioned_f;

    // Public methods: Constructors, getters, helpers
    // =========================================================================
  public:

    //! Default constructor.
    table_crf_factor()
      : base(), log_space_(false) { }

    /**
     * Constructor for a factor with default parameters.
     * @param Y           Output arguments.
     * @param X           Input arguments.
     * @param log_space_  Indicates if parameters are in log-space.
     *                    (default = true)
     */
    table_crf_factor(const output_domain_type& Y_,
                     const input_domain_type& X_,
                     bool log_space_ = true)
      : base(Y_, copy_ptr<finite_domain>(new finite_domain(X_))),
        f(set_union(Y_,X_),0), log_space_(log_space_), conditioned_f(Y_,0) {
      if (f.f.arguments().size() != Y_.size() + X_.size()) {
        throw std::invalid_argument
          ("table_crf_factor constructor given Y,X which overlap.");
      }
    }

    /**
     * Constructor.
     * @param f           table factor defining this factor
     * @param Y           Y variables; other variables are assumed to be in X
     * @param log_space_  indicates if data is in log-space
     */
    table_crf_factor(const table_factor& f, const finite_domain& Y_,
                     bool log_space_)
      : base(Y_,
             copy_ptr<finite_domain>
             (new finite_domain(set_difference(f.arguments(), Y_)))),
        f(f), log_space_(log_space_), conditioned_f(Y_, 0.) {
      if (!includes(f.arguments(), Y_)) {
        std::cerr << "table_crf_factor constructor: f arguments "
                  << "do not include Y" << std::endl;
        assert(false);
      }
    }

    //! Constructor from a constant factor.
    table_crf_factor(const constant_factor& other)
      : f(table_factor_opt_vector::size_type(), other), log_space_(false) {
    }

    //! Return the underlying factor.
    const table_factor& get_table() const {
      return f.f;
    }

    void print(std::ostream& out) const {
      base::print(out);
      out << f << "\n";
    }

    /**
     * Relabels outputs Y, inputs X so that
     * inputs may become outputs (if variable_type = output_variable_type) and
     * outputs may become inputs (if variable_type = input_variable_type).
     * The entire argument set must remain the same, i.e.,
     * union(Y,X) must equal union(new_Y, new_X).
     */
    void relabel_outputs_inputs(const output_domain_type& new_Y,
                                const input_domain_type& new_X);

    // Public methods: Probabilistic queries
    // =========================================================================

    //! Evaluates this factor for the given datapoint, returning its value
    //! in real-space (not log-space).
    double v(const finite_assignment& a) const;

    //! Evaluates this factor for the given datapoint, returning its value
    //! in real-space (not log-space).
    double v(const finite_record& r) const;

    /**
     * If this factor is f(Y,X), compute f(Y, X = x).
     *
     * @param a  This must assign values to all X in this factor
     *           (but may assign values to any other variables as well).
     * @return  table factor representing the factor with
     *          the given input variable (X) instantiation;
     *          in real space
     */
    const table_factor& condition(const finite_assignment& a) const;

    /**
     * If this factor is f(Y,X), compute f(Y, X = x).
     *
     * @param r Record with values for X in this factor
     *          (which may have values for any other variables as well).
     * @return  table factor representing the factor with
     *          the given input variable (X) instantiation;
     *          in real space
     */
    const table_factor& condition(const finite_record& r) const;

    /**
     * If this factor is f(Y_retain, Y_part, X) (not in log space),
     * return a new factor f(Y_retain, X) which represents
     * exp(E[log(f)]) where the expectation is w.r.t. a uniform distribution
     * over all values of Y_part.
     *
     * @param Y_part   Output variables w.r.t. which the expectation is taken
     *                 in log space.
     *
     * @return  This modified factor.
     */
    table_crf_factor&
    partial_expectation_in_log_space(const output_domain_type& Y_part);

    /**
     * If this factor is f(Y_retain, Y_part, X) (not in log space),
     * return a new factor f(Y_retain, X) which represents
     * exp(E[log(f)]) where the expectation is over values of Y_part in
     * records in the given dataset.
     *
     * @param Y_part   Output variables w.r.t. which the expectation is taken
     *                 in log space.
     *
     * @return  This modified factor.
     */
    table_crf_factor&
    partial_expectation_in_log_space(const output_domain_type& Y_part,
                                     const dataset& ds);

    /**
     * If this factor is f(Y_retain, Y_other, X),
     * marginalize out Y_other to get a new factor f(Y_retain, X).
     *
     * @param Y_other   Output variables to marginalize out.
     *
     * @return  This modified factor.
     */
    table_crf_factor& marginalize_out(const output_domain_type& Y_other);

    /**
     * If this factor is f(Y_part, Y_other, X_part, X_other),
     * set it to f(Y_part = y_part, Y_other, X_part = x_part, X_other).
     *
     * @param a Assignment with values for Y_part, X_part in this factor
     *          (which may have values for any other variables as well).
     * @return  This modified factor.
     */
    table_crf_factor& partial_condition(const assignment_type& a,
                                        const output_domain_type& Y_part,
                                        const input_domain_type& X_part);

    /**
     * If this factor is f(Y_part, Y_other, X_part, X_other),
     * set it to f(Y_part = y_part, Y_other, X_part = x_part, X_other).
     *
     * @param r Record with values for Y_part, X_part in this factor
     *          (which may have values for any other variables as well).
     * @return  This modified factor.
     */
    table_crf_factor& partial_condition(const record_type& r,
                                        const output_domain_type& Y_part,
                                        const input_domain_type& X_part);

    /**
     * Returns the empirical expectation of the log of this factor.
     * In particular, if this factor represents P(A|B), then
     * this returns the expected log likelihood of the distribution P(A | B).
     * (But this does not normalize the factor after conditioning.)
     */
    double log_expected_value(const dataset& ds) const;

    //! implements Factor::combine_in
    table_crf_factor& combine_in(const table_crf_factor& other, op_type op);

    //! combines a constant factor into this factor
    table_crf_factor& combine_in(const constant_factor& other, op_type op);

    //! Combine with constant factor via op(f, *this).
    //! @return This modified factor.
//    table_crf_factor& combine_in_left(const constant_factor& cf, op_type op);

    // Public: Learning-related methods from crf_factor interface
    // =========================================================================

    //! @return  true iff the data is stored in log-space
    bool log_space() const {
      return log_space_;
    }

    //! Tries to change this factor's internal representation to log-space.
    //! This is not guaranteed to work.
    //! @return  true if successful or if the format was already log-space
    bool convert_to_log_space() {
      if (log_space_)
        return true;
      f.f.update(logarithm<double>());
      log_space_ = true;
      return true;
    }

    //! Tries to change this factor's internal representation to real-space.
    //! This is not guaranteed to work.
    //! @return  true if successful or if the format was already real-space
    bool convert_to_real_space() {
      if (!log_space_)
        return true;
      f.f.update(exponent<double>());
      log_space_ = false;
      return true;
    }

    //! This has not been implemented for this class.
    void fix_records(const finite_record& r) {
    }

    //! This has not been implemented for this class.
    void unfix_records() {
    }

    //! The weights which, along with the feature values, define the factor.
    //! This uses log-space or real-space, whatever is currently set,
    //! but it should only be used with log-space.
    const table_factor_opt_vector& weights() const {
      return f;
    }

    //! The weights which, along with the feature values, define the factor.
    //! This uses log-space or real-space, whatever is currently set,
    //! but it should only be used with log-space.
    table_factor_opt_vector& weights() {
      return f;
    }

    // Public: Learning methods from learnable_crf_factor interface
    // =========================================================================

    //! Adds the gradient of the log of this factor w.r.t. the weights,
    //! evaluated at the given datapoint with the current weights.
    //! @param grad   Pre-allocated vector to which to add the gradient.
    //! @param r      Datapoint.
    //! @param w      Weight by which to multiply the added values.
    void add_gradient(table_factor_opt_vector& grad, const finite_record& r,
                      double w) const;

    /**
     * Adds the expectation of the gradient of the log of this factor
     * w.r.t. the weights, evaluated with the current weights and at the
     * given datapoint for the X values.  The expectation is over the Y
     * values and w.r.t. the given factor's distribution.
     * @param grad   Pre-allocated vector to which to add the gradient.
     * @param r      Datapoint.
     * @param fy     Distribution over (at least) the Y variables in this
     *               factor.
     * @param w      Weight by which to multiply the added values.
     * @tparam YFactor  Factor type for a distribution over Y variables.
     */
    void add_expected_gradient(table_factor_opt_vector& grad,
                               const finite_record& r,
                               const table_factor& fy, double w = 1) const;

    /**
     * This is equivalent to (but faster than) calling:
     *   add_gradient(grad, r, w);
     *   add_expected_gradient(grad, r, fy, -1 * w);
     */
    void
    add_combined_gradient(optimization_vector& grad, const finite_record& r,
                          const output_factor_type& fy, double w = 1) const;

    /**
     * Adds the diagonal of the Hessian of the log of this factor w.r.t. the
     * weights, evaluated at the given datapoint with the current weights.
     * @param hessian Pre-allocated vector to which to add the hessian.
     * @param r       Datapoint.
     * @param w       Weight by which to multiply the added values.
     */
    void
    add_hessian_diag(optimization_vector& hessian, const finite_record& r,
                     double w) const;

    /**
     * Adds the expectation of the diagonal of the Hessian of the log of this
     * factor w.r.t. the weights, evaluated with the current weights and at the
     * given datapoint for the X values.  The expectation is over the Y
     * values and w.r.t. the given factor's distribution.
     * @param hessian Pre-allocated vector to which to add the Hessian.
     * @param r       Datapoint.
     * @param fy      Distribution over (at least) the Y variables in this
     *                factor.
     * @param w       Weight by which to multiply the added values.
     */
    void
    add_expected_hessian_diag(optimization_vector& hessian,
                              const finite_record& r,
                              const table_factor& fy, double w) const;

    /**
     * Adds the expectation of the element-wise square of the gradient of the
     * log of this factor w.r.t. the weights, evaluated with the current
     * weights and at the given datapoint for the X values. The expectation is
     * over the Y values and w.r.t. the given factor's distribution.
     * @param sqrgrad Pre-allocated vector to which to add the squared gradient.
     * @param r       Datapoint.
     * @param fy      Distribution over (at least) the Y variables in this
     *                factor.
     * @param w       Weight by which to multiply the added values.
     */
    void
    add_expected_squared_gradient(optimization_vector& sqrgrad,
                                  const finite_record& r,
                                  const table_factor& fy, double w) const;

    /**
     * Returns the regularization penalty for the current weights and
     * the given regularization parameters.
     * This is:  - .5 * lambda * inner_product(weights, weights)
     */
    double regularization_penalty(const regularization_type& reg) const;

    /**
     * Adds the gradient of the regularization term for the current weights
     * and the given regularization parameters.
     * This is:  - lambda * weights
     */
    void add_regularization_gradient(optimization_vector& grad,
                                     const regularization_type& reg,
                                     double w) const;

    /**
     * Adds the diagonal of the Hessian of the regularization term for the
     * current weights and the given regularization parameters.
     */
    void add_regularization_hessian_diag(optimization_vector& hd,
                                         const regularization_type& reg,
                                         double w) const;

  };  // class table_crf_factor

}  // namespace sill

#include <sill/macros_undef.hpp>

#endif // SILL_TABLE_CRF_FACTOR_HPP