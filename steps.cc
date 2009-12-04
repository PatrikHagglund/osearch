#include "steps.hh"

#include "print.hh" // o
#include "read_conf.hh" // conf
#include "measure.hh" // measure
#include "getopts.hh" // opt_init_t

#include <sstream>
using std::ostringstream;
#ifdef DEBUG
using std::endl;
#endif

#include <algorithm>
using std::sort;

// BEGIN option handling for 'max_level'

static unsigned max_level = 1;

static void opt_l() {
  max_level = strtoul(optarg, NULL, 0);
}

static opt_reg_t
opt_reg('l', opt_l, "l:", "  [-l max_level]",
	 "  -l max_level \tthe maximal number of options to alter at once (default 1)\n");

// END

#ifdef DEBUG
string delta_ind_str(delta_ind_t d_ind) {
  ostringstream ss;
  ss << "delta: ";
  for (delta_ind_t::const_iterator i = d_ind.begin(); i != d_ind.end(); ++i) {
    ss << *i << " (" << conf.flags[*i]->size() << ")";
  }
  return ss.str();
}
#endif

delta_t::delta_t(point_t p, point_t p_prev, bool equal, obj_t diff):
  p(p), p_prev(p_prev), equal(equal), diff(diff) {
  assert(p.val.size() == p_prev.val.size());
}

delta_t::delta_t(): p(), p_prev(), equal(), diff() {}

bool delta_t::operator<(delta_t const& delta) const {
  // only use diff, but with a twist
  return alt_diff() < delta.alt_diff();
}

string delta_t::str() const {
  ostringstream ss;
  if (equal)
    ss << "= ";
  else
    ss << diff.str() << " ";
  for (unsigned i = 0; i < p.val.size(); ++i) {
    unsigned num = p.val[i];
    unsigned num_prev = p_prev.val[i];
    if (num != num_prev) {
      if (num != 0)
	ss << conf.flags[i]->get_flag(num);
      if (num_prev != 0)
	ss << "!" << conf.flags[i]->get_flag(num_prev);
    }
  }
  return ss.str();
}

// revert the sign if any option has been removed
obj_t delta_t::alt_diff() const {
  bool backwards = false; // true if any option has been removed
  for (unsigned i = 0; i < p.val.size(); ++i)
    backwards = backwards || (p.val[i] == 0 && p_prev.val[i] != 0);
  return backwards ? obj_t(0)-diff : diff;
}

static unsigned top_list_size = 7;

// new combinations compared to the previous level
static unsigned new_comb(unsigned level) {
  unsigned res = 1;
  for (unsigned j = 0; j < level; ++j) {
    res *= conf.flags.size() - j;
    res /= j+1;
  }
  return res;
}


steps_t::steps_t(): done(), delta_info(), level(0), number_of_comb(0) {}

delta_ind_t steps_t::get_next(point_t p) {
  assert(done.size() <= number_of_comb);
  if (done.size() == number_of_comb) {
    // search space exausted
    if (level > 0) {
      fprintf(o1, "\n### Search at level %u completed. Result:", level);
      fprintf(o1, "\n%s %s", measure(p).str().c_str(), p.str().c_str());
      print();
    }
    ++level;
    if (level > max_level) {
      delta_info = finish;
      return delta_ind_t();
    }
    number_of_comb += new_comb(level);
    fprintf(o1, "\n# restarting, options combined %u "
	    "(number of combinations %u)\n",
	    level, number_of_comb);
  }
  delta_info = valid;
  return get_rand_delta();
}

void steps_t::store(delta_t delta) {
#ifdef DEBUG
  //delta_print(o1, delta);
#endif
  done.push_back(delta);
  sort(done.begin(), done.end());
}

void steps_t::print() const {
  fprintf(o1, "\ntotal number of samples: %u", progress.cnts['*']);
  fprintf(o1, "\nexplored alterations:\n");
  for (done_t::const_iterator i = done.begin(); i != done.end(); ++i) {
    if (i != done.begin())
      fprintf(o1, ", ");
    fprintf(o1, "%s", i->str().c_str());
  }
}

void steps_t::summary_exit() const {
  fprintf(o3, "\nBest options:");
  for (done_t::const_iterator i = done.begin();
       i - done.begin() < top_list_size && i != done.end();
       ++i)
    if (i->alt_diff() < obj_t(0)) {
      fprintf(o3, "\n+");
      fprintf(o3, "%s", i->str().c_str());
    }

  fprintf(o3, "\n");
  fprintf(o3, "\nWorst options:");
  for (done_t::const_reverse_iterator i = done.rbegin();
       i - done.rbegin()  < top_list_size && i != done.rend();
       ++i)
    if (i->alt_diff() > obj_t(0)) {
      fprintf(o3, "\n+");
      fprintf(o3, "%s", i->str().c_str());
    }

  fprintf(o3, "\n\n");
}

bool steps_t::find_d_ind(delta_ind_t d_ind) const {
  for (done_t::const_iterator i = done.begin(); i != done.end(); ++i) {

    delta_ind_t d_ind_p;
    // construct d_ind_p
    for (unsigned j = 0; j < i->p.val.size(); ++j)
      if (i->p.val[j] != i->p_prev.val[j])
	d_ind_p.insert(j);

    if (d_ind == d_ind_p)
      return true;
  }
  return false;
}

delta_ind_t steps_t::get_rand_delta() const {
  delta_ind_t d_ind;
  do {
    d_ind.clear();
    for (unsigned i = 0; i < level; ++i)
      d_ind.insert( unsigned(rand()) % conf.flags.size() );
    assert(d_ind.size() <= level);
  } while (d_ind.size() < level || find_d_ind(d_ind));
  assert(d_ind.size() == level);
  return d_ind;
}

steps_t steps;
