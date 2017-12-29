#ifndef STEPS_HH
#define STEPS_HH

#include "point.hh"

#include <map>
#include <set>
#include <utility>

// steps currently searched

// list of option indices altered (from current point)
using delta_ind_t = std::set<unsigned short>;

#ifdef DEBUG
std::string delta_ind_str(delta_ind_t const &d_ind);
#endif

// data for a single step
struct delta_t {
  point_t p; // current point
  point_t p_prev; // previous point
  bool equal{}; // tell if executable files are equal
  obj_t diff{}; // if unequal, tell the difference
  explicit delta_t(point_t p_, point_t p_p, bool e, obj_t d);
  explicit delta_t() = default;
  bool operator<(delta_t const& delta) const;
  std::string str() const;
  obj_t alt_diff() const;
};

enum delta_info_t {
  valid,
  finish
};

struct steps_t {
private:
  using done_t = std::vector<delta_t>;
public:
  done_t done;
  delta_info_t delta_info{};
  explicit steps_t() noexcept;
  delta_ind_t get_next(const point_t& p);
  void store(const delta_t& delta);
  void print() const;
  void summary_exit() const;
private:
  unsigned level{0};
  unsigned number_of_comb{0};
  bool find_d_ind(const delta_ind_t& d_ind) const;
  delta_ind_t get_rand_delta() const;
};

extern steps_t steps;

#endif
