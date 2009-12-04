#ifndef STEPS_HH
#define STEPS_HH

#include "point.hh"

#include <map>
using std::map;
#include <set>
using std::set;
#include <utility>
using std::pair;

// steps currently searched

// list of option indices altered (from current point)
typedef set<unsigned short> delta_ind_t;

#ifdef DEBUG
string delta_ind_str(delta_ind_t delta_ind);
#endif

// data for a single step
struct delta_t {
  point_t p; // current point
  point_t p_prev; // previous point
  bool equal; // tell if executable files are equal
  obj_t diff; // if unequal, tell the difference
  explicit delta_t(point_t p, point_t p_prev, bool equal, obj_t diff);
  explicit delta_t();
  bool operator<(delta_t const& delta) const;
  string str() const;
  obj_t alt_diff() const;
};

enum delta_info_t {
  valid,
  finish
};

struct steps_t {
private:
  typedef vector<delta_t> done_t;
public:
  done_t done;
  delta_info_t delta_info;
  explicit steps_t();
  delta_ind_t get_next(point_t x);
  void store(delta_t delta);
  void print() const;
  void summary_exit() const;
private:
  unsigned level;
  unsigned number_of_comb;
  bool find_d_ind(delta_ind_t d_ind) const;
  delta_ind_t get_rand_delta() const;
};

extern steps_t steps;

#endif
