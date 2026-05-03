#include "steps.hh"

#include "getopts.hh"   // opt_init_t
#include "measure.hh"   // measure
#include "my_rand.hh"   // my_rand()
#include "print.hh"     // o
#include "read_conf.hh" // conf

#include <sstream>

#include <algorithm>

/// \file
/// Store (in #steps) the steps currently searched. See steps_t and
/// delta_t for details.

// BEGIN option handling for 'max_level'

/// The maximal "level" (number of options to alter at once) in the
/// search.
static unsigned long max_level = 1;

/// Option helper function.
/// \todo Possible to use lambda function?
static void opt_l() { max_level = strtoul(optarg, nullptr, 0); }

/// Option helper varaible.
static int dummy_ =
    (opt_reg_t::append('l', opt_l, "l:", "  [-l max_level]",
                       "  -l max_level \tthe maximal number of options to "
                       "alter at once (default 1)\n"),
     1);

// END

// BEGIN option handling for 'json'

/// Output results as JSON.
static bool opt_json_flag = false;

/// Option helper function.
static void opt_j() { opt_json_flag = true; }

/// Option helper variable.
static int dummy_j_ =
    (opt_reg_t::append('j', opt_j, "j", "  [-j]",
                       "  -j \t\toutput results as JSON to stdout\n"),
     1);

// END

/// Quick mode: cap on the number of combinations sampled per level.
/// 0 means "no cap" (full search). Set via -Q.
static unsigned long quick_cap = 0;

/// Option helper function.
static void opt_Q() { quick_cap = strtoul(optarg, nullptr, 0); }

/// Option helper variable.
static int dummy_Q_ =
    (opt_reg_t::append('Q', opt_Q, "Q:", "  [-Q n]",
                       "  -Q n \t\tquick mode: at each level, stop after "
                       "sampling\n  \t\tat most 'n' combinations "
                       "(0 = no cap, default)\n"),
     1);

// END

#ifdef DEBUG
std::string delta_ind_str(delta_ind_t const &d_ind) {
  std::ostringstream ss;
  ss << "delta: ";
  for (unsigned short i : d_ind) {
    ss << i << " (" << flag::flag_size(conf.flags[i]) << ")";
  }
  return ss.str();
}
#endif

delta_t::delta_t(const point_t &p_, const point_t &p_p, bool e, obj_t d)
    : p(p_), p_prev(p_p), equal(e), diff(d)
{
  contract_assert(p.val.size() == p_prev.val.size());
}

bool delta_t::operator<(delta_t const &delta) const {
  // only use diff, but with a twist
  return alt_diff() < delta.alt_diff();
}

std::string delta_t::str() const {
  std::ostringstream ss;
  if (equal) {
    ss << "= ";
  } else {
    ss << diff.to_string() << " ";
  }
  for (size_t i = 0; i < p.val.size(); ++i) {
    unsigned const num = p.val[i];
    unsigned const num_prev = p_prev.val[i];
    if (num != num_prev) {
      if (num != 0) {
        ss << flag::get_flag(conf.flags[i], num);
      }
      if (num_prev != 0) {
        ss << "!" << flag::get_flag(conf.flags[i], num_prev);
      }
    }
  }
  return ss.str();
}

obj_t delta_t::alt_diff() const {
  bool backwards = false; // true if any option has been removed
  for (size_t i = 0; i < p.val.size(); ++i) {
    backwards = backwards || (p.val[i] == 0 && p_prev.val[i] != 0);
  }
  // Negation is only defined for finite diffs; if the diff is the "worst"
  // sentinel (failed compile/run), leave it as-is — such a delta should
  // always sort as worst regardless of direction.
  return (backwards && diff.is_finite()) ? -diff : diff;
}

/// New combinations compared to the previous level.
/// \todo Document better.
static unsigned new_comb(unsigned level) {
  unsigned res = 1;
  for (unsigned j = 0; j < level; ++j) {
    res *= conf.flags.size() - j;
    res /= j + 1;
  }
  return res;
}

steps_t::steps_t() noexcept = default;

delta_ind_t steps_t::get_next(const point_t &p)
{
  contract_assert(dispatched <= number_of_comb);
  if (dispatched == number_of_comb) {
    // search space exausted
    if (level > 0) {
      o1 << "\n### Search at level " << level << " completed. Result:";
      o1 << "\n" << measure(p).to_string() << " " << p.to_string();
      print();
    }
    ++level;
    seq_index = 0;
    if (level > max_level) {
      delta_info = delta_info_t::finish;
      return {};
    }
    unsigned const this_level_comb = new_comb(level);
    unsigned const capped = (quick_cap != 0 && this_level_comb > quick_cap)
                                ? static_cast<unsigned>(quick_cap)
                                : this_level_comb;
    number_of_comb += capped;
    o1 << "\n# restarting, options combined " << level
       << " "
          "(number of combinations "
       << number_of_comb;
    if (capped != this_level_comb) {
      o1 << ", capped from " << this_level_comb << " by -Q";
    }
    o1 << ")\n";
  }
  delta_info = delta_info_t::valid;
  ++dispatched;
  auto result = get_next_delta();
  ++seq_index;
  return result;
}

void steps_t::store(const delta_t &delta) {
#ifdef DEBUG
  // delta_print(o1, delta);
#endif
  done.push_back(delta);
  std::ranges::sort(done, std::less{});
}

void steps_t::print() const {
  o1 << "\ntotal number of samples: " << progress.cnts['*'];
  o1 << "\nexplored alterations:\n";
  for (auto i = done.cbegin(); i != done.cend(); ++i) {
    if (i != done.cbegin()) {
      o1 << ", ";
    }
    o1 << i->str();
  }
}

void steps_t::summary_exit() const {
  static const unsigned top_list_size = 7;
  o3 << "\nBest options:";
  for (auto const &i : done) {
    if (!(&i - &*done.cbegin() < top_list_size)) {
      break;
    }
    if (i.alt_diff() < obj_t(0)) {
      o3 << "\n+" << i.str();
    }
  }

  o3 << "\n"
     << "\nWorst options:";
  for (auto i = done.crbegin(); i != done.crend(); ++i) {
    if (!(i - done.crbegin() < top_list_size)) {
      break;
    }
    if (i->alt_diff() > obj_t(0)) {
      o3 << "\n+" << i->str();
    }
  }

  o3 << "\n\n";
}

void steps_t::json_exit() const {
  if (!opt_json_flag) return;
  o3 << "[";
  bool first = true;
  for (auto const &i : done) {
    if (!first) o3 << ",";
    first = false;
    auto diff = i.alt_diff();
    o3 << "\n  {\"diff\":";
    if (diff.is_finite())
      o3 << diff.to_string();
    else
      o3 << "null";
    o3 << ",\"equal\":" << (i.equal ? "true" : "false")
       << ",\"flags\":\"" << i.str() << "\"}";
  }
  o3 << "\n]\n";
}

bool steps_t::find_d_ind(const delta_ind_t &d_ind) const {
  for (auto const &i : done) {

    delta_ind_t d_ind_p;
    // construct d_ind_p
    for (size_t j = 0; j < i.p.val.size(); ++j) {
      if (i.p.val[j] != i.p_prev.val[j]) {
        d_ind_p.insert(j);
      }
    }

    if (d_ind == d_ind_p) {
      return true;
    }
  }
  return false;
}

delta_ind_t steps_t::get_next_delta() const
{
  // Sequential exploration in config-file order.
  // seq_index tracks position within the current level.
  unsigned const n = static_cast<unsigned>(conf.flags.size());
  delta_ind_t d_ind;

  if (level == 1) {
    d_ind.insert(static_cast<unsigned short>(seq_index % n));
  } else if (level == 2) {
    // Map seq_index to combination (i, j) where i < j.
    unsigned idx = 0;
    for (unsigned i = 0; i < n; ++i) {
      for (unsigned j = i + 1; j < n; ++j) {
        if (idx == seq_index) {
          d_ind.insert(static_cast<unsigned short>(i));
          d_ind.insert(static_cast<unsigned short>(j));
          return d_ind;
        }
        ++idx;
      }
    }
  } else {
    // For level >= 3, fall back to random.
    do {
      d_ind.clear();
      for (unsigned i = 0; i < level; ++i) {
        d_ind.insert(static_cast<unsigned>(my_rand()) %
                     static_cast<unsigned char>(n));
      }
      contract_assert(d_ind.size() <= level);
    } while (d_ind.size() < level || find_d_ind(d_ind));
  }
  return d_ind;
}

/// Data for all steps, stored in a vector of delta_t.
steps_t steps;
