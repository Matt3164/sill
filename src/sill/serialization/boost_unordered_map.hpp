#ifndef SILL_SERIALIZE_BOOST_UNORDERED_MAP_HPP
#define SILL_SERIALIZE_BOOST_UNORDERED_MAP_HPP

#include <boost/unordered_map.hpp>

#include <sill/serialization/iarchive.hpp>
#include <sill/serialization/oarchive.hpp>
#include <sill/serialization/range.hpp>

namespace sill {

  //! Serializes a map. \relates oarchive
  template <typename T, typename U>
  oarchive& operator<<(oarchive& a, const boost::unordered_map<T,U>& map) {
    serialize_range(a, map.begin(), map.end(), map.size());
    return a;
  }

  //! Deserializes a map. \relates iarchive
  template <typename T, typename U>
  iarchive& operator>>(iarchive& a, boost::unordered_map<T,U>& map) {
    map.clear();
    deserialize_range<std::pair<T,U> >(a, std::inserter(map, map.end()));
    return a;
  }

} // namespace sill

#endif // SILL_SERIALIZE_BOOST_UNORDERED_MAP_HPP
