#ifndef SILL_LINE_SEARCH_BUILDER_HPP
#define SILL_LINE_SEARCH_BUILDER_HPP

#include <boost/program_options.hpp>

#include <sill/optimization/line_search/line_search.hpp>
#include <sill/optimization/line_search/backtracking_line_search.hpp>
#include <sill/optimization/line_search/exponential_decay_search.hpp>
#include <sill/optimization/line_search/slope_binary_search.hpp>
#include <sill/optimization/line_search/value_binary_search.hpp>
#include <sill/optimization/line_search/wolfe_conditions.hpp>

namespace sill {

  // TODO defaults

  /**
   * Class for parsing command-line options that specify line saerch
   * parameters.
   * \tparam Vec the optimization vector type
   */
  template <typename Vec>
  class line_search_builder {
  public:
    typedef typename Vec::value_type real_type;

    line_search_builder() { }

    /**
     * Add options to the given Options Description.
     * Once the Options Description is used to parse argv, this struct will
     * hold the specified values.
     */
    void add_options(boost::program_options::options_description& desc,
                     const std::string& desc_prefix = "") {
      namespace po = boost::program_options;
      po::options_description sub_desc(desc_prefix + "line search options");
      sub_desc.add_options()
        ("algorithm",
         po::value<std::string>(&type)->default_value("decay"),
         "The line search algorithm (decay/backtrack/value_binary/slope_binary")
        ("initial",
         po::value<real_type>(&decay.initial),
         "Initial step size")
        ("rate",
         po::value<real_type>(&decay.rate),
         "The rate for the exponential decay")
        ("acceptance",
         po::value<real_type>(&backtrack.acceptance),
         "Acceptable decrease of the objective based on linear extrapolation")
        ("discount",
         po::value<real_type>(&backtrack.discount),
         "Discount factor by which step is shrunk during line search")
        ("convergence",
         po::value<real_type>(&bracketing.convergence),
         "The size of the bracket at which point bracketing declares convergence")
        ("multiplier",
         po::value<real_type>(&bracketing.multiplier),
         "Value by which the step size is multiploied / divided by in initial bracketing")
        ("min_step",
         po::value<real_type>(&bracketing.min_step),
         "Minimum allowable step size")
        ("max_step",
         po::value<real_type>(&bracketing.max_step),
         "Maximum allowable step size")
        ("c1",
         po::value<real_type>(&wolfe.c1),
         "The parameter controlling the Armijo rule of the Wolfe conditions")
        ("c2",
         po::value<real_type>(&wolfe.c2),
         "The parameter controlling the curvature Wolfe condition")
        ("strong",
         po::value<bool>(&wolfe.strong),
         "If true, use the strong Wolfe conditions");
      desc.add(sub_desc);
    }

    /**
     * Return the line saerch object with parameters set according to the
     * command-line options.
     */
    line_search<Vec>* get() const {
      backtrack.min_step = bracketing.min_step;
      if (algorithm == "decay") {
        return new exponential_decay_search(decay);
      }
      if (algorithm == "backtrack") {
        return new backtracking_line_search(f, g, backtrack);
      }
      if (algorithm == "value_binary") {
        return new value_binary_search(f, bracketing);
      }
      if (algorithm == "slope_bianry") {
        return new slope_binary_saerch(f, g, bracketing);
        // TODO: Wolfe conditions
      }
      throw std::invalid_argument("Invalid algorithm");
    }

  private:
    std::string algorithm;
    real_type min_step;
    real_type max_step;
    exponential_decay_search_parameters<real_type> decay;
    backtracking_line_search_parameters<real_type> backtrack;
    bracketing_line_search_parameters<real_type>   bracketing;

  }; // class line_search_builder

} // namespace sill

#endif
