#ifndef SILL_PRIOR_LIKELIHOOD_HPP
#define SILL_PRIOR_LIKELIHOOD_HPP

#include <sill/global.hpp>
#include <sill/factor/concepts.hpp>
#include <sill/factor/operations.hpp>
#include <sill/serialization/serialize.hpp>

#include <sill/macros_def.hpp>

namespace sill
{

  /**
   * A special kind of a factor that contains a prior of a distribution
   * and its likelihood. Multiplication of two prior_likelihoods assumes
   * that the two priors are neighbors in the external junction tree,
   * and performs implicit division of the prior separator.
   *
   * @tparam F the prior factor type
   * @tparam G the likelihood factor type
   *        The result of combining F and G must be convertible to F
   *
   * \ingroup factor_types
   * \see Factor
   */
  template <typename F, typename G = F>
  class prior_likelihood : public factor {
    concept_assert((DistributionFactor<F>));
    concept_assert((DistributionFactor<G>));

    // Public type declarations
    //==========================================================================
  public:
    //! The base factor type.
    typedef factor base;

    //! Tyhe type of values stored in the factor
    typedef typename F::result_type result_type;

    //! implements Factor::domain_type
    typedef typename F::domain_type domain_type;

    //! implements Factor::variable_type
    typedef typename F::variable_type variable_type;

    //! the assignment type of the factor
    typedef std::map<variable_type*, typename variable_type::value_type>
      assignment_type;

    //! implements Factor::record_type
    typedef typename F::record_type record_type;

    //! implements Factor::collapse_type
    typedef prior_likelihood collapse_type;

    // Private data members
    //==========================================================================
  private:
    //! The prior factor
    F prior_;

    //! The likelihood factor
    G likelihood_;

    // Constructors and conversion operators
    //==========================================================================
  public:
    //! Default constructor
    prior_likelihood() : prior_(F(1)), likelihood_(G(1)) { }

    //! Constructor with optional likelihood
    //! \todo Ideally, this would be explicit
    prior_likelihood(const F& prior, const G& likelihood = G(1))
      : prior_(prior), likelihood_(likelihood) {
      assert(includes(prior.arguments(), likelihood.arguments()));
    }

    //! Constructor with a constant value for the likelihood
    explicit prior_likelihood(double val)
      : prior_(1), likelihood_(val) { }

//     //! Constant factor conversion operator
//     //! returns likelihood(), converted to a constant_factor
//     operator constant_factor() {
//       assert(this->arguments().empty());
//       return likelihood_;
//     }

    //! Conversion to human-readable representation
    operator std::string() const {
      std::ostringstream out; out << *this; return out.str();
    }

    // Accessors and comparison operators
    //==========================================================================

    //! Returns the argument set of this factor
    const domain_type& arguments() const {
      return prior_.arguments();
    }

    //! Returns the prior
    const F& prior() const {
      return prior_;
    }

    //! Returns the likelihood
    const G& likelihood() const {
      return likelihood_;
    }

    //! Assigns the prior.
    //! @require prior.arguments() == this->arguments()
    prior_likelihood& set_prior(const F& prior) {
      assert(prior.arguments() == this->arguments());
      prior_ = prior;
      return *this;
    }

    //! Returns the product of prior and likelihood
    F joint() const {
      assert(includes(prior_.arguments(), likelihood_.arguments()));
      return prior_ * likelihood_;
    }

    //! Returns true if two PL factors are equal
    bool operator==(const prior_likelihood& other) const {
      return prior_ == other.prior_ && likelihood_ == other.likelihood_;
    }

    //! Returns true if two PL factors are not equal
    bool operator!=(const prior_likelihood& other) const {
      return !operator==(other);
    }

    // Factor operations
    //==========================================================================

    //! Multiplies by a constant
    prior_likelihood& operator*=(double val) {
      likelihood_ *= val;
      return *this;
    }

    //! Multiplies a likelihood factor into this PL factor
    prior_likelihood& operator*=(const G& likelihood) {
      assert(includes(arguments(), likelihood.arguments()));
      likelihood_ *= likelihood;
      return *this;
    }

    //! Multiplies in another prior-likelihood factor into this PL factor
    prior_likelihood& operator*=(const prior_likelihood& x) {
      if(includes(arguments(), x.arguments())) {
        likelihood_ *= x.likelihood;
      } else {
        *this = (*this) * x;
      }
      return *this;
    }

    //! Computes a marginal of the PL factor
    prior_likelihood marginal(const domain_type& retain) const {
      if (likelihood().arguments().empty()) {
        return prior_likelihood(prior().marginal(retain), likelihood());
      } else {
        F pmarginal = prior().marginal(retain);
        F jmarginal = joint().marginal(retain);
        return prior_likelihood(pmarginal, jmarginal/pmarginal);
      }
    }

    //! implements Factor::restrict
    prior_likelihood restrict(const assignment_type& a) const {
      return prior_likelihood(prior().restrict(a), likelihood().restrict(a));
    }

    //! implements Factor::subst_args
    prior_likelihood&
    subst_args(const std::map<variable_type*, variable_type*>& map) {
      prior_.subst_args(map);
      likelihood_.subst_args(map);
      return *this;
    }

    //! Transfers the likelihood from another P-L factor to this factor
    prior_likelihood& transfer_from(const prior_likelihood& from) {
      if (includes(this->arguments(), from.likelihood().arguments()))
        likelihood_ *= from.likelihood();
      else
        likelihood_ *= from.marginal(arguments()).likelihood();
      return *this;
    }

    //! Serialize / deserialize members
    void save(oarchive& ar) const {
      ar << prior_;
      ar << likelihood_;
    }
    
    void load(iarchive& ar) {
      ar >> prior_;
      ar >> likelihood_;
    }

  }; // class prior_likelihood

  // Free functions
  //============================================================================

  //! multiplies two PL factors
  //! \relates prior_likelihood
  template <typename F, typename G>
  prior_likelihood<F,G>
  operator*(const prior_likelihood<F,G>& x, const prior_likelihood<F,G>& y) {
    // Handle the special cases more efficiently
    // (when the domain of one prior is a superset of the other)
    if (includes(x.arguments(), y.arguments())) {
      return prior_likelihood<F,G>(x.prior(), x.likelihood()*y.likelihood());
    }
    else if (includes(y.arguments(), x.arguments())) {
      return prior_likelihood<F,G>(y.prior(), x.likelihood()*y.likelihood());
    }
    else {
      F prior = x.prior() * y.prior() / x.prior().marginal(y.arguments());
      return prior_likelihood<F,G>(prior, x.likelihood()*y.likelihood());
    }
  }

  //! multiplies a PL factor with a likelihood
  template <typename F, typename G>
  prior_likelihood<F,G>
  operator*(prior_likelihood<F,G> x, const G& likelihood) {
    return x *= likelihood;
  }

  //! multiplies a PL factor with a likelihood
  template <typename F, typename G>
  prior_likelihood<F,G>
  operator*(const G& likelihood, prior_likelihood<F,G> x) {
    return x *= likelihood;
  }

  //! Outputs a PL factor to a stream
  //! \relates prior_likelihood
  template <typename F, typename G>
  std::ostream& operator<<(std::ostream& out, const prior_likelihood<F,G>& pl) {
    out << "(" << pl.prior() << "|" << pl.likelihood() << ")";
    return out;
  }

}

#include <sill/macros_undef.hpp>

#endif




