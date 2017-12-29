#include "search.hh"

#include "assume.hh"
#include "compile.hh"
#include "measure.hh"
#include "point.hh"
#include "print.hh"
#include "read_conf.hh"
#include "steps.hh"

#include <string>
#include <utility>
#include <vector>

static bool skip(point_t p, point_t p_old, delta_ind_t const &d_ind) {
  bool same = false;
  for (uint16_t i : d_ind) {
    same = same || p.val[i] == p_old.val[i];
  }
  return same;
}

// static point_t normalize(point_t p, point_t p_old, delta_ind_t d_ind) {
// #ifdef DEBUG
//   o1 << // "normalize: %s %s %s\n", p.str().c_str(),
//      p_old.str().c_str(), delta_ind_str(d_ind).c_str());
// #endif
//   GNUC_BUILTIN_ASSUME(p.val.size() == p_old.val.size());
//   for (auto const &i : d_ind)
//   {
//     GNUC_BUILTIN_ASSUME(i < p.val.size());
//     if (p.val[i] == p_old.val[i])
//       p.val[i] = (unsigned(p.val[i])+1u) % conf.flags[i]->size();
//   }
// #ifdef DEBUG
//   o1 << // "normalize ret: %s\n", p.str().c_str());
// #endif
//   return p;
// }

static point_t get_next(point_t p, point_t p_old, delta_ind_t const &d_ind) {
#ifdef DEBUG
  o1 << "get_next: " << p.str() << " " << p_old.str() << " "
     << delta_ind_str(d_ind) << "\n";
#endif
  for (uint16_t i : d_ind) {
    p.val[i] = (unsigned(p.val[i]) + 1) %
               static_cast<unsigned char>(conf.flags[i]->size());
    if (p.val[i] != p_old.val[i]) {
      break;
    }
  }
#ifdef DEBUG
  o1 << "get_next ret: " << p.str() << "\n";
#endif
  return p;
}

static bool advance(NONNULL_IMPLICIT(point_t *) p, const point_t &p_old,
                    delta_ind_t const &d_ind) {
  *p = get_next(*p, p_old, d_ind);
  return *p != p_old;
}

void search() {
  point_t p = get_min_point();
  obj_t res = measure(p);
  o1 << "\n## Best (so far):\n" << res.str() << " " << p.str() << "\n";
  steps = steps_t();
  for (;;) {
    // get new step data
    delta_ind_t d_ind = steps.get_next(p);
#ifdef DEBUG
    o1 << delta_ind_str(d_ind);
#endif
    if (steps.delta_info == finish) {
      return;
    }

#ifdef DEBUG
    GNUC_BUILTIN_ASSUME(measure(p) == res);
#endif

    point_t p_start = p;
    point_t p_new = p;
    delta_t delta;

    do {
      // avoid options in p/delta
      if (skip(p_new, p_start, d_ind)) {
        continue;
      }

      obj_t res_new = measure(p_new);

      // get difference
#ifdef DEBUG
      // GNUC_BUILTIN_ASSUME(measure(p_old) == res_old);
#endif
      obj_t diff = res_new - res;
      bool equal = equivalent_p(p_new, p);
      delta = delta_t(p_new, p, equal, diff);
      // #ifdef DEBUG
      o1 << " point: " << p_new.str() << " res: " << res_new.str()
         << " delta: " << delta.str() << " ";
      // #endif
      steps.store(delta);

      // decide if this point is better or not
      unsigned optw1 = 0;
      unsigned optw2 = 0;
      for (uint16_t i : d_ind) {
        optw1 += p_new.val[i] != 0 ? 1U : 0;
        optw2 += p.val[i] != 0 ? 1U : 0;
      }
      if (equal ? optw1 < optw2 : diff < obj_t(0)) {

        p = p_new;
        res = res_new;

        o1 << "\nAlteration adopted: " << delta.str();
        o1 << "\n" << res.str() << " " << p.str();
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
  o3 << "\nCompiler version: " << get_compiler_version() << "\n";
}

void summary_exit() {
  o3 << "\n";
  o3 << "\nTotal number of compiliations and samples: " << progress.cnts['o']
     << ", " << progress.cnts['*'] << " ";
  o3 << "\n";
  o3 << "\nBest combination found:";
  point_t p = get_min_point();
  o3 << "\n" << measure(p).str() << " " << p.str() << "\n";
  steps.summary_exit();
}
