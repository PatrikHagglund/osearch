#ifndef STEPS_HH
#define STEPS_HH

#include "point.hh" // point_t
#include "obj.hh" // obj_t

#include <map>
#include <set>
#include <utility>

/// \file
/// Store (in #steps) the steps currently searched. See steps_t and
/// delta_t for details.

/// List of option indices altered (from current point).
using delta_ind_t = std::set<unsigned short>;

#ifdef DEBUG
std::string delta_ind_str(delta_ind_t const &d_ind);
#endif

/// Data for a single step.
struct delta_t {
  /// Current point.
  point_t p;
  /// Previous point.
  point_t p_prev;
  /// Tell if executable files are equal.
  bool equal{};
  /// If unequal, tell the difference.
  obj_t diff{};
  explicit delta_t(const point_t& p_, const point_t& p_p, bool e, obj_t d);
  explicit delta_t() = default;
  /// Less operator, using alt_diff().
  bool operator<(delta_t const& delta) const;
  /// Provide a string ready for prinout of the class data.
  std::string str() const;
  /// Alternative to the delta_t::diff field. Revert the sign if any
  /// option has been removed.
  obj_t alt_diff() const;
};

/// Information to tell if level has reached max_level (finish).
/// \todo Just use a 'valid' bool in delta_ind_t?
enum delta_info_t {
  valid,
  finish
};

/// Data for all steps, stored in a vector of delta_t.
struct steps_t {
private:
  /// Vector of all delta_t that has been executed.
  using done_t = std::vector<delta_t>;
public:
  /// Vector of all delta_t that has been executed.
  done_t done;
  /// Information to tell if level has reached max_level (finish).
  delta_info_t delta_info{};
  explicit steps_t() noexcept;
  /// Get new step data (a random delta). Check that max_level hasn't
  /// been reached.
  /// \param p point_t
  /// \return delta_ind_t (also steps_t::delta_info to signal that
  /// max_level has been reached)
  delta_ind_t get_next(const point_t& p);
  /// Store the delta in the #done field.
  /// \param delta delta_t
  void store(const delta_t& delta);
  /// Print all steps in #done.
  void print() const;
  /// Print out the best and worst options found.
  void summary_exit() const;
private:
  unsigned level{0};
  unsigned number_of_comb{0};
  bool find_d_ind(const delta_ind_t& d_ind) const;
  delta_ind_t get_rand_delta() const;
};

extern steps_t steps;

#endif
