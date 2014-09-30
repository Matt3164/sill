#ifndef SILL_SYN_ORACLE_KNORM_HPP
#define SILL_SYN_ORACLE_KNORM_HPP

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/random/variate_generator.hpp>

#include <sill/base/finite_variable.hpp>
#include <sill/base/vector_variable.hpp>
#include <sill/learning/dataset_old/oracle.hpp>

#include <sill/macros_def.hpp>

// uncomment to print debugging information
//#define SILL_SYN_ORACLE_KNORM_HPP_VERBOSE

namespace sill {

  /**
   * Class for generating synthetic data from k Gaussians.
   * This picks k centers in a hypercube, with the centers placed
   * uniformly at random, and then rescales to make them an average distance of
   * 2 * RADIUS from their closest neighbors.
   * Datapoints are then generated by picking a center uniformly at random
   * and then adding Gaussian noise along each dimension with zero mean and
   * STD_DEV standard deviation.
   * Note: This generalizes the twonorm dataset from Brieman (1998);
   *       the default parameters for this class are those for twonorm
   *       (if there are 20 variables) and simulate twonorm up to a rotation.
   *
   * \author Joseph Bradley
   * \ingroup learning_dataset
   * @todo Think of a better way to choose the default radius.
   */
  class syn_oracle_knorm : public oracle<dense_linear_algebra<> > {

    // Public type declarations
    //==========================================================================
  public:

    typedef dense_linear_algebra<> la_type;

    //! The base type (oracle)
    typedef oracle<la_type> base;

    typedef record<la_type> record_type;

    struct parameters {

      //! .5 * Gaussians' centers' average distances from each other
      //!  (default = 4 / sqrt((# non-class variables) / NMEANS))
      double radius;

      //! Standard deviation of Gaussians
      //!  (default = sqrt(12))
      double std_dev;

      //! Used to make this algorithm deterministic.
      //!  (default = time)
      double random_seed;

      parameters()
        : radius(0), std_dev(sqrt(12.)) {
        std::time_t time_tmp;
        time(&time_tmp);
        random_seed = time_tmp;
      }

      bool valid() const {
        if (radius <= 0)
          return false;
        if (std_dev <= 0)
          return false;
        return true;
      }

    protected:

      friend class syn_oracle_knorm;

      //! Set radius to default if it has not been set yet
      void set_radius(size_t nfeatures, size_t nmeans) {
        if (radius == 0)
          radius = 4. / sqrt(static_cast<double>(nfeatures) / nmeans);
      }

    };  // class parameters

    // Private data members
    //==========================================================================
  private:

    parameters params;

    //! NMEANS (passed via var_order, not as a parameter)
    size_t nmeans_;

    //! NFEATURES (passed via var_order, not as a parameter)
    size_t nfeatures_;

    //! centers[k] = center k
    std::vector<vec> centers;

    //! random number generator
    boost::mt11213b rng;
/*
    //! normal distribution
    boost::normal_distribution<double> normal_dist;
*/
    //! NDRAWS = # draws from uniform to generate normal
    //!  (treated as constant, not as a parameter)
    static const size_t NDRAWS = 200;

    //! Large integer (for drawing from normal distribution)
    const size_t BIG_INT;

    //! uniform distribution over centers
    boost::uniform_smallint<> discrete_uniform_dist;

    //! Current record
    record_type current_rec;

    // Private methods
    //==========================================================================

    //! Initialize the oracle (after the parameters have been set)
    void init();

    /**
     * Draw from a normal distribution N(0,params.std_dev).
     * This uses the same method Boost does, but I can't get the Boost
     * normal_distribution to work for some reason.  Plus, this will let us
     * change the number of draws from the uniform distribution.
     */
    double normal_dist();

    // Constructors
    //==========================================================================
  public:

    /**
     * Constructs a synthetic oracle for knorm data.
     * @param vector_var_order  vector variables for dataset features
     * @param class_variable    finite variable for class
     * @param var_type_order    order of variable types
     */
    syn_oracle_knorm(const vector_var_vector& vector_var_order,
                     finite_variable* class_variable,
                     const std::vector<variable::variable_typenames>& var_type_order,
                     parameters params = parameters())
      : base(finite_var_vector(1,class_variable), vector_var_order,
             var_type_order),
        params(params),
        BIG_INT(std::numeric_limits<boost::uniform_int<>::input_type>::max()),
        current_rec(finite_numbering_ptr_, vector_numbering_ptr_, dvector) {
      for (size_t j = 0; j < vector_seq.size(); j++)
        assert(vector_seq[j]->size() == 1);
      assert(class_variable != NULL);
      nmeans_ = class_variable->size();
      nfeatures_ = vector_var_order.size();
      this->params.set_radius(nfeatures_, nmeans_);

      finite_class_vars.push_back(class_variable);
      assert(std::log(static_cast<double>(nmeans_))/std::log(2.) < nfeatures_);
      init();
    }

    // Getters and helpers
    //==========================================================================

    //! Returns the current record.
    const record_type& current() const {
      return current_rec;
    }

    //! Return the centers of the k Gaussians
    const std::vector<vec>& center_list() const {
      return centers;
    }

    template <typename Char, typename Traits>
    void write(std::basic_ostream<Char, Traits>& out) const {
      out << "knorm oracle\n";
      out << " parameters: nmeans="<< nmeans_ << ", nfeatures=" << nfeatures_
          << ", radius=" << params.radius << ", std_dev=" << params.std_dev
          << ", random_seed=" << params.random_seed << ", NDRAWS=" << NDRAWS
          << ", BIG_INT=" << BIG_INT << "\n";
      out << " Means:\n";
      for (size_t j = 0; j < vector_seq.size(); ++j)
        out << "\t" << vector_seq[j];
      out << "\n";
      for (size_t k = 0; k < nmeans_; k++) {
        for (size_t j = 0; j < nfeatures_; j++)
          out << "\t" << centers[k][j];
        out << std::endl;
      }
    }

    // Mutating operations
    //==========================================================================

    //! Increments the oracle to the next record.
    //! This returns true iff the operation was successful; false indicates
    //! a depleted oracle.
    //! NOTE: This must be explicitly called after the oracle is constructed.
    bool next();

    //! Test generator for normal distribution with i draws.
    void test_normal_distribution(size_t i) {
      std::cout << "syn_oracle_knorm: test_normal_distribution:\n";
      for (size_t k = 0; k < i; k++)
        std::cout << "\t" << normal_dist() << std::endl;
    }

  }; // class syn_oracle_knorm

  // Free functions
  //==========================================================================

  template <typename Char, typename Traits>
  std::basic_ostream<Char,Traits>&
  operator<<(std::basic_ostream<Char, Traits>& out,
             const syn_oracle_knorm& knorm) {
    knorm.write(out);
    return out;
  }

  /**
   * Constructs a synthetic oracle for knorm data, creating new variables
   * in universe u.
   * @param k         number of means/classes
   * @param nfeatures number of features (non-class variables)
   * @param u         universe
   * @param params    other parameters for syn_oracle_knorm
   * \todo Merge this with functions in data_loader.
   */
  syn_oracle_knorm
  create_syn_oracle_knorm
  (size_t k, size_t nfeatures, universe& u,
   syn_oracle_knorm::parameters params = syn_oracle_knorm::parameters());

} // namespace sill

#include <sill/macros_undef.hpp>

#endif // #ifndef SILL_SYN_ORACLE_KNORM_HPP