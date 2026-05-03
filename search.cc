#include "search.hh"

#include "assume.hh" // NONNULL
#include "compile.hh"
#include "measure.hh"
#include "point.hh"
#include "print.hh"
#include "read_conf.hh"
#include "steps.hh"

#include <algorithm>

/// \file
/// Main loop (search()), to search for good options.

/// Check if p and p_old are "equal".
static bool skip(point_t p, point_t p_old, delta_ind_t const &d_ind) {
  return std::ranges::any_of(d_ind, [&](auto i) {
    return p.val[i] == p_old.val[i];
  });
}

// static point_t normalize(point_t p, point_t p_old, delta_ind_t d_ind) {
// #ifdef DEBUG
//   o1 << // "normalize: %s %s %s\n", p.str().c_str(),
//      p_old.str().c_str(), delta_ind_str(d_ind).c_str());
// #endif
//   contract_assert(p.val.size() == p_old.val.size());
//   for (auto const &i : d_ind)
//   {
//     contract_assert(i < p.val.size());
//     if (p.val[i] == p_old.val[i])
//       p.val[i] = (unsigned(p.val[i])+1u) % conf.flags[i]->size();
//   }
// #ifdef DEBUG
//   o1 << // "normalize ret: %s\n", p.str().c_str());
// #endif
//   return p;
// }

/// Step forward a point, increasing options. Used in advance().
/// \param p
/// \param p_old
/// \param d_ind
/// \return point_t (the "next" point)
/// \todo Improve documentation.
static point_t get_next(point_t p, point_t p_old, delta_ind_t const &d_ind) {
#ifdef DEBUG
  o1 << "get_next: " << p.to_string() << " " << p_old.to_string() << " "
     << delta_ind_str(d_ind) << "\n";
#endif
  for (uint16_t const i : d_ind) {
    p.val[i] = (static_cast<unsigned>(p.val[i]) + 1) %
               static_cast<unsigned char>(conf.flags[i]->size());
    if (p.val[i] != p_old.val[i]) {
      break;
    }
  }
#ifdef DEBUG
  o1 << "get_next ret: " << p.to_string() << "\n";
#endif
  return p;
}

/// Step forward a point using get_next().
/// \param p
/// \param p_old
/// \param d_ind
/// \return return false if a fixed point has been reached, true otherwise
static bool advance(NONNULL(point_t *) p, const point_t &p_old,
                    delta_ind_t const &d_ind) {
  *p = get_next(*p, p_old, d_ind);
  return *p != p_old;
}

/// Search for better points (until fixed point is reached).
/// \todo Improve documentation.
void search() {
  point_t p = get_min_point();
  obj_t res = measure(p);
  o1 << "\n## Best (so far):\n"
     << res.to_string() << " " << p.to_string() << "\n";
  steps = steps_t();
  for (;;) {
    // get new step data
    delta_ind_t const d_ind = steps.get_next(p);
#ifdef DEBUG
    o1 << delta_ind_str(d_ind);
#endif
    if (steps.delta_info == delta_info_t::finish) {
      return;
    }

#ifdef DEBUG
    contract_assert(measure(p) == res);
#endif

    point_t const p_start = p;
    point_t p_new = p;
    delta_t delta;

    do {
      // avoid options in p/delta
      if (skip(p_new, p_start, d_ind)) {
        continue;
      }

      obj_t const res_new = measure(p_new);

      // get difference (only meaningful when both measurements are finite;
      // otherwise mark as "infinitely worse" so the delta sorts last).
#ifdef DEBUG
      // contract_assert(measure(p_old) == res_old);
#endif
      obj_t const diff =
          (res_new.is_finite() && res.is_finite()) ? (res_new - res) : obj_t_inf;
      bool const equal = equivalent_p(p_new, p);
      delta = delta_t(p_new, p, equal, diff);
      // #ifdef DEBUG
      o1 << " point: " << p_new.to_string() << " res: " << res_new.to_string()
         << " delta: " << delta.str() << " ";
      // #endif
      steps.store(delta);

      // decide if this point is better or not
      if (equal ? p_new.popcnt() < p.popcnt() : res_new < res) {

        p = p_new;
        res = res_new;

        o1 << "\nAlteration adopted: " << delta.str();
        o1 << "\n" << res.to_string() << " " << p.to_string();
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
  point_t const p = get_min_point();
  o3 << "\n" << measure(p).to_string() << " " << p.to_string() << "\n";
  steps.summary_exit();
}
