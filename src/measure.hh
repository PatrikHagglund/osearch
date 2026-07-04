#ifndef MEASURE_HH
#define MEASURE_HH

#include "point.hh" // point_t
#include "obj.hh" // obj_t

/// \file
/// Measure (compile and sample) given a point_t, using measure(). Store results in
/// the results mapping.

/// Print out what we measure.
void summary_first_measure();

/// True when the active objective is generated code size (`-s`); false when
/// it is speed (wall-clock time or, with `-p`, retired instructions). Used to
/// pick which per-flag effectiveness weight steers the search order.
bool objective_is_size();

// results

/// Reset search. Scrap all measurment results (and progress printouts).
void reset_measurements();

/// Compile, sample and store the result (in results).
/// param p point_t
/// return obj_t
obj_t measure(const point_t& p);

/// Get the minimal point_t so far (from results).
/// \return point_t
/// \todo Make a member function of results(?).
point_t get_min_point();

/// Post-search validation pass. Starting from `get_min_point()`, try
/// removing each adopted flag; if removing it does not make the result
/// worse (by more than the `-T` threshold), drop it. Iterates to fixpoint.
/// \return the validated point_t (may have fewer active flags than
/// `get_min_point()`).
point_t validate();

/// Return true if `new_val` is a sufficient improvement over `baseline`
/// to warrant adoption. Honours the `-T permille` threshold (if set,
/// requires new_val <= baseline * (1 - threshold/1000)).
bool is_improvement(obj_t new_val, obj_t baseline);

#endif // MEASURE_HH
