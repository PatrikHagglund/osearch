#include "search.hh"

#include "compat.hh" // NONNULL
#include "compile.hh"
#include "measure.hh"
#include "point.hh"
#include "print.hh"
#include "read_conf.hh"
#include "steps.hh"

#include <algorithm>
#include <thread>
#include <vector>

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
               static_cast<unsigned char>(flag::flag_size(conf.flags[i]));
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
    // Collect points from multiple steps for batch compilation.
    struct step_data {
      delta_ind_t d_ind;
      std::vector<point_t> points;
    };
    std::vector<step_data> pending_steps;
    std::vector<point_t> all_points;

    // Gather up to hardware_concurrency steps worth of points.
    unsigned const batch_limit =
        std::max(1U, std::thread::hardware_concurrency());
    while (pending_steps.size() < batch_limit) {
      delta_ind_t const d_ind = steps.get_next(p);
      if (steps.delta_info == delta_info_t::finish) {
        break;
      }
      step_data sd;
      sd.d_ind = d_ind;
      point_t p_pre = p;
      do {
        if (!skip(p_pre, p, d_ind)) {
          sd.points.push_back(p_pre);
          all_points.push_back(p_pre);
        }
      } while (advance(&p_pre, p, d_ind));
      pending_steps.push_back(std::move(sd));
    }

    if (pending_steps.empty()) {
      return;
    }

    // Batch-compile all collected points in parallel.
    compile_batch(all_points);

    // Evaluate each step sequentially.
    bool adopted = false;
    for (auto &sd : pending_steps) {
      point_t const p_start = p;

      for (auto &p_new : sd.points) {
        obj_t const res_new = measure(p_new);

        obj_t const diff =
            (res_new.is_finite() && res.is_finite()) ? (res_new - res) : obj_t_inf;
        bool const equal = equivalent_p(p_new, p);
        delta_t delta(p_new, p, equal, diff);
        o1 << " point: " << p_new.to_string() << " res: " << res_new.to_string()
           << " delta: " << delta.str() << " ";
        steps.store(delta);

        if (equal ? p_new.active_count() < p.active_count() : is_improvement(res_new, res)) {
          p = p_new;
          res = res_new;
          o1 << "\nAlteration adopted: " << delta.str();
          o1 << "\n" << res.to_string() << " " << p.to_string();
        }
      }

      if (p != p_start) {
        adopted = true;
        steps.print();
        steps = steps_t();
        break;
      }
    }

    if (adopted) {
      continue;
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
  point_t const p = validate();
  o3 << "\nBest combination found:";
  o3 << "\n" << measure(p).to_string() << " " << p.to_string() << "\n";
  steps.summary_exit();
  steps.json_exit();
}
