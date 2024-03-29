#ifndef SILL_DOMAIN_HPP
#define SILL_DOMAIN_HPP

#include <set>
#include <map>
#include <vector>

#include <boost/range/algorithm/set_algorithm.hpp>

#include <sill/iterator/counting_output_iterator.hpp>
#include <sill/serialization/set.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  template <typename V>
  class domain : public std::set<V> {
  public:
    domain() { }

    explicit domain(const V& var) {
      this->insert(var);
    }
    
    // can also take variable_vector (which is a child of std::vector)
    explicit domain(const std::vector<V>& vars) {
      this->insert(vars.begin(), vars.end());
    }
    
    bool includes(const domain& other) const {
      return boost::includes(*this, other);
    }

    void partition(const domain& other, domain& a, domain& b) const {
      boost::set_intersection(*this, other, std::inserter(a, a.begin()));
      boost::set_difference(*this, other, std::inserted(b, b.begin()));
    }

    /**
     * Substitutes variables in a domain.
     *
     * @param  map
     *         a map from (some of the) variables in vars to a new set
     *         of variables; this mapping must be 1:1, and each variable
     *         in vars must map to a type-compatible variable; any
     *         missing variable is assumed to map to itself
     *         can also be a variable_map (which is a child of std::map)
     * @return the image of *this under subst
     */
    domain subst_vars(const std::map<V,V>& map) {
      domain result;
      foreach(V var, *this) {
        typename std::map<V,V>::const_iterator it = map.find(var);
        if (it != map.end()) {
          V new_var = it->second;
          if (!var->type_compatible(new_var)) {
            std::cerr << "Variables "
                      << var << "," << new_var
                      << " are not type-compatible." << std::endl;
            assert(false);
          }
          assert(result.count(new_var) == 0);
          result.insert(new_var);
        } else {
          assert(result.count(var) == 0);
          result.insert(var);
        }
      }
      return result;
    }
  };

  std::ostream& operator<<(std::ostream& out, const domain<V>& dom) {
    print_range(out, dom, '{', ',', '}');
    return out;
  }

  template <typename V>
  domain<V> set_union(const domain<V>& a, const domain<V>& b) {
    domain<V> result;
    boost::set_union(a, b, std::inserter(result, result.begin()));
    return result;
  }

  template <typename V>
  domain<V> set_union(const domain<V>& a, const V& b) {
    domain<V> result = a;
    result.insert(a);
    return result;
  }

  template <typename V>
  domain<V> intersection(const domain<V>& a, const domain<V>& b) {
    domain<V> result;
    boost::set_intersection(a, b, std::inserter(result, result.begin()));
    return result;
  }
  
  template <typename V>
  domain<V> difference(const domain<V>& a, const domain<V>& b) {
    domain<V> result;
    boost::set_difference(a, b, std::inserter(result, result.begin()));
    return result;
  }
  
  template <typename V>
  bool disjoint(const domain<V>& a, const domain<V>& b) {
    counting_output_iterator counter;
    return boost::set_intersection(a, b, counter).count() == 0;
  }

} // namespace sill

#include <macros_undef.hpp>

#endif
