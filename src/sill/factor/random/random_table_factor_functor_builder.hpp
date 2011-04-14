#ifndef SILL_RANDOM_TABLE_FACTOR_FUNCTOR_BUILDER_HPP
#define SILL_RANDOM_TABLE_FACTOR_FUNCTOR_BUILDER_HPP

#include <sill/factor/random/random_factor_functor_builder_i.hpp>
#include <sill/factor/random/random_table_factor_functor.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  //! \addtogroup factor_random
  //! @{

  /**
   * Helper struct for random_table_factor_functor which allows easy parsing
   * of command-line options via Boost Program Options.
   *
   * Usage: Create your own Options Description desc.
   *        Call this struct's add_options() method with desc.
   *        Parse the command line using the modified options description.
   *        Use this struct's create_functor() method to create a functor with
   *        the parsed options.
   */
  struct random_table_factor_functor_builder
    : random_factor_functor_builder_i<random_table_factor_functor> {

    typedef random_table_factor_functor rff_type;

    random_table_factor_functor_builder()
      : factor_choice_string("random_range") { }

    /**
     * Add options to the given Options Description.
     * Once the Options Description is used to parse argv, this struct will
     * hold the specified values.
     */
    void add_options(boost::program_options::options_description& desc);

    /**
     * Add options to the given Options Description.
     * Once the Options Description is used to parse argv, this struct will
     * hold the specified values.
     *
     * @param opt_prefix  Prefix added to command line option names.
     *                    This is useful when using multiple functor instances.
     */
    void add_options(boost::program_options::options_description& desc,
                     const std::string& opt_prefix);

    //! Check options.  Assert false if invalid.
    void check() const;

    //! Get the parsed options.
    const rff_type::parameters& get_parameters() const;

  private:
    std::string factor_choice_string;

    mutable random_table_factor_functor::parameters params;

  }; // struct random_table_factor_functor_builder

  //! @} group factor_random

} // namespace sill

#include <sill/macros_undef.hpp>

#endif // SILL_RANDOM_TABLE_FACTOR_FUNCTOR_BUILDER_HPP