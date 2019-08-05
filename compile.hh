#ifndef COMPILE_HH
#define COMPILE_HH

#include "point.hh"  // for point_t
#include <string>    // for string
struct tmp_file_t;

/// \file
/// Compile given a point_t, using compile(). Store results in exe_files.

/// Source file to optimize on.
extern std::string input_file;

/// Print out information before starting.
void summary_first_compile();

/// Number/id for set of points compiling to the same executable.
using pset_t = unsigned int;

/// Number/id for an "invalid" set of points.
inline constexpr pset_t pset_invalid = 0;

/// Compile given a point_t, if no previous compilation has been done
/// for this point (on demand).
/// \param p a point_t list of options
/// \return a pset_t
pset_t compile(const point_t& p);

/// Helper function for point_to_pset: Check if two point_t are in the same pset.
/// \param p1 point_t
/// \param p2 point_t
/// \return true if in the same pset, false otherwise
/// \todo Make a member function.
bool equivalent_p(const point_t& p1, const point_t& p2);

/// Lookup path name (in exe_files) for generated file, given a pset_t.
/// \param p pset_t
/// \return tmp_file_t
/// \todo Make this a exe_file_t member function.
tmp_file_t const& get_tmp_file(pset_t p);

/// Given a pset_t, return a point_t representative (the "minimal" option string).
/// \param pset pset_t
/// \return point_t
/// \todo Make a member function.
point_t get_point(pset_t pset);

/// Reset search. Scrap all compilations.
void reset_compilations();

/// Print out the version of the compiler used.
std::string get_compiler_version();

#endif
