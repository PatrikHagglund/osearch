#ifndef FLAT_MAP_HH
#define FLAT_MAP_HH

#include <algorithm>
#include <utility>
#include <vector>

/// \file
/// A sorted-vector-based associative container (flat map).

template<typename Key, typename Value>
struct flat_map {
  using pair_t = std::pair<Key, Value>;
  using vec_t = std::vector<pair_t>;

  [[nodiscard]] constexpr bool contains(const Key &k) const {
    auto it = lower(k);
    return it != data_.end() && it->first == k;
  }

  constexpr Value &operator[](const Key &k) {
    auto it = lower(k);
    if (it == data_.end() || it->first != k) {
      it = data_.insert(it, {k, Value{}});
    }
    return it->second;
  }

  [[nodiscard]] constexpr bool empty() const { return data_.empty(); }
  [[nodiscard]] constexpr auto size() const { return data_.size(); }
  [[nodiscard]] constexpr auto cbegin() const { return data_.cbegin(); }
  [[nodiscard]] constexpr auto cend() const { return data_.cend(); }
  [[nodiscard]] constexpr auto begin() { return data_.begin(); }
  [[nodiscard]] constexpr auto end() { return data_.end(); }

  constexpr auto insert(pair_t p) {
    auto it = lower(p.first);
    if (it == data_.end() || it->first != p.first) {
      it = data_.insert(it, std::move(p));
    }
    return it;
  }

  constexpr auto find(const Key &k) {
    auto it = lower(k);
    return (it != data_.end() && it->first == k) ? it : data_.end();
  }

  [[nodiscard]] constexpr auto find(const Key &k) const {
    auto it = lower(k);
    return (it != data_.end() && it->first == k) ? it : data_.cend();
  }

private:
  vec_t data_;

  constexpr auto lower(const Key &k) {
    return std::ranges::lower_bound(data_, k, {}, &pair_t::first);
  }
  [[nodiscard]] constexpr auto lower(const Key &k) const {
    return std::ranges::lower_bound(data_, k, {}, &pair_t::first);
  }
};

#endif
