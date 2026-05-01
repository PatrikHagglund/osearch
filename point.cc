#include "point.hh"
#include "assume.hh"

#include "read_conf.hh" // conf

#include <algorithm>
#include <iostream> // std::cerr

#if 0
#include <cstddef> // ptrdiff_t
#endif

/// \file
/// For point_t (measurement point).

point_t::point_t() : val(conf.flags.size(), 0U) {}

#if 0
point_t::point_t(std::string flags_str) {
  flags_str.append(" ");
  for (auto const &i : conf.flags) {
    for (size_t j = 1; j < i->size(); ++j) {
      std::string flag = i->get_flag(j);
      flag.append(" ");
      std::string::size_type loc = flags_str.find(flag);
      if (loc != std::string::npos) {
        val[&i - &*conf.flags.cbegin()] = j;
        flags_str.erase(flags_str.cbegin() + ptrdiff_t(loc),
                        flags_str.cbegin() + ptrdiff_t(loc) +
                            ptrdiff_t(flag.length()));
      }
    }
  }
  if (flags_str.find_first_not_of(' ') != std::string::npos) {
    std::cerr
        << "\nError in configuration file. The following flags have not been "
           "found in the flags section: "
        << flags_str << "\n";
    exit(1);
  }
}
#endif

unsigned point_t::popcnt() const {
  return static_cast<unsigned>(
      std::ranges::count_if(val, [](uint8_t v) { return v != 0; }));
}

std::string point_t::to_string() const {
  std::string flags_str;
  for (auto const &i : val) {
    if (i != 0U) {
      flags_str.append(conf.flags[&i - &*val.cbegin()]->get_flag(i));
      flags_str.append(" ");
    }
  }
  return flags_str;
}

#if 0
void point_t::set_rand() {

  for (auto &i : val) {
    i = static_cast<unsigned char>(my_rand() % 2 == 0);
  }
}
#endif
