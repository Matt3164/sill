
#ifndef SILL_LEARNING_DISCRIMINATIVE_CLASSIFIER_CASCADE_HPP
#define SILL_LEARNING_DISCRIMINATIVE_CLASSIFIER_CASCADE_HPP

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

#include <sill/learning/dataset/ds_oracle.hpp>
#include <sill/learning/dataset/vector_dataset.hpp>
#include <sill/learning/discriminative/binary_classifier.hpp>
#include <sill/math/statistics.hpp>
#include <sill/stl_io.hpp>

#include <sill/macros_def.hpp>

// set to 1 to print debugging information
#define DEBUG_CLASSIFIER_CASCADE 1

namespace sill {

  struct classifier_cascade_parameters {

    /**
     * Empty classifiers to use to construct the base classifiers.  The
     * parameters from these classifiers are used for the base classifiers,
     * save for random seeds, which are generated by the cascade classifier.
     * If the number of base classifiers becomes > BASE_CLASSIFIERS.size(),
     * then the last classifier from BASE_CLASSIFIERS is used to construct
     * the extra base classifiers.
     *  (required)
     */
    std::vector<boost::shared_ptr<binary_classifier> > base_classifiers;

    //! Initial number of base classifiers to build.
    //!  (default = 0)
    size_t init_base_classifiers;

    //! Label/class which is rare
    //!  (required)
    size_t rare_class;

    /**
     * Max misclassification rate for the
     * rare class for each level of the cascade
     * (If RARE_CLASS = 0, this is the false positive rate;
     *  if RARE_CLASS = 1, this is the false negative rate.)
     * This should be fairly high (say .95 to .99).
     *  (required)
     */
    double max_false_common_rate;

    //! Number of training examples to pass to the base dataset,
    //! including all examples from the rare class.
    //!  (default = 2 * number of examples in rare class)
    size_t base_dataset_size;

    //! Used to make the algorithm deterministic
    //!  (default = time)
    double random_seed;

    /**
     * Max number of examples which can be thrown
     * out to generate one example from the common class.
     * This resets to the default if it has value 0.
     *  (default = 100 / (1 - MAX_FALSE_COMMON_RATE), or
     *           = 100000 if the rate is 1)
     * @todo MAX_FILTER_COUNT should be set more intelligently.
     */
    size_t max_filter_count;

    classifier_cascade_parameters()
      : base_classifiers(), init_base_classifiers(0),
        rare_class(2), max_false_common_rate(2),
        base_dataset_size(0), max_filter_count(0) {
      std::time_t time_tmp;
      time(&time_tmp);
      random_seed = time_tmp;
    }

    bool valid() const {
      if (rare_class != 0 && rare_class != 1)
        return false;
      if (max_false_common_rate < 0 || max_false_common_rate > 1)
        return false;
      return true;
    }

    void set_check_params(size_t n_rare_exs) {
      if (base_dataset_size == 0) { // then set to default
        base_dataset_size = 2 * n_rare_exs;
      } else
        assert(base_dataset_size > n_rare_exs);
      if (max_filter_count == 0) { // then set to default
        if (max_false_common_rate == 1)
          max_filter_count = 100000;
        else
          max_filter_count = 100 / (1. - max_false_common_rate);
      }
    }

    void save(std::ofstream& out) const {
      out << base_classifiers.size() << " " << init_base_classifiers << " "
          << rare_class << " " << max_false_common_rate << " "
          << base_dataset_size << " " << random_seed << " "
          << max_filter_count << "\n";
      for (size_t t = 0; t < base_classifiers.size(); ++t)
        base_classifiers[t]->save(out);
    }

    void load(std::ifstream& in, const datasource& ds);

  };  // class classifier_cascade_parameters

  /**
   * Cascade of increasingly complex/accurate binary classifiers, useful
   * for rapid classification with highly imbalanced classes.
   *
   * This is based on the cascades used by Viola and Jones for face detection.
   * The classifier cascade is given a range of empty binary classifiers,
   * and it trains these iteratively on a combination of examples from a fixed
   * set and of examples generated from a given oracle.  The examples from
   * the oracle are passed through a classifier_filter_oracle which can, e.g.,
   * only select examples which the current cascade misclassifies.
   *
   * At test time, the cascade works as follows:
   *  - The test example is given to the first classifier.
   *  - The first classifier's predict_raw() value is compared to
   *    a threshold (chosen at training time).
   *  - If the rare class is 1 (0), then if predict_raw() is greater (less)
   *    than this threshold, then the example is passed to the next classifier
   *    in the cascade.  Otherwise, it is classified as 0 (1).
   *
   * At training time, the cascade:
   *  - Builds a dataset using a fixed set of examples (which should be
   *    examples from the rare class) and new examples from an oracle (which
   *    should provide examples from the common class)
   *  - Trains the first base classifier
   *  - Chooses a threshold such that, if the rare class is 1 (0),
   *    using the prediction rule
   *    (base_classifier.predict_raw() > threshold ? 1 : 0)
   *    gives a low false negative (positive) rate.
   *  - Iterates, training the next base classifiers in the same way.
   *    In the following iterations, though, the examples from the oracle
   *    are filtered such that only examples which are misclassified by the
   *    current set of cascades are accepted.
   *
   * NOTE: This must be used with a margin-based classifier.
   *
   * \author Joseph Bradley
   * \ingroup learning_discriminative
   * @todo serialization
   */
  class classifier_cascade : public binary_classifier {

    // Protected data members
    //==========================================================================
  protected:

    typedef binary_classifier base;

    classifier_cascade_parameters params;

    // copied from params:
    size_t max_filter_count_;

    //! random number generator
    boost::mt11213b rng;

    //! Dataset to be passed to base learners
    //! The first fixed_ds_size examples are from the fixed dataset,
    //! and the remaining ones are from the oracle.
    vector_dataset base_ds;

    //! Number of rare class examples permanently stored in base_ds.
    size_t rare_ds_size;

    //! for constructing an empty classifier_cascade
    ds_oracle* ds_o_ptr;

    //! Oracle for common class
    oracle& common_o;

    //! Used for choosing a threshold (and avoiding reallocation)
    vec base_ds_preds;

    //! Base classifiers
    std::vector<boost::shared_ptr<binary_classifier> > base_classifiers;

    //! Thresholds for base classifiers
    std::vector<double> thresholds;

    // Protected methods
    //==========================================================================

    void init(const dataset& rare_ds);

    //! Advance the oracle to the next example which is misclassified.
    //! @return true iff a valid next example has been found
    bool next_example();

    // Constructors and destructors
    //==========================================================================
  public:

    /**
     * Constructor for a cascade of binary classifiers without associated data;
     * useful for:
     *  - creating other instances
     *  - loading a saved cascade
     * @param params     algorithm parameters
     */
    explicit classifier_cascade(classifier_cascade_parameters params)
      : params(params), base_ds(), rare_ds_size(0),
        ds_o_ptr(new ds_oracle(base_ds)), common_o(*ds_o_ptr) {
    }

    /**
     * Constructor for a cascade of binary classifiers.
     * @param rare_ds    dataset for rare class
     * @param common_o   oracle for common class
     * @param params     algorithm parameters
     */
    classifier_cascade(const dataset& rare_ds, oracle& common_o,
                       classifier_cascade_parameters params)
      : base(rare_ds), params(params), base_ds(rare_ds.datasource_info()),
        rare_ds_size(rare_ds.size()), ds_o_ptr(NULL), common_o(common_o) {
      assert(rare_ds.is_weighted() == false);
      init(rare_ds);
    }

    ~classifier_cascade() {
      if (ds_o_ptr != NULL)
        delete(ds_o_ptr);
    }

    //! Warning: This should not be used for this class!
    boost::shared_ptr<binary_classifier> create(statistics& stats) const {
      assert(false);
      return boost::shared_ptr<binary_classifier>();
    }

    //! Warning: This should not be used for this class!
    boost::shared_ptr<binary_classifier> create(oracle& o, size_t n) const {
      assert(false);
      return boost::shared_ptr<binary_classifier>();
    }

    // Getters and helpers
    //==========================================================================

    //! Return a name for the algorithm without template parameters.
    std::string name() const {
      return "classifier_cascade";
    }

    //! Return a name for the algorithm with comma-separated template parameters
    //! (e.g., objective).
    std::string fullname() const {
      return name();
    }

    //! Returns true iff learner is naturally an online learner.
    bool is_online() const {
      return false;
    }

    //! Returns the current iteration number (from 0)
    //!  (i.e., the number of boosting iterations completed).
    size_t iteration() const {
      return base_classifiers.size();
    }

    //! Computes the accuracy after each iteration on a test set.
    std::vector<double> test_accuracies(const dataset& testds) const {
      assert(false);
    }

    // Learning and mutating operations
    //==========================================================================

    //! Reset the random seed in this algorithm's parameters and in its
    //! random number generator.
    void random_seed(double value) {
      params.random_seed = value;
      rng.seed(static_cast<unsigned>(value));
    }

    //! Train the next level of the cascade.
    //! @return  false iff the cascade may not be trained further
    bool step();

    // Prediction methods
    //==========================================================================

    //! Predict the 0/1 label of a new example.
    std::size_t predict(const assignment& example) const;

    //! Predict the 0/1 label of a new example.
    std::size_t predict(const record& example) const;

    // Save and load methods
    //==========================================================================

    using base::save;
    using base::load;

    //! Output the learner to a human-readable file which can be reloaded.
    //! @param save_part  0: save function (default), 1: engine, 2: shell
    //! @param save_name  If true, this saves the name of the learner.
    void save(std::ofstream& out, size_t save_part = 0,
              bool save_name = true) const;

    /**
     * Input the classifier from a human-readable file.
     * @param in          input filestream for file holding the saved learner
     * @param ds          datasource used to get variables
     * @param load_part   0: load function (default), 1: engine, 2: shell
     * This assumes that the learner name has already been checked.
     * @return true if successful
     */
    bool load(std::ifstream& in, const datasource& ds, size_t load_part);

  }; // class classifier_cascade

} // namespace sill

#include <sill/macros_undef.hpp>

#endif // #ifndef SILL_LEARNING_DISCRIMINATIVE_CLASSIFIER_CASCADE_HPP