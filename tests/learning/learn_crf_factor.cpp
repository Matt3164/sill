#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/timer.hpp>

#include <sill/base/universe.hpp>
#include <sill/factor/random.hpp>
#include <sill/learning/crf/crf_parameter_learner.hpp>
#include <sill/learning/dataset/data_conversions.hpp>
#include <sill/learning/dataset/vector_assignment_dataset.hpp>
#include <sill/learning/learn_crf_factor.hpp>
#include <sill/model/random.hpp>
#include <sill/optimization/real_optimizer_builder.hpp>

#include <sill/macros_def.hpp>

/**
 * \file learn_crf_factor.cpp   Test learning CRF factors (crf_factor).
 */

//! Create a finite variable dataset for testing table log_reg CRF factors.
static void
create_finite_var_data
(sill::finite_var_vector& Y, sill::finite_var_vector& X,
 sill::finite_var_vector& YX, sill::table_factor& truth_YX,
 sill::table_factor& truth_Y_given_X, sill::table_factor& truth_X,
 boost::shared_ptr<sill::vector_assignment_dataset>& ds_ptr,
 sill::vector_assignment_dataset& test_ds,
 size_t ntrain, size_t ntest, size_t Ysize, size_t Xsize,
 sill::universe& u, boost::mt11213b& rng) {

  using namespace std;
  using namespace sill;

  // Fixed dataset parameters
  double lower = .001;
  double upper = 1;

  // Create P(Y | X), P(X)
  Y.clear();
  X.clear();
  for (size_t j(0); j < Ysize; ++j)
    Y.push_back(u.new_finite_variable(2));
  for (size_t j(0); j < Xsize; ++j)
    X.push_back(u.new_finite_variable(2));
  YX = sill::concat(Y, X);
  truth_YX =
    random_range_discrete_factor<table_factor>
    (make_domain<finite_variable>(YX), rng, lower, upper);
  truth_YX.normalize();
  truth_Y_given_X = truth_YX.conditional(make_domain<finite_variable>(X));
  truth_X = truth_YX.marginal(make_domain(X));

  cout << "Learning CRFs with Y = " << Y << "\n"
       << "          and with X = " << X << endl;
  if (truth_YX.size() < 100)
    cout << "True P(Y,X):\n" << truth_YX << endl;
  if (truth_X.size() < 100)
    cout << "True P(X):\n" << truth_X << endl;
  if (truth_Y_given_X.size() < 100)
    cout << "True P(Y|X):\n" << truth_Y_given_X << endl;

  // Generate a dataset
  cout << "Sampling " << (ntrain+ntest) << " samples from the model" << endl;
  ds_ptr.reset(new vector_assignment_dataset
               (YX, vector_var_vector(),
                std::vector<variable::variable_typenames>
                (YX.size(), variable::FINITE_VARIABLE)));
  for (size_t i(0); i < ntrain; ++i) {
    finite_assignment fa(truth_YX.sample(rng));
    ds_ptr->insert(assignment(fa));
  }
  test_ds =
    vector_assignment_dataset(YX, vector_var_vector(),
                              std::vector<variable::variable_typenames>
                              (YX.size(), variable::FINITE_VARIABLE));
  for (size_t i(0); i < ntest; ++i) {
    finite_assignment fa(truth_YX.sample(rng));
    test_ds.insert(assignment(fa));
  }
} // create_finite_var_data

//! Learn P(Y|X) via learn_crf_factor<F>().
//! @return <training log likelihood, test log likelihood>
template <typename F>
static std::pair<double,double>
test_learn_crf_factor
(double& learn_crf_factor_time, F* & f1,
 const typename sill::variable_type_group<typename F::output_variable_type>::var_vector_type& Y,
 const typename sill::variable_type_group<typename F::input_variable_type>::var_vector_type& X,
 boost::shared_ptr<sill::vector_assignment_dataset> ds_ptr,
 const sill::vector_assignment_dataset& test_ds, bool do_cv,
 const sill::crossval_parameters<F::regularization_type::nlambdas>& cv_params,
 typename F::parameters& f_params, boost::mt11213b& rng) {

  using namespace std;
  using namespace sill;

  typedef typename F::input_variable_type input_variable_type;
  typedef typename F::input_domain_type input_domain_type;
  typedef typename F::output_variable_type output_variable_type;
  typedef typename F::output_domain_type output_domain_type;

  boost::uniform_int<unsigned>
    unif_int(0,std::numeric_limits<unsigned>::max());

  std::vector<typename F::regularization_type> reg_params;
  vec means, stderrs;
  assert(f1 == NULL);
  boost::timer timer;
  if (do_cv) {
    f1 = learn_crf_factor_cv<F>
      (reg_params, means, stderrs, cv_params, ds_ptr,
       make_domain<output_variable_type>(Y),
       copy_ptr<input_domain_type>(new input_domain_type(X.begin(), X.end())),
       f_params, unif_int(rng));
  } else {
    f1 = learn_crf_factor<F>
      (ds_ptr, make_domain<output_variable_type>(Y),
       copy_ptr<input_domain_type>(new input_domain_type(X.begin(), X.end())),
       f_params, unif_int(rng));
  }
  learn_crf_factor_time = timer.elapsed();

  if (do_cv) {
    cout << "CV results for CRF factor learning:\n"
         << "lambdas: ";
    for (size_t j(0); j < reg_params.size(); ++j)
      cout << reg_params[j].lambdas << " ";
    cout << "\n"
         << "means:   " << means << "\n"
         << "stderrs: " << stderrs << "\n"
         << endl;

    size_t max_i = max_index(means);
    cout << "Chose lambda = " << reg_params[max_i].lambdas
         << ", with score = " << means[max_i] << "\n";
  }
  cout << "Learned factor:\n"
       << (*f1) << endl;

  double f_ll(0);
  foreach(const record& r, ds_ptr->records()) {
    typename F::output_factor_type f(f1->condition(r));
    f.normalize();
    f_ll += f.logv(r);
  }
  f_ll /= ds_ptr->size();
  double f_test_ll(0);
  foreach(const record& r, test_ds.records()) {
    typename F::output_factor_type f(f1->condition(r));
    f.normalize();
    f_test_ll += f.logv(r);
  }
  f_test_ll /= test_ds.size();

  return std::make_pair(f_ll, f_test_ll);
} // test_learn_crf_factor

//! Learn P(Y|X) via crf_parameter_learner.
//! @return <training log likelihood, test log likelihood>
template <typename F>
static std::pair<double,double>
test_crf_parameter_learner
(double& cpl_time, const F& f1,
 const typename
 sill::variable_type_group<typename F::output_variable_type>::var_vector_type& Y,
 const typename
 sill::variable_type_group<typename F::input_variable_type>::var_vector_type& X,
 boost::shared_ptr<sill::vector_assignment_dataset> ds_ptr,
 const sill::vector_assignment_dataset& test_ds, bool do_cv,
 size_t cpl_method, size_t line_search_type,
 const sill::crossval_parameters<F::regularization_type::nlambdas>& cv_params,
 boost::mt11213b& rng) {

  using namespace std;
  using namespace sill;

  typedef typename F::input_variable_type input_variable_type;
  typedef typename F::input_domain_type input_domain_type;
  typedef typename F::output_variable_type output_variable_type;
  typedef typename F::output_domain_type output_domain_type;

  boost::uniform_int<unsigned>
    unif_int(0,std::numeric_limits<unsigned>::max());

  // Learn via crf_parameter_learner.
  crf_model<F> tmp_true_model;
  tmp_true_model.add_factor(f1);
  gradient_method_parameters gradient_method_params;
  switch (line_search_type) {
  case 0:
    gradient_method_params.step_type = gradient_method_parameters::LINE_SEARCH;
    break;
  case 1:
    gradient_method_params.step_type =
      gradient_method_parameters::LINE_SEARCH_WITH_GRAD;
    break;
  default:
    assert(false);
  }
  gradient_method_params.convergence_zero = .00001;
  crf_parameter_learner_parameters cpl_params;
  cpl_params.gm_params = gradient_method_params;
  cpl_params.init_iterations = 100;
  cpl_params.opt_method =
    (real_optimizer_builder::real_optimizer_type)cpl_method;
  cpl_params.debug = 0;
  std::vector<typename F::regularization_type> cpl_reg_params;
  vec cpl_means, cpl_stderrs;
  boost::timer timer;
  if (do_cv) {
    cpl_params.lambdas =
      crf_parameter_learner<F>::choose_lambda
      (cpl_reg_params, cpl_means, cpl_stderrs, cv_params,
       tmp_true_model, false, *ds_ptr, cpl_params, 0, unif_int(rng));
  } else {
    cpl_params.lambdas = .01;
  }
  crf_parameter_learner<F> cpl(tmp_true_model, ds_ptr, false, cpl_params);
  cpl_time = timer.elapsed();

  if (do_cv) {
    cout << "CV results for CRF parameter learner:\n"
         << "lambdas: ";
    for (size_t j(0); j < cpl_reg_params.size(); ++j)
      cout << cpl_reg_params[j].lambdas << " ";
    cout << "\n"
         << "means:   " << cpl_means << "\n"
         << "stderrs: " << cpl_stderrs << "\n" << endl;

    size_t max_i = max_index(cpl_means);
    cout << "CRF parameter learner chose lambda = "
         << cpl_reg_params[max_i].lambdas
         << ", with score = " << cpl_means[max_i] << "\n";
  }

  cout << "Learned via CRF parameter learner:\n" << cpl.current_model() << "\n"
       << endl;

  cout << "crf_parameter_learner made " << cpl.iteration()
       << " calls to gradient, with "
       << cpl.objective_calls_per_iteration()
       << " avg calls to objective per gradient call."
       << endl;

  double cpl_ll(0);
  foreach(const record& r, ds_ptr->records()) {
    cpl_ll += cpl.current_model().log_likelihood(r);
  }
  cpl_ll /= ds_ptr->size();
  double cpl_test_ll(0);
  foreach(const record& r, test_ds.records()) {
    cpl_test_ll += cpl.current_model().log_likelihood(r);
  }
  cpl_test_ll /= test_ds.size();

  return std::make_pair(cpl_ll, cpl_test_ll);
} // test_crf_parameter_learner

template <typename F>
static void
print_results
(const sill::vector_assignment_dataset& ds,
 const sill::vector_assignment_dataset& test_ds,
 const sill::vector_assignment_dataset& orig_ds,
 const sill::vector_assignment_dataset& orig_test_ds,
 const typename sill::variable_type_group<typename F::input_variable_type>::var_vector_type& X,
 const typename F::output_factor_type& truth_YX,
 const typename F::output_factor_type& truth_Y_given_X,
 double learn_crf_factor_time, double cpl_time,
 std::pair<double,double> f_train_test_ll,
 std::pair<double,double> cpl_train_test_ll) {

  using namespace std;
  using namespace sill;

  // Compare the results.
  double joint_ll(0);
  foreach(const record& r, orig_ds.records()) {
    joint_ll += truth_YX.logv(r);
  }
  joint_ll /= ds.size();
  double true_ll(0);
  foreach(const record& r, orig_ds.records()) {
    typename F::output_factor_type
      f(truth_Y_given_X.restrict
        (r.assignment(domain(X.begin(), X.end()))));
    f.normalize();
    true_ll += f.logv(r);
  }
  true_ll /= ds.size();

  double joint_test_ll(0);
  foreach(const record& r, orig_test_ds.records()) {
    joint_test_ll += truth_YX.logv(r);
  }
  joint_test_ll /= test_ds.size();
  double true_test_ll(0);
  foreach(const record& r, orig_test_ds.records()) {
    typename F::output_factor_type
      f(truth_Y_given_X.restrict
        (r.assignment(domain(X.begin(), X.end()))));
    f.normalize();
    true_test_ll += f.logv(r);
  }
  true_test_ll /= test_ds.size();

  cout << "\nTime for CV and training CRF factor: " << learn_crf_factor_time
       << " seconds" << endl;
  cout << "Time for CV and training via CRF parameter learner: " << cpl_time
       << " seconds\n" << endl;

  cout << "Joint factor P(Y,X)'s training log likelihood: " << joint_ll
       << "\n"
       << "True factor P(Y|X)'s training log likelihood: " << true_ll
       << "\n"
       << "CRF factor's training log likelihood: " << f_train_test_ll.first
       << "\n"
       << "CRF parameter learner's training log likelihood: "
       << cpl_train_test_ll.first << "\n"
       << endl;
  cout << "Joint factor's test log likelihood: " << joint_test_ll
       << "\n"
       << "True factor's test log likelihood: " << true_test_ll << "\n"
       << "CRF factor's test log likelihood: " << f_train_test_ll.second
       << "\n"
       << "CRF parameter learner's test log likelihood: "
       << cpl_train_test_ll.second << "\n"
       << endl;

} // print_results

int main(int argc, char** argv) {

  using namespace sill;
  using namespace std;

  //==========================================================

  // Factor and dataset options
  std::string factor_type;
  size_t ntrain;
  size_t ntest;
  // gaussian_crf_factor options
  size_t Ysize;
  size_t Xsize;
  // Optimization options
  size_t cpl_method;
  size_t line_search_type;
  unsigned random_seed;

  namespace po = boost::program_options;
  po::options_description
    desc(std::string("Allowed options"));

  // Factor and dataset options
  desc.add_options()
    ("factor_type", po::value<std::string>(&factor_type),
     "CRF factor type (table/log_reg/gaussian)")
    ("ntrain", po::value<size_t>(&ntrain)->default_value(50),
     "Number of training samples (default = 50)")
    ("ntest", po::value<size_t>(&ntest)->default_value(500),
     "Number of test samples (default = 500)");
  // gaussian_crf_factor options
  desc.add_options()
    ("Ysize", po::value<size_t>(&Ysize)->default_value(5),
     "Number of Y (output) variables (default = 5)")
    ("Xsize", po::value<size_t>(&Xsize)->default_value(10),
     "Number of X (input) variables (default = 10)");
  // Optimization options
  desc.add_options()
    ("cpl_method",
     po::value<size_t>(&cpl_method)->default_value(1),
     "crf_parameter_learner optimization method (default = conjugate gradient)")
    ("line_search_type", po::value<size_t>(&line_search_type)->default_value(0),
     "crf_parameter_learner optimization line search method (default = 0 = objective only")
    ("random_seed",
     po::value<unsigned>(&random_seed)->default_value(time(NULL)),
     "random seed (default = time)");

  // Parse options
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  // Check options
  if (!vm.count("factor_type") ||
      ntrain == 0 || ntest == 0 || Ysize == 0 || Xsize == 0) {
    cout << desc << endl;
    return 1;
  }

  //==========================================================

  // Fixed learning parameters
  bool do_cv = true;

  universe u;
  boost::mt11213b rng(random_seed);

  if (factor_type == "table") { //=============================================
    finite_var_vector Y;
    finite_var_vector X;
    finite_var_vector YX;
    table_factor truth_YX;
    table_factor truth_Y_given_X;
    table_factor truth_X;
    boost::shared_ptr<vector_assignment_dataset> ds_ptr;
    vector_assignment_dataset test_ds;

    create_finite_var_data(Y, X, YX, truth_YX, truth_Y_given_X, truth_X,
                           ds_ptr, test_ds,
                           ntrain, ntest, Ysize, Xsize, u, rng);

    // CRF factor cross validation parameters
    crossval_parameters<table_crf_factor::regularization_type::nlambdas> cv_params;
    cv_params.nfolds = 2;
    cv_params.minvals = .001;
    cv_params.maxvals = 20.;
    cv_params.nvals = 2;
    cv_params.zoom = 1;
    cv_params.log_scale = true;
    // CRF factor learning parameters
    table_crf_factor::parameters tcf_params;
    tcf_params.reg.lambdas = .01;

    // Run tests
    double learn_crf_factor_time, cpl_time;
    table_crf_factor* f1 = NULL;
    std::pair<double,double> tcf_train_test_ll =
      test_learn_crf_factor<table_crf_factor>
      (learn_crf_factor_time, f1, Y, X, ds_ptr, test_ds,
       do_cv, cv_params, tcf_params, rng);
    std::pair<double,double> cpl_train_test_ll =
      test_crf_parameter_learner<table_crf_factor>
      (cpl_time, *f1, Y, X, ds_ptr, test_ds,
       do_cv, cpl_method, line_search_type, cv_params, rng);
    delete(f1);
    f1 = NULL;

    print_results<table_crf_factor>
      (*ds_ptr, test_ds, *ds_ptr, test_ds,
       X, truth_YX, truth_Y_given_X, learn_crf_factor_time, cpl_time,
       tcf_train_test_ll, cpl_train_test_ll);

  } else if (factor_type == "log_reg") {
    // TO DO: Test this with both finite- and vector-valued X variables.
    finite_var_vector Y;
    finite_var_vector X;
    finite_var_vector YX;
    table_factor truth_YX;
    table_factor truth_Y_given_X;
    table_factor truth_X;
    boost::shared_ptr<vector_assignment_dataset> ds_ptr;
    vector_assignment_dataset test_ds;

    create_finite_var_data(Y, X, YX, truth_YX, truth_Y_given_X, truth_X,
                           ds_ptr, test_ds,
                           ntrain, ntest, Ysize, Xsize, u, rng);

    // CRF factor cross validation parameters
    crossval_parameters<log_reg_crf_factor::regularization_type::nlambdas>
      cv_params;
    cv_params.nfolds = 2;
    cv_params.minvals = .001;
    cv_params.maxvals = 20.;
    cv_params.nvals = 2;
    cv_params.zoom = 1;
    cv_params.log_scale = true;
    // CRF factor learning parameters
    log_reg_crf_factor::parameters lrcf_params(u);
    lrcf_params.reg.lambdas = .01;

    var_vector Xalt(X.begin(), X.end());

    // Run tests
    double learn_crf_factor_time, cpl_time;
    log_reg_crf_factor* f1 = NULL;
    std::pair<double,double> tcf_train_test_ll =
      test_learn_crf_factor<log_reg_crf_factor>
      (learn_crf_factor_time, f1, Y, Xalt, ds_ptr, test_ds,
       do_cv, cv_params, lrcf_params, rng);
    std::pair<double,double> cpl_train_test_ll =
      test_crf_parameter_learner<log_reg_crf_factor>
      (cpl_time, *f1, Y, Xalt, ds_ptr, test_ds,
       do_cv, cpl_method, line_search_type, cv_params, rng);
    delete(f1);
    f1 = NULL;

    print_results<log_reg_crf_factor>
      (*ds_ptr, test_ds, *ds_ptr, test_ds,
       Xalt, truth_YX, truth_Y_given_X, learn_crf_factor_time, cpl_time,
       tcf_train_test_ll, cpl_train_test_ll);

  } else if (factor_type == "gaussian") { //===================================
    // Fixed dataset parameters
    double b_max = 5;
    double spread = 2;
    double cov_strength = 1;
    // Fixed learning parameters
    bool normalize_data = false;
    // Gaussian CRF factor cross validation parameters
    crossval_parameters<gaussian_crf_factor::regularization_type::nlambdas>
      cv_params;
    cv_params.nfolds = 2;
    cv_params.minvals = .001;
    cv_params.maxvals = 20.;
    cv_params.nvals = 2;
    cv_params.zoom = 1;
    cv_params.log_scale = true;
    // CRF factor learning parameters
    gaussian_crf_factor::parameters gcf_params;
    gcf_params.reg.lambdas = .01;

    // Create P(Y | X), P(X)
    vector_var_vector Y, X;
    for (size_t j(0); j < Ysize; ++j)
      Y.push_back(u.new_vector_variable(1));
    for (size_t j(0); j < Xsize; ++j)
      X.push_back(u.new_vector_variable(1));
    vector_var_vector YX(sill::concat(Y, X));
    moment_gaussian truth_YX(make_marginal_gaussian_factor
                             (YX, b_max, spread, cov_strength, rng));
    truth_YX.normalize();
    if (1) { // Make sure this is a valid Gaussian.
      canonical_gaussian cg1(truth_YX);
      canonical_gaussian cg2(truth_YX);
      cg2.enforce_psd(truth_YX.mean());
      if (cg1.inf_matrix() != cg2.inf_matrix())
        assert(false);
      if (cg1.inf_vector() != cg2.inf_vector())
        assert(false);
      if (cg1.log_multiplier() != cg2.log_multiplier())
        assert(false);
    }
    moment_gaussian
      truth_Y_given_X(truth_YX.conditional(make_domain<vector_variable>(X)));
    moment_gaussian truth_X(truth_YX.marginal(make_domain(X)));

    cout << "Learning CRFs with Y = " << Y << "\n"
         << "          and with X = " << X << endl;
    cout << "True P(Y|X):\n" << truth_Y_given_X << endl;

    // Generate a dataset
    cout << "Sampling " << (ntrain+ntest) << " samples from the model" << endl;
    boost::shared_ptr<vector_assignment_dataset>
      ds_ptr(new vector_assignment_dataset
             (finite_var_vector(), YX, 
              std::vector<variable::variable_typenames>
              (YX.size(), variable::VECTOR_VARIABLE)));
    for (size_t i(0); i < ntrain; ++i) {
      vector_assignment fa(truth_YX.sample(rng));
      ds_ptr->insert(assignment(fa));
    }
    vector_assignment_dataset
      test_ds(finite_var_vector(), YX, 
              std::vector<variable::variable_typenames>
              (YX.size(), variable::VECTOR_VARIABLE));
    for (size_t i(0); i < ntest; ++i) {
      vector_assignment fa(truth_YX.sample(rng));
      test_ds.insert(assignment(fa));
    }
    vector_assignment_dataset orig_ds(ds_ptr->datasource_info());
    foreach(const record& r, ds_ptr->records())
      orig_ds.insert(r);
    vector_assignment_dataset orig_test_ds(test_ds.datasource_info());
    foreach(const record& r, test_ds.records())
      orig_test_ds.insert(r);

    if (normalize_data) {
      std::pair<vec, vec> means_stddevs(ds_ptr->normalize());
      test_ds.normalize(means_stddevs.first, means_stddevs.second);
    }

    // Run tests
    double learn_crf_factor_time, cpl_time;
    gaussian_crf_factor* f1 = NULL;
    std::pair<double,double> gcf_train_test_ll =
      test_learn_crf_factor<gaussian_crf_factor>
      (learn_crf_factor_time, f1, Y, X, ds_ptr, test_ds,
       do_cv, cv_params, gcf_params, rng);
    std::pair<double,double> cpl_train_test_ll =
      test_crf_parameter_learner<gaussian_crf_factor>
      (cpl_time, *f1, Y, X, ds_ptr, test_ds,
       do_cv, cpl_method, line_search_type, cv_params, rng);
    delete(f1);
    f1 = NULL;

    print_results<gaussian_crf_factor>
      (*ds_ptr, test_ds, orig_ds, orig_test_ds,
       X, truth_YX, truth_Y_given_X, learn_crf_factor_time, cpl_time,
       gcf_train_test_ll, cpl_train_test_ll);

  } else { //==================================================================
    cout << desc << endl;
    return 1;
  }

  return 0;
}