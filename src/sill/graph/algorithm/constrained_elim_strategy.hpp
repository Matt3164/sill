#ifndef SILL_CONSTRAINED_ELIM_STRATEGY_HPP
#define SILL_CONSTRAINED_ELIM_STRATEGY_HPP

#include <sill/global.hpp>
#include <sill/graph/concepts.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  /**
   * Implements an elimination strategy subject to an elimination
   * order constraint.  The elimination (partial) order is specified
   * by an intrinsic priority associated with each vertex.  For
   * example, if some vertices have intrinsic priority 0 and others
   * have intrinsic priority 1, then all vertices with intrinsic
   * priority 1 will be eliminated before those with intrinsic
   * priority 0. This type models the EliminationStrategy concept.
   *
   * @tparam Function
   *        The type of a functor which computes a vertex's intrinsic
   *        priority given a vertex descriptor and a graph.  This type
   *        must define a type Function::result_type which is the type
   *        of the intrinsic priorities, and it must also
   *        define a binary operator() which accepts a vertex descriptor
   *        and a graph, and returns a priority of type
   *        IntrinsicPriority::result_type.
   * @tparam Strategy
   *        A type that models the EliminationStrategy concept.
   *        It is used to choose the relative elimination order
   *        of vertices with the same intrinsic primary priority.
   *
   * \ingroup graph_types
   */
  template <typename Function, typename Strategy>
  struct constrained_elim_strategy {

    /**
     * The type of the priority of this constrained elimination
     * strategy.  Note that the ordering defined on std::pair
     * implements the correct semantics for ordering vertices given
     * the primary and secondary priorities.
     */
    typedef std::pair<typename Function::result_type,
                      typename Strategy::priority_type> priority_type;

    //! The functor that computes the primary priority.
    Function intrinsic_priority;

    //! The secondary strategy.
    Strategy secondary_strategy;

    //! Default constructor. Only applicable when Function and Strategy are
    //! default-constructible.
    constrained_elim_strategy() { }

    //! Constructor.
    constrained_elim_strategy(Function intrinsic_priority,
                              Strategy secondary_strategy)
      : intrinsic_priority(intrinsic_priority),
        secondary_strategy(secondary_strategy)
    { }

    /**
     * Computes the priority of a vertex using the intrinsic priority
     * and the secondary strategy.
     */
    template <typename Graph>
    priority_type priority(typename Graph::vertex v, const Graph& g) {
      concept_assert((EliminationStrategy<Strategy, Graph>));
      return std::make_pair(intrinsic_priority(v, g),
                            secondary_strategy.priority(v, g));
    }

    /**
     * Computes the set of vertices whose priority may change if a
     * designated vertex is eliminated.  Since the intrinsic priority
     * does not change during the elimination process, this is the
     * same set of vertices as reported by the secondary strategy.
     */
    template <typename Graph, typename OutIt>
    void updated(typename Graph::vertex v, const Graph& g, OutIt updated) {
      concept_assert((EliminationStrategy<Strategy, Graph>));
      concept_assert((OutputIterator<OutIt, typename Graph::vertex>));
      secondary_strategy.updated(v, g, updated);
    }

  }; // struct constrained_elim_strategy

} // namespace sill

#include <sill/macros_undef.hpp>

#endif
