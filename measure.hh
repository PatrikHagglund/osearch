#ifndef MEASURE_HH
#define MEASURE_HH

#include "point.hh" // point_t
#include "obj.hh" // obj_t

/// \file
/// Measure (compile and sample) given a point_t, using measure(). Store results in
/// the results mapping.

/// Print out what we measure.
void summary_first_measure();

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

#endif // MEASURE_HH
