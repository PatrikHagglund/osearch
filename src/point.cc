#include "point.hh"

#include "read_conf.hh" // conf

#include <algorithm>

/// \file
/// For point_t (measurement point).

point_t::point_t() : val(conf.flags.size(), 0U) {}

std::string point_t::to_string() const {
  std::string flags_str;
  for (auto const &i : val) {
    if (i != 0U) {
      flags_str.append(flag::get_flag(conf.flags[&i - &*val.cbegin()], i));
      flags_str.append(" ");
    }
  }
  return flags_str;
}
