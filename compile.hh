#ifndef COMPILE_HH
#define COMPILE_HH

#include "point.hh"  // for point_t
#include <string>    // for string
struct tmp_file_t;

extern std::string input_file;

void summary_first_compile();

// set of points compiling to the same executable
using pset_t = unsigned int;

pset_t error_pset(); // handle for set of "invalid" points

pset_t compile(const point_t& p);

bool equivalent_p(const point_t& p1, const point_t& p2);

tmp_file_t const& get_tmp_file(pset_t p);

point_t get_point(pset_t pset);

void reset_compilations();

// other

std::string get_compiler_version();

#endif
