#include "point.hh"
#include "assume.hh"

#include "read_conf.hh" // conf

#include <iostream> // std::cerr

#include <random> // default_random_engine, uniform_int_distribution

#include <cstddef>
#include <cstdlib> // rand, strtoul

// point_t

point_t::point_t() : val(conf.flags.size(), 0U) {}

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

bool point_t::operator==(point_t const &p) const { return val == p.val; }

bool point_t::operator!=(point_t const &p) const { return val != p.val; }

bool point_t::operator<(point_t const &p) const {
  GNUC_BUILTIN_ASSUME(val.size() == p.val.size());

  // if the number of options turned on (size) are unequal, use the
  // size
  unsigned size1 = 0;
  for (auto const &i : val) {
    if (i != 0) {
      ++size1;
    }
  }
  unsigned size2 = 0;
  for (auto const &i : p.val) {
    if (i != 0) {
      ++size2;
    }
  }
  if (size1 != size2) {
    return size1 < size2;
  }

  // otherwise use lexicographical order
  for (size_t i = 0; i < val.size(); ++i) {
    unsigned t1 = val[i];
    unsigned t2 = p.val[i];
    if (t1 != t2) {
      return t1 < t2;
    }
  }
  return false;
}

std::string point_t::str() const {
  std::string flags_str;
  for (auto const &i : val) {
    if (i != 0U) {
      flags_str.append(conf.flags[&i - &*val.cbegin()]->get_flag(i));
      flags_str.append(" ");
    }
  }
  return flags_str;
}

constexpr int rand_max = 32767;
// Seed with a real random value, if available
// std::random_device r;
// std::default_random_engine e(r());
// NOLINTNEXTLINE(cert-msc51-cpp)
static std::default_random_engine e(0);
static std::uniform_int_distribution<int> uniform_dist(0, rand_max);

int my_rand() { return uniform_dist(e); }

void point_t::set_rand() {

  for (auto &i : val) {
    i = static_cast<unsigned char>(my_rand() % 2 == 0);
  }
}
