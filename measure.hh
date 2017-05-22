#ifndef MEASURE_HH
#define MEASURE_HH

#include "point.hh"

void summary_first_measure();

// results

void reset_measurements();

obj_t measure(const point_t& p);

point_t get_min_point();

#endif // MEASURE_HH
