#ifndef SILL_UNDIRECTED_EDGE_HPP
#define SILL_UNDIRECTED_EDGE_HPP

#include <boost/algorithm/minmax.hpp>
#include <boost/functional/hash.hpp>

#include <sill/global.hpp>

namespace sill {

  template<typename V, typename VP, typename EP> 
  class undirected_graph;

  /**
   * A class that represents an undirected edge.  
   * \ingroup graph_types
   */
  template <typename Vertex>
  class undirected_edge {
  private:
    //! Vertex from which the edge originates
    Vertex m_source;

    //! Vertex to which the edge emenates
    Vertex m_target;

    /**
     * The property associated with this edge.  Edges maintain a private
     * pointer to the associated property.  However, this pointer can only
     * be accessed through the associated graph. This permits graphs to
     * return iterators over edges and permits constant time lookup for
     * the corresponding edge properties. The property is stored as a void*,
     * to simplify the type of the edges.
     */
    void* m_property;

    /**
     * undirected_graphs are made friends here to have access to the internal
     * edge property.  
     */
    template <typename V, typename VP, typename EP>
    friend class undirected_graph;

  public:
    //! Default constructor, initializes to the null edge
    undirected_edge() : m_source(), m_target(), m_property() { }

    //! Constructor which permits setting the edge_property
    undirected_edge(const Vertex& source, 
                    const Vertex& target, 
                    void* edge_property = NULL)
      : m_source(source), m_target(target), m_property(edge_property) { }

    bool operator<(const undirected_edge& o) const {
      return boost::minmax(m_source, m_target) < 
        boost::minmax(o.m_source, o.m_target);
    }

    bool operator==(const undirected_edge& o) const {
      return boost::minmax(m_source, m_target) == 
        boost::minmax(o.m_source, o.m_target);
    }

    bool operator!=(const undirected_edge& other) const {
      return !operator==(other);
    }

    const Vertex& source() const {
      return m_source;
    }

    const Vertex& target() const {
      return m_target;
    }
  }; // class directed_edge

  //! \relates undirected_edge
  template <typename Vertex>
  std::ostream& operator<<(std::ostream& out, const undirected_edge<Vertex>& e){
    out << e.source() << " -- " << e.target();
    return out;
  }

  //! \relates undirected_edge
  template <typename Vertex>
  inline size_t hash_value(const undirected_edge<Vertex>& e) {
    return boost::hash_value(boost::minmax(e.source(), e.target()));
  }

} // namespace sill

#endif
