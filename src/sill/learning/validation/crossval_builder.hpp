
#ifndef SILL_CROSSVAL_BUILDER_HPP
#define SILL_CROSSVAL_BUILDER_HPP

#include <boost/program_options.hpp>

#include <sill/base/string_functions.hpp>
#include <sill/learning/validation/crossval_parameters.hpp>

namespace sill {

  /**
   * Class for parsing command-line options for cross validation.
   */
  class crossval_builder {

    size_t nfolds;

    vec minvals;

    vec maxvals;

    ivec nvals;

    size_t zoom;

    bool real_scale;

  public:

    crossval_builder() { }

    /**
     * Add options to the given Options Description.
     * Once the Options Description is used to parse argv, this struct will
     * hold the specified values.
     */
    void add_options(boost::program_options::options_description& desc,
                     const std::string& desc_prefix = "");

    //! Return the CV options specified in this builder.
    //! @param N   Dimensionality of parameter vector.
    crossval_parameters get_parameters(size_t N) {
      crossval_parameters params(N);
      params.nfolds = nfolds;
      if (minvals.size() == N) {
        params.minvals = minvals;
      } else if (minvals.size() == 1) {
        params.minvals = minvals[0];
      } else {
        throw std::invalid_argument
          ("crossval_builder given minvals of length " +
           to_string(minvals.size()) + " but expected length " + to_string(N));
      }
      if (maxvals.size() == N) {
        params.maxvals = maxvals;
      } else if (maxvals.size() == 1) {
        params.maxvals = maxvals[0];
      } else {
        throw std::invalid_argument
          ("crossval_builder given maxvals of length " +
           to_string(maxvals.size()) + " but expected length " + to_string(N));
      }
      if (nvals.size() == N) {
        params.nvals = nvals;
      } else if (nvals.size() == 1) {
        params.nvals = nvals[0];
      } else {
        throw std::invalid_argument
          ("crossval_builder given nvals of length " +
           to_string(nvals.size()) + " but expected length " + to_string(N));
      }
      params.zoom = zoom;
      params.log_scale = !real_scale;
      return params;
    } // get_parameters

  }; // class crossval_builder

} // namespace sill

#endif // #ifndef SILL_CROSSVAL_BUILDER_HPP