#ifndef SILL_DECOMPOSABLE_ITERATOR_HPP
#define SILL_DECOMPOSABLE_ITERATOR_HPP

#include <vector>

#include <boost/tuple/tuple.hpp>

#include <sill/set.hpp>
#include <sill/subset_iterator.hpp>
#include <sill/map.hpp>
#include <sill/factor/factor.hpp>
#include <sill/factor/table_factor.hpp>
#include <sill/model/junction_tree.hpp>
#include <sill/model/decomposable.hpp>

#include <sill/macros_def.hpp>

/**
 * \file decomposable_iterator.hpp  Outdated iterator class; I need to
 *                                  reorganize structure search.
 */

namespace sill {

  //! Initial model for search.
  enum structure_initial_enum {
    //! Empty model: single-variable cliques.
    INIT_MODEL_EMPTY,
    //! Star-shaped junction tree with maximal cliques
    INIT_MODEL_STAR
  };

  //! Types of steps allowed.
  enum structure_step_enum {
    //! Extend subtree induced by a variable by adding variable to a node.
    STEP_LOCAL_EXTEND_SUBTREE,
    /**
     * Retract subtree induced by a variable by removing variable from a node.
     * This might disconnect subtrees.  This may also create a singleton
     * clique.
     */
    STEP_LOCAL_RETRACT_SUBTREE,
    //! Replace variable in one node with variable in adjacent node.
    STEP_LOCAL_PUSH_BACK_SUBTREE,
    //! Split node into 2 nodes, each with fewer variables.
    STEP_LOCAL_SPLIT_NODE,
    //! Combine adjacent nodes.
    STEP_LOCAL_COMBINE_NODES,
    //! Create a new node between two adjacent nodes.
    STEP_LOCAL_EDGE_2_NODE,
    /**
     * Connect disconnected trees by extending subtree induced by a variable
     * by adding variable to a node.
     */
    STEP_LONG_EXTEND_SUBTREE,
    //! Combine nodes in disconnected trees if union is of size <= k.
    STEP_LONG_COMBINE_NODES,
    /**
     * Connect disconnected subtrees by creating a new node from variables
     * from a node from each subtree.
     */
    STEP_LONG_EDGE_2_NODE,
    //! Move leaf variable to another leaf.
    STEP_LONG_MOVE_VAR,
    //! Move leaf variable to a new leaf.
    STEP_LONG_MOVE_VAR2,
    //! Move leaf to a new leaf.
    STEP_LONG_MOVE_LEAF,
    //! Swap one variable with another.
    STEP_LONG_SWAP_VAR
  };

  //! Parameter estimation methods.
  enum param_method_enum {
    //! local MLE
    PARAM_LOCAL_MLE
  };

  // TODO: It'd be nice to separate iterators over structures from iterators
  //       over models, with the latter using the former.
  // TODO: How should I do this for other models, and should we tie those
  //       iterators together with this one?
  /**
   * Class for iterating over a set of decomposable models.
   * 2 types of search:
   * 1) Search over a fixed set of models (without respect to an initial
   * model).
   * 2) Search over models formed by a given set of steps from a given model.
   */
  template <typename F> class decomposable_iterator :
    public std::iterator<std::forward_iterator_tag,
			 const sill::decomposable<F> >{

    concept_assert((Factor<F>));
    //should be:
    //concept_assert((DistributionFactor<factor_t>));
    //concept_assert((Invertible<factor_t>));
    // (Check decomposable.hpp)

  protected:

    //! Current model.
    decomposable<F> model;

    //! Data used for parameter estimation.
    copy_ptr<data_t> data_ptr;

    //! Parameter estimation method.
    param_method_enum param_method;

    //! Max clique size allowed.
    domain::size_type max_clique_size;

    //! Indicates if the iterator is done iterating through all new models.
    bool done;

    //! Amount by which to smooth marginals.
    double smooth;

    /*--------Stuff for iterating over fixed set of structures.--------*/

    //! Variables over which the model will be created.
    domain vars;

    //! Set of structures over which to iterate, if any.
    std::vector<structure_initial_enum> model_types;

    /**
     * Index in model_types of the current type of model over which this
     * iterator is iterating.
     */
    std::vector<structure_initial_enum>::size_type which_model;

    // TODO: Implement removed_potentials and added_potentials!
    //! Potentials removed from the original model to create the current model.
    std::list<F> removed_potentials;

    //! Potentials added to the original model to create the current model.
    std::list<F> added_potentials;

    /*--------Stuff for steps from initial structure.------------*/

    //! Allowed steps, if any.
    std::vector<structure_step_enum> steps;

    /**
     * Index in steps of the current type of step over which this
     * iterator is iterating.
     */
    std::vector<structure_step_enum>::size_type which_step;

    typedef typename decomposable<F>::jt_t jt_t;
    typedef typename decomposable<F>::vertex_t vertex_t;
    typedef typename decomposable<F>::vertex_iterator vertex_iterator;
    typedef typename decomposable<F>::edge_t edge_t;
    typedef typename decomposable<F>::undir_edge_iterator undir_edge_iterator;
    typedef typename decomposable<F>::out_edge_iterator out_edge_iterator;

    typedef std::vector<boost::tuple<edge_t, F > >
      edge_fctr_vec_t;

    typedef std::vector<boost::tuple<vertex_t, F > >
      vertex_fctr_vec_t;

    /*
     * The following classes hold info for each type of step.
     * These are partly to keep the namespace clean and partly
     * to keep from allocating the space for types of steps we
     * aren't going to allow.
     */

    //! STEP_LOCAL_PUSH_BACK_SUBTREE
    class local_push_back_subtree_info_t {

    public:

      /**
       * True if step_local_push_back_subtree() is inside its loop.
       * If false, then the function skips to its inner loop and continues
       * from there.
       */
      bool inside;

      //! Iterator over vertices v.
      vertex_iterator v_it, v_end; 
      //! Variables in vertex v.
      domain v_args;
      //! Iterator over variables in v_args.
      domain::const_iterator d_it, d_end;
      //! Iterator over edges e out of vertex v.
      out_edge_iterator e_it, e_end;
      //! Edge e = *e_it.
      edge_t e;
      //! Reverse of edge e.
      edge_t e_reverse;
      //! Variables in separator of edge e.
      domain s_args;
      //! Vertex v2 (connected to v by edge e).
      vertex_t v2;
      //! Variables in vertex v2.
      domain v2_args;
      /**
       * Variables in separator of exactly one edge incident to v2 other than
       * edge e.  (Variable *d_it will push each of these variables out of
       * v2 in each of the possible models.)
       */
      domain v2_arg_subset;
      //! Iterator over variables in v2_arg_subset.
      domain::const_iterator d2_it, d2_end;

      //! Initial model: saved marginal for vertex v.
      F orig_c_potential;
      //! Initial model: saved marginals for separators.
      edge_fctr_vec_t orig_s_potentials;
      //! Initial model: saved set of variables in v2.
      domain orig_v2_args; // probably don't need to save

      local_push_back_subtree_info_t() : inside(true) { }
    };

    //! STEP_LOCAL_COMBINE_NODES
    class local_combine_nodes_info_t {

    public:

      /**
       * True if step_local_push_back_subtree() is inside its loop.
       * If false, then the function skips to its inner loop and continues
       * from there.
       */
      bool inside;

      /**
       * Vector of edges collected in post-order traversal.
       * Edges point towards start vertex of traversal.
       */
      std::vector<edge_t> edge_vec;
      //! Iterator over (undirected) edges.
      typename std::vector<edge_t>::const_iterator e_it, e_end;
      //! Vertices on either side of e: source, target.
      vertex_t v1, v2;
      //! Union of cliques v1, v2.
      domain clique_union;

      //! Initial model: saved marginals for vertices v1,v2.
      F orig_v1_potential, orig_v2_potential;
      //! Initial model: saved domains of v1, v2.
      domain orig_v1_args, orig_v2_args;
      //! Initial model: saved neighbors of v1, in set form.
      set<vertex_t> orig_v1_neighbor_set;
      /**
       * Initial model: saved neighbors of v2, with marginals of separators.
       * Includes edge v1-v2.
       */
      vertex_fctr_vec_t orig_v2_neighbors;

      explicit local_combine_nodes_info_t(const jt_t& jt)
	: inside(true) {
	// Choose any start vertex for post-order traversal.
	vertex_iterator it, end;
	boost::tie(it, end) = jt.vertices();
//	boost::tie(it, end) = boost::vertices(jt.graph());
	if (it == end)
	  return;
	post_order_traversal(jt.graph(), *it,
	   make_out_it_edge_visitor(std::back_inserter(edge_vec)));
      }
    };

    //! STEP_LOCAL_EDGE_2_NODE
    class local_edge_2_node_info_t {

    public:

      /**
       * True if step_local_push_back_subtree() is inside its loop.
       * If false, then the function skips to its inner loop and continues
       * from there.
       */
      bool inside;

      /**
       * Vector of edges collected in post-order traversal.
       * Edges point towards start vertex of traversal.
       *
       * TODO: We don't need to store this vector of edges if we keep
       *    using setS or listS for storing adjacency_list info since
       *    operations don't mess up iterators or descriptors unless
       *    the iterators point to the affected edges or vertices.
       *    But I should make sure the documentation is really correct;
       *    for example, even if the iterator is not invalid, might the
       *    order of the items be messed up?
       *   ACTUALLY, I'M NOT SURE THIS IS TRUE--the order may be messed up.
       *   LET'S STORE THE VECTOR FOR NOW TO BE SAFE.
       */
      std::vector<edge_t> edge_vec;
      //! Iterator over (undirected) edges.
      typename std::vector<edge_t>::const_iterator e_it, e_end;
      /**
       * Edge e = *e_it, or a replacement edge if *e_it has been deleted
       * and recreated.
       */
      edge_t e;
      //! Vertices on either side of e: source, target.
      vertex_t v1, v2;
      //! Variables in v1 only, v2 only, and separator.
      domain c1_only, c2_only, s;
      //! Iterators over subsets of c1_only, c2_only.
      subset_iterator<domain> c1_only_it, c1_only_end,
	c2_only_it, c2_only_end;
      //! Marginal over c1 U c2.
      F c1_c2_potential;

      //! Initial model: saved marginal of edge e.
      F orig_e_potential;
      //! Initial model: added vertex (in new model).
      vertex_t new_v;
      //! Variables in new clique.
      domain new_v_args;

      explicit local_edge_2_node_info_t(const jt_t& jt)
	: inside(true) {
	undir_edge_iterator it, end;
	for (boost::tie(it, end) = jt.undir_edges();
	     it != end;
	     ++it) {
	  edge_vec.push_back(*it);
	}
      }
    };

    //! STEP_LONG_MOVE_LEAF
    class long_move_leaf_info_t {

    public:

      /**
       * True if step_long_move_leaf() is inside its loop.
       * If false, then the function skips to its inner loop and continues
       * from there.
       */
      bool inside;

      //! Vector of vertices.
      std::vector<vertex_t> vertex_vec;
      //! Iterator over vertices.
      typename std::vector<vertex_t>::const_iterator v_it, v_end;
      //! Variables only appearing in v1 = *v_it.
      domain c1;
      //! Vector of vertices v2 (minus v1).
      std::vector<vertex_t> vertex2_vec;
      //! Iterator over vertices v2 (which will skip v1).
      typename std::vector<vertex_t>::const_iterator v2_it, v2_end;
      //! Vertex v2 = *v2_it
      vertex_t v2;
      //! Clique of variables in v2.
      domain c2;
      //! Iterator over subsets of c2 of size <= max_clique_size - size(c1)
      subset_iterator<domain> c2_it, c2_end;
      //! New leaf.
      vertex_t new_v;

      //! Initial model: saved potential of v1.
      F orig_v1_potential;
      //! Initial model: saved potential of edge of leaf v1.
      F orig_e_potential;
      //! Initial model: vertex v1 was connected to.
      vertex_t orig_v1_neighbor;

      long_move_leaf_info_t()
	: inside(true)
      { }

    };

    //! STEP_LONG_SWAP_VAR
    class long_swap_var_info_t {

    public:

      /**
       * True if step_long_swap_var() is inside its loop.
       * If false, then the function skips to its inner loop and continues
       * from there.
       */
      bool inside;

      //! Iterator over pairs of variables.
      subset_iterator<domain> var_pair_it, var_pair_end;
      //! Pair of variables to swap.
      variable_h x1, x2;

      //! Initial model: list of vertex/marginal pairs.
      vertex_fctr_vec_t vertex_fctr_vec;
      //! Initial model: list of edge/marginal pairs.
      edge_fctr_vec_t edge_fctr_vec;

      long_swap_var_info_t()
	: inside(true)
      { }

    };

    //! STEP_LOCAL_PUSH_BACK_SUBTREE
    boost::shared_ptr<local_push_back_subtree_info_t>
      local_push_back_subtree_info_ptr;

    //! STEP_LOCAL_COMBINE_NODES
    boost::shared_ptr<local_combine_nodes_info_t> local_combine_nodes_info_ptr;

    //! STEP_LOCAL_EDGE_2_NODE
    boost::shared_ptr<local_edge_2_node_info_t> local_edge_2_node_info_ptr;

    //! STEP_LONG_MOVE_LEAF
    boost::shared_ptr<long_move_leaf_info_t> long_move_leaf_info_ptr;

    //! STEP_LONG_SWAP_VAR
    boost::shared_ptr<long_swap_var_info_t> long_swap_var_info_ptr;

    /*
     * The following classes hold info for each type of model.
     * These are partly to keep the namespace clean and partly
     * to keep from allocating the space for types of models we
     * aren't going to allow.
     */

    //! INIT_MODEL_EMPTY
    class model_empty_info_t { };

    //! INIT_MODEL_STAR
    class model_star_info_t {

    public:

      //! Iterator over subsets of vars of size max_clique_size.
      subset_iterator<domain> clique_it, clique_end;

      model_star_info_t(domain vars,
			domain::size_type max_clique_size) {
	clique_it = subset_iterator<domain>(vars, max_clique_size);
	clique_end = subset_iterator<domain>();
      }

    };

    //! INIT_MODEL_EMPTY
    boost::shared_ptr<model_empty_info_t> model_empty_info_ptr;

    //! INIT_MODEL_STAR
    boost::shared_ptr<model_star_info_t> model_star_info_ptr;

  public:

    //! End iterator constructor.
    decomposable_iterator()
      : done(true) { }

    /**
     * Constructor for iterator over fixed set of structures.
     *
     * @param data            data used for parameter estimation
     * @param vars            variables to be included in model
     * @param model_types     set of structures over which to iterate
     * @param param_method    parameter estimation method
     * @param max_clique_size maximum clique size allowed
     * @param smooth          amount by which to smooth marginals, default 0
     */
    decomposable_iterator(const data_t& data, domain vars,
			  std::vector<structure_initial_enum> model_types,
			  param_method_enum param_method,
			  domain::size_type max_clique_size,
			  double smooth = 0)
      : data_ptr(copy_ptr<data_t>(new data_t(data))), param_method(param_method),
	max_clique_size(max_clique_size), smooth(smooth), vars(vars),
	model_types(model_types) {
      done = false;
      which_model = 0;
      init_model();
    }

    /**
     * Constructor for iterator over structures which may be formed from the
     * given structure using steps of the type specified.
     * This iterator takes a vector of types of steps allowed,
     * and it iterates through them in the order given.  For each type of
     * step allowed, it iterates through the possible models in
     * some order.
     *
     * @param model           initial model
     * @param data            data used for parameter estimation
     * @param steps           set of steps allowed
     * @param param_method    parameter estimation method
     * @param max_clique_size maximum clique size allowed
     * @param smooth          amount by which to smooth marginals, default 0
     */
    decomposable_iterator(const decomposable<F>& model,
			  const data_t& data,
			  std::vector<structure_step_enum> steps,
			  param_method_enum param_method,
			  domain::size_type max_clique_size,
			  double smooth = 0)
      : model(model), data_ptr(copy_ptr<data_t>(new data_t(data))),
	param_method(param_method),
	max_clique_size(max_clique_size), smooth(smooth), steps(steps) {
      done = false;
      which_step = 0;
      init_step();
    }

    // TODO: Check to see if this really works:
    //       I think we'll have to do more work to copy the info_ptr's
    //       correctly since they hold iterators which will be invalidated
    //       if the copied graph has new descriptors.
    //       Or, we could make this iterator non-copy-constructible.
    // TODO: To avoid duplicate code, should we just call the assignment
    //       operator from this copy constructor?  What's standard practice?
    //       Actually, I don't even need all this in the copy constructor b/c
    //       it'll do it by default, right?
    //! Copy constructor.
    decomposable_iterator(const decomposable_iterator& it)
      : model(it.model), data_ptr(it.data_ptr), param_method(it.param_method),
	max_clique_size(it.max_clique_size), done(it.done),
	vars(it.vars), model_types(it.model_types), which_model(it.which_model),
	removed_potentials(it.removed_potentials),
	added_potentials(it.added_potentials), steps(it.steps),
	which_step(it.which_step)
    {
      copy_info_pointers(it);
    }

    // TODO: See copy constructor notes.
    //! Assignment operator.
    decomposable_iterator& operator=(const decomposable_iterator& it) {
      model = it.model;
      data_ptr = it.data_ptr;
      param_method = it.param_method;
      max_clique_size = it.max_clique_size;
      model_types = it.model_types;
      which_model = it.which_model;
      vars = it.vars;
      steps = it.steps;
      which_step = it.which_step;
      copy_info_pointers(it);
      return *this;
    }

    //! Prefix increment.
    decomposable_iterator& operator++() {
      if (done)
	return *this;
      if (steps.size() > 0) {
	bool done_step = false;
	switch(steps[which_step]) {
	case STEP_LOCAL_PUSH_BACK_SUBTREE:
	  done_step = step_local_push_back_subtree();
	  break;
	case STEP_LOCAL_COMBINE_NODES:
	  done_step = step_local_combine_nodes();
	  break;
	case STEP_LOCAL_EDGE_2_NODE:
	  done_step = step_local_edge_2_node();
	  break;
	case STEP_LOCAL_EXTEND_SUBTREE:
	case STEP_LOCAL_RETRACT_SUBTREE:
	case STEP_LOCAL_SPLIT_NODE:
	case STEP_LONG_EXTEND_SUBTREE:
	case STEP_LONG_COMBINE_NODES:
	case STEP_LONG_EDGE_2_NODE:
	case STEP_LONG_MOVE_VAR:
	case STEP_LONG_MOVE_VAR2:
	  std::cerr << "not yet implemented" << std::endl;
	  done_step = true;
	  break;
	case STEP_LONG_SWAP_VAR:
	  done_step = step_long_swap_var();
	  break;
	case STEP_LONG_MOVE_LEAF:
	  done_step = step_long_move_leaf();
	  break;
	}
	if (done_step) {
	  clean_up_step();
	  which_step++;
	  init_step();
	}
      } else {
	bool done_model = false;
	switch(model_types[which_model]) {
	case INIT_MODEL_EMPTY:
	  done_model = next_model_empty();
	  break;
	case INIT_MODEL_STAR:
	  done_model = next_model_star();
	  break;
	}
	if (done_model) {
	  clean_up_model();
	  which_model++;
	  init_model();
	}
      }
      return *this;
    }

    //! Postfix increment.
    decomposable_iterator operator++(int) {
      decomposable_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    //! Returns a const reference to the current model.
    const decomposable<F>& operator*() {
      return model;
    }

//    //! Returns a const pointer to the current model.
//    const decomposable<F>* operator->() { return &model; }

    //! Returns truth if the two model iterators are the same.
    bool operator==(const decomposable_iterator& it) const {
      // TODO: Should I make iterators which aren't done comparable?
      if (done == it.done)
	return true;
      return false;
    }

    //! Returns truth if the two model iterators are different.
    bool operator!=(const decomposable_iterator& it) const {
      return !(*this == it);
    }

    /**
     * Returns potentials which were removed from the original model to
     * create the current model.
     * For decomposable models, this includes added separator marginals,
     * while removed separator marginals are with the added potentials.
     */
    const std::list<F>& get_removed_potentials() const {
      return removed_potentials;
    }

    /**
     * Returns potentials which were added to the original model to
     * create the current model.
     * For decomposable models, this includes removed separator marginals,
     * while added separator marginals are with the removed potentials.
     */
    const std::list<F>& get_added_potentials() const {
      return added_potentials;
    }

  protected:

    /**
     * Initializes iterator over types of models to iterate over models
     * of type model_types[which_model].
     * Returns true if at end of list of model types.
     */
    void init_model() {
      if (which_model >= model_types.size()) {
	model = decomposable<F>();
	done = true;
	return;
      }
      switch(model_types[which_model]) {
      case INIT_MODEL_EMPTY:
	model_empty_info_ptr.reset(new model_empty_info_t());
	break;
      case INIT_MODEL_STAR:
	model_star_info_ptr.reset
	  (new model_star_info_t(vars, max_clique_size));
	break;
      }

      ++(*this);

      if (done)
	model = decomposable<F>();
    }

    //! Initializes iterator for steps of type steps[which_step].
    void init_step() {
      if (which_step >= steps.size()) {
	model = decomposable<F>();
	done = true;
	return;
      }
      switch(steps[which_step]) {
      case STEP_LOCAL_PUSH_BACK_SUBTREE:
	local_push_back_subtree_info_ptr.reset
	  (new local_push_back_subtree_info_t());
	break;
      case STEP_LOCAL_COMBINE_NODES:
	local_combine_nodes_info_ptr.reset
	  (new local_combine_nodes_info_t(*(model.jt_ptr)));
	break;
      case STEP_LOCAL_EDGE_2_NODE:
	local_edge_2_node_info_ptr.reset
	  (new local_edge_2_node_info_t(*(model.jt_ptr)));
	break;
      case STEP_LOCAL_EXTEND_SUBTREE:
      case STEP_LOCAL_RETRACT_SUBTREE:
      case STEP_LOCAL_SPLIT_NODE:
      case STEP_LONG_EXTEND_SUBTREE:
      case STEP_LONG_COMBINE_NODES:
      case STEP_LONG_EDGE_2_NODE:
      case STEP_LONG_MOVE_VAR:
      case STEP_LONG_MOVE_VAR2:
	std::cerr << "not yet implemented" << std::endl;
	break;
      case STEP_LONG_SWAP_VAR:
	long_swap_var_info_ptr.reset(new long_swap_var_info_t());
	break;
      case STEP_LONG_MOVE_LEAF:
	long_move_leaf_info_ptr.reset(new long_move_leaf_info_t());
	break;
      }

      ++(*this);

      if (done)
	model = decomposable<F>();
    }

    /**
     * Clean up iterator after iterating over models of type
     * model_types[which_model].
     */
    void clean_up_model() {
      assert(which_model < model_types.size());
      switch(model_types[which_model]) {
	case INIT_MODEL_EMPTY:
	  model_empty_info_ptr.reset();
	  break;
	case INIT_MODEL_STAR:
	  model_star_info_ptr.reset();
	  break;
      }
    }

    /**
     * Clean up iterator after steps of type steps[which_step].
     */
    void clean_up_step() {
      assert(which_step < steps.size());
      switch(steps[which_step]) {
      case STEP_LOCAL_PUSH_BACK_SUBTREE:
	local_push_back_subtree_info_ptr.reset();
	break;
      case STEP_LOCAL_COMBINE_NODES:
	local_combine_nodes_info_ptr.reset();
	break;
      case STEP_LOCAL_EDGE_2_NODE:
	local_edge_2_node_info_ptr.reset();
	break;
      case STEP_LOCAL_EXTEND_SUBTREE:
      case STEP_LOCAL_RETRACT_SUBTREE:
      case STEP_LOCAL_SPLIT_NODE:
      case STEP_LONG_EXTEND_SUBTREE:
      case STEP_LONG_COMBINE_NODES:
      case STEP_LONG_EDGE_2_NODE:
      case STEP_LONG_MOVE_VAR:
      case STEP_LONG_MOVE_VAR2:
	std::cerr << "not yet implemented" << std::endl;
	break;
      case STEP_LONG_SWAP_VAR:
	long_swap_var_info_ptr.reset();
	break;
      case STEP_LONG_MOVE_LEAF:
	long_move_leaf_info_ptr.reset();
	break;
      }
    }

    /**
     * Copy pointers to info about iterator state.
     * Used by copy constructor and assignment operator.
     */
    void copy_info_pointers(const decomposable_iterator& it) {
      if (it.local_push_back_subtree_info_ptr != NULL)
	local_push_back_subtree_info_ptr
	  = boost::shared_ptr<local_push_back_subtree_info_t>
	  (new local_push_back_subtree_info_t
	   (*(it.local_push_back_subtree_info_ptr)));
      if (it.local_combine_nodes_info_ptr != NULL)
	local_combine_nodes_info_ptr
	  = boost::shared_ptr<local_combine_nodes_info_t>
	  (new local_combine_nodes_info_t
	   (*(it.local_combine_nodes_info_ptr)));
      if (it.local_edge_2_node_info_ptr != NULL)
	local_edge_2_node_info_ptr
	  = boost::shared_ptr<local_edge_2_node_info_t>
	  (new local_edge_2_node_info_t
	   (*(it.local_edge_2_node_info_ptr)));
      if (it.long_move_leaf_info_ptr != NULL)
	long_move_leaf_info_ptr
	  = boost::shared_ptr<long_move_leaf_info_t>
	  (new long_move_leaf_info_t
	   (*(it.long_move_leaf_info_ptr)));
      if (it.long_swap_var_info_ptr != NULL)
	long_swap_var_info_ptr
	  = boost::shared_ptr<long_swap_var_info_t>
	  (new long_swap_var_info_t
	   (*(it.long_swap_var_info_ptr)));

      if (it.model_empty_info_ptr != NULL)
	model_empty_info_ptr
	  = boost::shared_ptr<model_empty_info_t>
	  (new model_empty_info_t
	   (*(it.model_empty_info_ptr)));
      if (it.model_star_info_ptr != NULL)
	model_star_info_ptr
	  = boost::shared_ptr<model_star_info_t>
	  (new model_star_info_t
	   (*(it.model_star_info_ptr)));
    }

    /**
     * STEP_LOCAL_PUSH_BACK_SUBTREE:
     *   Replace variable in one node with variable in adjacent node.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool step_local_push_back_subtree() {

      // TODO: I should change this so it collapses nodes if it creates
      // adjacent ones s.t. one is a subset of the other.

      // -for each vertex *v_it,
      //  -for each variable *d_it in node,
      //   -for each separator s_args with adjacent nodes v2 (v2_args),
      //    -if *d_it is not in s_args
      //     -for each *d2_it in v2_args which is not in s_args but is in
      //      another separator S' of v2,
      //      -replace *d2_it with *d_it in v2

      boost::shared_ptr<local_push_back_subtree_info_t> info
	= local_push_back_subtree_info_ptr;

      // for each vertex *v_it (v_args)
      if (info->inside)
	boost::tie(info->v_it, info->v_end)
	  = model.jt_ptr->vertices();
//	  = boost::vertices(model.jt_ptr->graph());
      while (info->v_it != info->v_end) {
	// for each variable *d_it in clique *v_it
	if (info->inside) {
	  info->v_args = model.jt_ptr->clique(*(info->v_it));
	  boost::tie(info->d_it, info->d_end) = info->v_args.values();
	}
	while (info->d_it != info->d_end) {
	  // for each separator s_args between *v_it and v2_args (v2)
	  if (info->inside) {
	    boost::tie(info->e_it, info->e_end)
	      = model.jt_ptr->out_edges(*(info->v_it));
//	      = boost::out_edges(*(info->v_it), model.jt_ptr->graph());
	  }
	  while (info->e_it != info->e_end) {
	    if (info->inside) {
	      info->e = *(info->e_it);
	      info->e_reverse = reverse_edge(info->e, model.jt_ptr->graph());
	      info->s_args = model.jt_ptr->separator(info->e);
	      info->v2 = boost::target(info->e, model.jt_ptr->graph());
	      info->v2_args = model.jt_ptr->clique(info->v2);
	    }
	    // if *d_it is not in s_args
	    if (!(info->s_args.contains(*(info->d_it)))) {
	      // Collect variables in separators of v2 other than s_args:
	      //  v2_arg_subset2 holds all separator variables.
	      //  v2_arg_subset collects variables which have appeared in
	      //   only one separator.
	      if (info->inside) {
		domain v2_arg_subset2;
		info->v2_arg_subset.clear();
		out_edge_iterator e2_it, e2_end;
		for (boost::tie(e2_it, e2_end)
		       = model.jt_ptr->out_edges(info->v2);
//		       = boost::out_edges(info->v2, model.jt_ptr->graph());
		     e2_it != e2_end; ++e2_it) {
		  edge_t e2 = *e2_it;
		  domain new_vars = model.jt_ptr->separator(e2);
		  info->v2_arg_subset = info->v2_arg_subset.minus(new_vars);
		  new_vars = new_vars.minus(v2_arg_subset2);
		  info->v2_arg_subset = info->v2_arg_subset.plus(new_vars);
		  v2_arg_subset2 = v2_arg_subset2.plus(new_vars);
		}
		info->v2_arg_subset = info->v2_arg_subset.minus(info->s_args);
	      }
	      // for each *d2_it in v2_arg_subset
	      //  (i.e. for each variable in v2_args which is not in s_args
	      //  but is in exactly one other separator S' of v2)
	      if (info->inside)
		boost::tie(info->d2_it, info->d2_end)
		  = info->v2_arg_subset.values();
	      while (info->d2_it != info->d2_end) {
		if (info->inside) {
		  // Make actual change to model:
		  //  Replace *d2_it with *d_it in v2 and
		  //  update its separators.

		  // Save initial model.
		  info->orig_c_potential = model.potential(info->v2);
		  info->orig_v2_args = info->v2_args; // don't need to save
		  info->orig_s_potentials.clear();

		  info->v2_args
		    = info->v2_args.minus(*(info->d2_it)).plus(*(info->d_it));
		  // First, update the clique marginal.
		  model.potential(info->v2)
		    = data_ptr->marginal<F>(info->v2_args, smooth);
		  // QUESTION: Does the list data structure copy the items
		  //   passed to it as const references?
		  // Second, update the separator marginals as needed.
		  out_edge_iterator e2_it, e2_end;
		  for (boost::tie(e2_it, e2_end)
			 = model.jt_ptr->out_edges(info->v2);
//			 = boost::out_edges(info->v2, model.jt_ptr->graph());
		       e2_it != e2_end; ++e2_it) {
		    edge_t e2 = *e2_it;
		    if (e2 == info->e_reverse) {
		      // Add *d_it to separator.
		      info->orig_s_potentials.push_back
			(boost::tie(e2, model.potential(e2)));
		      domain new_sep
			= model.jt_ptr->separator(e2).plus(*(info->d_it));
		      model.potential(e2)
			= data_ptr->marginal<F>(new_sep, smooth);
		    } else {
		      // Possibly remove *d2_it from separator.
		      if (model.jt_ptr->separator(e2).contains
			  (*(info->d2_it))) {
			info->orig_s_potentials.push_back
			  (boost::tie(e2, model.potential(e2)));
			domain new_sep
			  = model.jt_ptr->separator(e2).minus(*(info->d2_it));
			model.potential(e2) =
			  model.potential(e2).marginal(new_sep);
		      }
		    }
		  }
		  // Third, update the structure of the junction tree.
		  model.jt_ptr->set_clique(info->v2, info->v2_args);

		  info->inside = false;

		  return false;
		}

		// Resume loop to increment iterator.
		info->inside = true;

		// Restore initial model.
		model.potential(info->v2) = info->orig_c_potential;
		for (typename edge_fctr_vec_t::const_iterator it =
		       info->orig_s_potentials.begin();
		     it != info->orig_s_potentials.end();
		     ++it) {
		  model.potential((*it).get<0>()) = (*it).get<1>();
		}
		model.jt_ptr->set_clique(info->v2, info->orig_v2_args);

		++(info->d2_it);
	      }
	    }
	    ++(info->e_it);
	  }
	  ++(info->d_it);
	}
	++(info->v_it);
      }

      return true;
    }

    /**
     * STEP_LOCAL_COMBINE_NODES:
     *   Combine adjacent nodes if union is of size <= max_clique_size.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool step_local_combine_nodes() {
      // -for each edge e,
      //  -if union of cliques on either side is of size <= max_clique_size,
      //   -combine cliques

      boost::shared_ptr<local_combine_nodes_info_t> info
	= local_combine_nodes_info_ptr;

      // For each edge,
      if (info->inside) {
	info->e_it = info->edge_vec.begin();
	info->e_end = info->edge_vec.end();
      }
      while (info->e_it != info->e_end) {
	if (info->inside) {
	  edge_t e = *(info->e_it);
	  // Get cliques on either side of edge e = v2-->v1.
	  // We'll be deleting v2, which will be the source vertex of e.
	  info->v1 = boost::target(e, model.jt_ptr->graph());
	  info->v2 = boost::source(e, model.jt_ptr->graph());
	  info->clique_union =
	   model.jt_ptr->clique(info->v1).plus(model.jt_ptr->clique(info->v2));
	}
	if (info->clique_union.size() <= max_clique_size) {
	  if (info->inside) {
	    // Save initial model.
	    info->orig_v1_potential = model.potential(info->v1);
	    info->orig_v2_potential = model.potential(info->v2);
	    info->orig_v1_args = model.jt_ptr->clique(info->v1);
	    info->orig_v2_args = model.jt_ptr->clique(info->v2);
	    info->orig_v1_neighbor_set.clear();
	    info->orig_v2_neighbors.clear();
	    out_edge_iterator e2_it, e2_end;
	    for (boost::tie(e2_it, e2_end)
		   = model.jt_ptr->out_edges(info->v1);
//		   = boost::out_edges(info->v1, model.jt_ptr->graph());
		 e2_it != e2_end; ++e2_it) {
	      vertex_t target = boost::target(*e2_it, model.jt_ptr->graph());
	      if (target == info->v2)
		continue;
	      info->orig_v1_neighbor_set.insert(target);
	    }
	    for (boost::tie(e2_it, e2_end)
		   = model.jt_ptr->out_edges(info->v2);
//		   = boost::out_edges(info->v2, model.jt_ptr->graph());
		 e2_it != e2_end; ++e2_it) {
	      vertex_t target = boost::target(*e2_it, model.jt_ptr->graph());
	      info->orig_v2_neighbors.push_back
		(boost::tie(target, model.potential(*e2_it)));
	    }

	    // Make actual change to model:
	    // Combine v1,v2 into single clique (which will be v1).
	    model.jt_ptr->merge(*(info->e_it));
	    // Update clique marginal.  (Separators remain the same.)
	    //  We do not use decomposable::merge() since we need to update
	    //  the marginal using data.
	    if (info->orig_v1_args.superset_of(info->orig_v2_args))
	      model.potential(info->v1) = info->orig_v1_potential;
	    else if (info->orig_v2_args.superset_of(info->orig_v1_args))
	      model.potential(info->v1) = info->orig_v2_potential;
	    else
	      model.potential(info->v1)
		= data_ptr->marginal<F>(info->clique_union, smooth);

	    info->inside = false;

	    return false;
	  }

	  // Resume loop to increment iterator.
	  info->inside = true;

	  // Restore initial model.
	  // Add vertex v2.
	  info->v2 = model.jt_ptr->add_vertex(info->orig_v2_args);
	  // Remove new edges of v1.
	  // Note: We collect the edges in a vector since iterators over edges
	  //       are messed up when you remove the edges being iterated over.
	  out_edge_iterator e2_it, e2_end;
	  std::vector<edge_t> tmp_edge_vec;
	  for (boost::tie(e2_it, e2_end)
		 = model.jt_ptr->out_edges(info->v1);
//		 = boost::out_edges(info->v1, model.jt_ptr->graph());
	       e2_it != e2_end; ++e2_it) {
	    tmp_edge_vec.push_back(*e2_it);
	  }
	  for (typename std::vector<edge_t>::const_iterator e3_it
		 = tmp_edge_vec.begin();
	       e3_it != tmp_edge_vec.end();
	       ++e3_it) {
	    if (!(info->orig_v1_neighbor_set.contains
		  (boost::target(*e3_it, model.jt_ptr->graph())))) {
	      model.jt_ptr->remove_edge(*e3_it);
	    }
	  }

	  // Add edges and separator marginals of v2.
	  for (typename vertex_fctr_vec_t::const_iterator it
		 = info->orig_v2_neighbors.begin();
	       it != info->orig_v2_neighbors.end();
	       ++it) {
	    edge_t v2_e1, v2_e2;
	    boost::tie(v2_e1, v2_e2)
	      = model.jt_ptr->add_edge(info->v2, (*it).get<0>());
	    model.potential(v2_e1) = (*it).get<1>();
	  }
	  // Update v1, v2 args and marginals.
	  model.jt_ptr->set_clique(info->v1, info->orig_v1_args);
	  model.jt_ptr->set_clique(info->v2, info->orig_v2_args);
	  model.potential(info->v1) = info->orig_v1_potential;
	  model.potential(info->v2) = info->orig_v2_potential;
	}
	++(info->e_it);
      }
      return true;
    }

    /**
     * STEP_LOCAL_EDGE_2_NODE:
     *   Create a new node between two adjacent nodes.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool step_local_edge_2_node() {
      // For each edge between cliques c1, c2
      //  Get c1_only = c1 \ c2, c2_only = c2 \ c1, s = c1 ^ c2
      //  For each non-empty subset c1s \in c1
      //   For each non-empty subset c2s \in c2
      //    Create new node between c1, c2 containing c1s U c2s U s

      boost::shared_ptr<local_edge_2_node_info_t> info
	= local_edge_2_node_info_ptr;
      // For each edge between cliques c1, c2
      if (info->inside) {
	info->e_it = info->edge_vec.begin();
	info->e_end = info->edge_vec.end();
	if (info->e_it == info->e_end)
	  return true;
      }
      do {
	if (info->inside) {
	  info->e = *(info->e_it);
	  ++(info->e_it);
	  // Get cliques on either side of edge e.
	  info->v1 = boost::source(info->e, model.jt_ptr->graph());
	  info->v2 = boost::target(info->e, model.jt_ptr->graph());
	  // Get c1_only = c1 \ c2, c2_only = c2 \ c1, s = c1 ^ c2,
	  //  and compute marginal over c1 U c2.
	  info->c1_only = model.jt_ptr->clique(info->v1);
	  info->c2_only = model.jt_ptr->clique(info->v2);
	  domain tmp_domain(info->c1_only.plus(info->c2_only));
	  info->c1_c2_potential = data_ptr->marginal<F>(tmp_domain, smooth);
	  info->s = model.jt_ptr->separator(info->e);
	  info->c1_only = info->c1_only.minus(info->s);
	  info->c2_only = info->c2_only.minus(info->s);
	  // Get iterator over non-empty subsets of c1
	  info->c1_only_it = subset_iterator<domain>
	    (info->c1_only, 1, info->c1_only.size()-1);
	  info->c1_only_end = subset_iterator<domain>();
	}
	// For each non-empty subset c1s \in c1
	while (info->c1_only_it != info->c1_only_end) {
	  if (info->inside) {
	    // Get iterator over non-empty subsets of c2
	    info->c2_only_it = subset_iterator<domain>
	      (info->c2_only, 1, info->c2_only.size());
	    info->c2_only_end = subset_iterator<domain>();
	  }
	  // For each non-empty subset c2s \in c2
	  while (info->c2_only_it != info->c2_only_end) {
	    // If new clique will be of size <= max_clique_size
	    if (info->inside) {
	      info->new_v_args
		= info->s.plus(*(info->c1_only_it)).plus(*(info->c2_only_it));
	    }
	    if (info->new_v_args.size() <= max_clique_size) {
	      if (info->inside) {
		// Save initial model.
		info->orig_e_potential = model.potential(info->e);

		// Create new node between c1, c2 containing c1s U c2s U s
		// First, delete edge e.
		model.jt_ptr->remove_edge(info->e);
		// Now, add new vertex and update its marginal.
		info->new_v = model.jt_ptr->add_vertex(info->new_v_args);
		model.potential(info->new_v)
		  = info->c1_c2_potential.marginal(info->new_v_args);
		// Create edges v1--new_v and v2--new_v and update marginals.
		edge_t new_e1, new_e2;
		boost::tie(new_e1, new_e2)
		  = model.jt_ptr->add_edge(info->v1, info->new_v);
		model.potential(new_e1) = info->c1_c2_potential.
		  marginal(model.jt_ptr->separator(new_e1));
		boost::tie(new_e1, new_e2)
		  = model.jt_ptr->add_edge(info->v2, info->new_v);
		model.potential(new_e1) = info->c1_c2_potential.
		  marginal(model.jt_ptr->separator(new_e1));

		info->inside = false;

		return false;
	      }

	      // Resume loop to increment iterator.
	      info->inside = true;

	      // Restore initial model.
	      model.jt_ptr->remove_vertex(info->new_v, true);
	      edge_t new_e_undir, new_e_dir;
	      boost::tie(new_e_undir, new_e_dir)
		= model.jt_ptr->add_edge(info->v1, info->v2);
	      model.potential(new_e_undir) = info->orig_e_potential;
	      info->e = new_e_undir;
	    }
	    ++(info->c2_only_it);
	  }
	  ++(info->c1_only_it);
	}
      } while (info->e_it != info->e_end);

      return true;
    }

    /**
     * STEP_LONG_MOVE_LEAF:
     *  Move leaf to a new leaf.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool step_long_move_leaf() {
      // For each leaf v1
      //   For each other clique v2
      //     Set c1 = vars only in v1
      //     Set c2 = vars in v2
      //     Remove v1
      //     For each subset s_new of c2 of size <= max_clique_size - size(c1)
      //       Make new clique v1' = c1 U s_new; connect it to v2

      boost::shared_ptr<long_move_leaf_info_t> info
	= long_move_leaf_info_ptr;
      // For each leaf v1
      if (info->inside) {
	vertex_iterator it, end;
	for (boost::tie(it, end) = model.jt_ptr->vertices();
	     it != end; ++it)
	  info->vertex_vec.push_back(*it);
	if (info->vertex_vec.size() == 0)
	  return true;
	info->v_it = info->vertex_vec.begin();
	info->v_end = info->vertex_vec.end();
      }
      do {
	if (info->inside) {
	  vertex_t v1 = *(info->v_it);
	  ++(info->v_it);
	  // Check to see if v1 is a leaf.
	  out_edge_iterator e_it, e_end;
	  boost::tie(e_it, e_end)
	    = model.jt_ptr->out_edges(v1);
//	    = boost::out_edges(v1, model.jt_ptr->graph());
	  edge_t e = *e_it;
	  ++e_it;
	  if (e_it != e_end)
	    if (info->v_it != info->v_end)
	      continue;
	    else
	      break;
	  // e now has the one edge of leaf v1
	  // Set c1 = vars only in v1
	  info->c1 = model.jt_ptr->clique(v1).
	    minus(model.jt_ptr->separator(e));
	  // Save initial model.
	  info->orig_v1_potential = model.potential(v1);
	  info->orig_e_potential = model.potential(e);
	  info->orig_v1_neighbor = boost::target(e, model.jt_ptr->graph());

	  // Remove v1 and its edge.
	  model.jt_ptr->remove_vertex(v1, true);

	  vertex_iterator it, end;
	  info->vertex2_vec.clear();
	  for (boost::tie(it, end) = model.jt_ptr->vertices();
	       it != end; ++it)
	    if (info->orig_v1_neighbor != *it)
	      info->vertex2_vec.push_back(*it);
	  info->v2_it = info->vertex2_vec.begin();
	  info->v2_end = info->vertex2_vec.end();
	}
	//   For each other clique v2 (!= v1)
	while (info->v2_it != info->v2_end) {
	  if (info->inside) {
	    info->v2 = *(info->v2_it);
	    // Set c2 = vars in v2
	    info->c2 = model.jt_ptr->clique(info->v2);
	    // Create iterator over subsets of c2
	    info->c2_it = subset_iterator<domain>
	      (info->c2, 1,
	       std::min(info->c2.size() - 1,
			max_clique_size - info->c1.size()));
	    info->c2_end = subset_iterator<domain>();
	  }
	  // For each non-empty strict subset s_new of c2
	  //    of size <= max_clique_size - size(c1)
	  while (info->c2_it != info->c2_end) {
	    if (info->inside) {
	      domain s_new = *(info->c2_it);

	      // Make new clique v1' = c1 U s_new; connect it to v2
	      domain new_c = s_new.plus(info->c1);

	      info->new_v = model.jt_ptr->add_vertex(new_c);
	      model.potential(info->new_v)
		= data_ptr->marginal<F>(new_c, smooth);
	      edge_t new_e1, new_e2;
	      boost::tie(new_e1, new_e2)
		= model.jt_ptr->add_edge(info->v2, info->new_v);
	      model.potential(new_e1)
		= model.potential(info->new_v).marginal(s_new);

	      info->inside = false;

	      return false;
	    }

	    // Resume loop to increment iterator.
	    info->inside = true;

	    // Restore initial model (partially) by removing new_v.
	    model.jt_ptr->remove_vertex(info->new_v, true);

	    ++(info->c2_it);
	  }

	  ++(info->v2_it);
	}

	// Restore initial model (completely) by adding v1 back in.
	vertex_t v1 = model.jt_ptr->add_vertex
	  (info->orig_v1_potential.arguments());
	model.potential(v1) = info->orig_v1_potential;
	edge_t new_e1, new_e2;
	boost::tie(new_e1, new_e2)
	  = model.jt_ptr->add_edge(v1, info->orig_v1_neighbor);
	model.potential(new_e1) = info->orig_e_potential;

      } while (info->v_it != info->v_end);
      return true;
    }

    /**
     * STEP_LONG_SWAP_VAR:
     *  Swap one variable with another.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool step_long_swap_var() {
      // For each pair of variables x1,x2
      //   Swap variables, and recompute marginals.

      boost::shared_ptr<long_swap_var_info_t> info
	= long_swap_var_info_ptr;

      // For each pair of variables x1,x2
      if (info->inside)
	info->var_pair_it = subset_iterator<domain>(model.arguments(), 2, 2);
      while (info->var_pair_it != info->var_pair_end) {
	if (info->inside) {
	  domain var_pair(*(info->var_pair_it));
	  domain::const_iterator it = var_pair.begin();
	  info->x1 = *it;
	  ++it;
	  info->x2 = *it;
	  // Make actual change to model:
	  //  Swap variables, and recompute marginals.
	  vertex_iterator v_it, v_end;
	  // Set true if the model has been changed.
	  //  (It is not changed if the 2 variables always appear in the
	  //   same nodes.)
	  bool model_changed = false;
	  for (boost::tie(v_it,v_end) = model.jt_ptr->vertices();
	       v_it != v_end; ++v_it) {
	    vertex_t v = *v_it;
	    domain c = model.jt_ptr->clique(v);
	    if (c.contains(info->x1)) {
	      if (!(c.contains(info->x2))) {
		// Clique c contains x1 but not x2
		c = c.minus(info->x1).plus(info->x2);
		info->vertex_fctr_vec.push_back
		  (boost::tie(v, model.potential(v)));
		model.potential(v) = data_ptr->marginal<F>(c, smooth);
		model.jt_ptr->set_clique(v, c);
		model_changed = true;
	      }
	    } else if (c.contains(info->x2)) {
	      // Clique c contains x2 but not x1
	      c = c.minus(info->x2).plus(info->x1);
	      info->vertex_fctr_vec.push_back
		(boost::tie(v, model.potential(v)));
	      model.potential(v) = data_ptr->marginal<F>(c, smooth);
	      model.jt_ptr->set_clique(v, c);
	      model_changed = true;
	    }
	  }
	  if (!model_changed) {
	    ++(info->var_pair_it);
	    continue;
	  }

	  undir_edge_iterator e_it, e_end;
	  for (boost::tie(e_it,e_end) = model.jt_ptr->undir_edges();
	       e_it != e_end; ++e_it) {
	    edge_t e = *e_it;
	    domain s = model.jt_ptr->separator(e);
	    if (s.contains(info->x1)) {
	      if (!(s.contains(info->x2))) {
		// Separator s contains x1 but not x2
		s = s.minus(info->x1).plus(info->x2);
	      } else
		continue;
	    } else if (s.contains(info->x2)) {
	      // Separator s contains x2 but not x1
	      s = s.minus(info->x2).plus(info->x1);
	    } else
	      continue;
	    info->edge_fctr_vec.push_back
	      (boost::tie(e, model.potential(e)));
	    model.potential(e) = data_ptr->marginal<F>(s, smooth);
	  }

	  info->inside = false;
	  return false;
	}

	// Resume loop to increment iterator.
	info->inside = true;

	// Restore initial model.
	typename vertex_fctr_vec_t::const_iterator orig_v_it
	  = info->vertex_fctr_vec.begin();
	while (orig_v_it != info->vertex_fctr_vec.end()) {
	  vertex_t v = (*orig_v_it).get<0>();
	  model.potential(v) = (*orig_v_it).get<1>();
	  model.jt_ptr->set_clique(v, model.potential(v).arguments());
	  ++orig_v_it;
	}
	typename edge_fctr_vec_t::const_iterator orig_e_it
	  = info->edge_fctr_vec.begin();
	while (orig_e_it != info->edge_fctr_vec.end()) {
	  edge_t e = (*orig_e_it).get<0>();
	  model.potential(e) = (*orig_e_it).get<1>();
	  ++orig_e_it;
	}

	++(info->var_pair_it);
      }
      return true;
    }

    /**
     * INIT_MODEL_EMPTY:
     *  Empty model: single-variable cliques.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool next_model_empty() {
      if (model_empty_info_ptr != NULL) {
	model = decomposable<F>();
	// Create a factor for each variable in vars,
	//  and multiply in to model.
	std::vector<F> factor_vec;
	for (domain::const_iterator it = vars.begin();
	     it != vars.end(); ++it)
	  factor_vec.push_back(data_ptr->marginal<F>(*it, smooth));
	model.multiply_in(factor_vec.begin(), factor_vec.end());
	model_empty_info_ptr.reset();
	return false;
      } else
	return true;
    }

    /**
     * INIT_MODEL_STAR:
     *  Star-shaped junction tree with maximal cliques.
     *  For each set of variables of size max_clique_size,
     *  make a central clique with those variables, and make one clique
     *  for each remaining variable, attached to the central clique
     *  and sharing max_clique_size - 1 variables with it.
     *
     * Run next step of iterator to load the next possible model.
     * If this is initial step, initialize iterator info.
     * Otherwise, skip to inner loop and continue from there.
     * Returns true if at end of set of models.
     */
    bool next_model_star() {
      // For each set c of variables of size max_clique_size
      //   Make a central node v1 from clique c
      //   For each other variable x
      //     Make a node v2 with variable x and max_clique_size - 1
      //      other variables from c
      //     Attach v2 to v1

      boost::shared_ptr<model_star_info_t> info = model_star_info_ptr;
      // For each set c of variables of size max_clique_size
      if (info->clique_it != info->clique_end) {
	// Make a central node v1 from clique c
	domain c = *(info->clique_it);
	std::cout << "---making central clique c = " << c << std::endl;
	model = decomposable<F>();
	std::vector<F> factor_vec;
	factor_vec.push_back(data_ptr->marginal<F>(c, smooth));
	// For each other variable x
	domain other_vars(vars.minus(c));
	domain::const_iterator v_it, v_end;
	// Set c to be one variable smaller, and use it to form
	//  the extra cliques we create.
	// TODO: It'd be better to choose the set more carefully.
	c = c.minus(*(c.begin()));
	for (boost::tie(v_it, v_end) = other_vars.values();
	     v_it != v_end; ++v_it)
	  // Make a factor v2 with variable x and max_clique_size - 1
	  //  other variables from c
	  factor_vec.push_back(data_ptr->marginal<F>(c.plus(*v_it), smooth));
	model.multiply_in(factor_vec);
	++(info->clique_it);
	return false;
      }
      return true;
    }

  }; // class decomposable_iterator<F>

} // namespace sill

#endif // #ifndef SILL_DECOMPOSABLE_ITERATOR_HPP
