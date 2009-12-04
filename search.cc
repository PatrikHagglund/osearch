#include "search.hh"

#include "print.hh"
#include "read_conf.hh"
#include "steps.hh"
#include "measure.hh"
#include "compile.hh"
#include "point.hh"

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <utility>
using std::make_pair;

#ifdef DEBUG
#include <cassert>
#endif

static bool skip(point_t p, point_t p_old, delta_ind_t d_ind) {
  bool same = false;
  for (delta_ind_t::const_iterator i = d_ind.begin(); i != d_ind.end(); ++i)
    same = same || p.val[*i] == p_old.val[*i];
  return same;
}

// static point_t normalize(point_t p, point_t p_old, delta_ind_t d_ind) {
// #ifdef DEBUG
//   fprintf(o1, "normalize: %s %s %s\n", p.str().c_str(),
//      p_old.str().c_str(), delta_ind_str(d_ind).c_str());
// #endif
//   assert(p.val.size() == p_old.val.size());
//   for (delta_ind_t::const_iterator i = d_ind.begin(); i != d_ind.end(); ++i) {
//     assert(*i < p.val.size());
//     if (p.val[*i] == p_old.val[*i])
//       p.val[*i] = (unsigned(p.val[*i])+1u) % conf.flags[*i]->size();
//   }
// #ifdef DEBUG
//   fprintf(o1, "normalize ret: %s\n", p.str().c_str());
// #endif
//   return p;
// }

static point_t get_next(point_t p, point_t p_old, delta_ind_t d_ind) {
#ifdef DEBUG
  fprintf(o1, "get_next: %s %s %s\n", p.str().c_str(),
	  p_old.str().c_str(), delta_ind_str(d_ind).c_str());
#endif
  for (delta_ind_t::const_iterator i = d_ind.begin(); i != d_ind.end(); ++i) {
    p.val[*i] = (unsigned(p.val[*i])+1) % conf.flags[*i]->size();
    if (p.val[*i] != p_old.val[*i])
      break;
  }
#ifdef DEBUG
  fprintf(o1, "get_next ret: %s\n", p.str().c_str());
#endif
  return p;
}

static bool advance(point_t* p, point_t p_old, delta_ind_t d_ind) {
  *p = get_next(*p, p_old, d_ind);
  return *p != p_old;
}

void search() {
  point_t p = get_min_point();
  obj_t res = measure(p);
  fprintf(o1, "\n## Best (so far):\n%s %s\n",
	  res.str().c_str(), p.str().c_str());

  steps = steps_t();
  for (;;) {
    // get new step data
    delta_ind_t d_ind = steps.get_next(p);
#ifdef DEBUG
    fprintf(o1, "%s", delta_ind_str(d_ind).c_str());
#endif
    if (steps.delta_info == finish) {
      return;
    }

#ifdef DEBUG
    assert(measure(p) == res);
#endif

    point_t p_start = p;
    point_t p_new = p;
    delta_t delta;

    do {
      // avoid options in p/delta
      if (skip(p_new, p_start, d_ind))
	continue;

      obj_t res_new = measure(p_new);

      // get difference
#ifdef DEBUG
      // assert(measure(p_old) == res_old);
#endif
      obj_t diff =  res_new - res;
      bool equal = equivalent_p(p_new, p);
      delta = delta_t(p_new, p, equal, diff);
// #ifdef DEBUG
      fprintf(o1, " point: %s res: %s delta: %s ",
	      p_new.str().c_str(), res_new.str().c_str(), delta.str().c_str());
// #endif
      steps.store(delta);

      // decide if this point is better or not
      unsigned optw1 = 0;
      unsigned optw2 = 0;
      for (delta_ind_t::const_iterator i = d_ind.begin();
	   i != d_ind.end(); ++i) {
	optw1 += p_new.val[*i] != 0 ? 1u : 0;
	optw2 += p.val[*i] != 0 ? 1u : 0;
      }
      if (equal ? optw1 < optw2 : diff < obj_t(0)) {

	p = p_new;
	res = res_new;

	fprintf(o1, "\nAlteration adopted: %s", delta.str().c_str());
	fprintf(o1, "\n%s %s", res.str().c_str(), p.str().c_str());

      }
    } while (advance(&p_new, p_start, d_ind));

    if (p != p_start) {
      steps.print();
      steps = steps_t();
    }
  }
}

void summary_first() {
  summary_first_measure();

  fprintf(o3, "\nCompiler version: %s", get_compiler_version().c_str());
  fprintf(o3, "\n");
}

void summary_exit() {
  fprintf(o3, "\n");
  fprintf(o3, "\nTotal number of compiliations and samples: %u, %u ",
	  progress.cnts['o'], progress.cnts['*']);
  fprintf(o3, "\n");
  fprintf(o3, "\nBest combination found:");
  point_t p = get_min_point();
  fprintf(o3, "\n%s %s", measure(p).str().c_str(), p.str().c_str());
  fprintf(o3, "\n");
  steps.summary_exit();
}
