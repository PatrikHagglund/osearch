#include "print.hh"
#include <cstdio>
#include <cassert>

/** \file print.cc Progress printouts.
 */

FILE* o1 = stdout; ///< for normal output
FILE* o2 = stdout; ///< for progress indicators
FILE* o3 = stdout; ///< for summary output

/** To be written */

progress_t progress;

progress_t::progress_t(): cnts(), o(o2), silent(false) {}

void progress_t::print_symbols() const {
  fprintf(o, "Progress indicators used. For a given option string:\n"
	  " .  request measurement result\n"
	  " o  compile\n"
	  " *  measure\n");
}

#ifdef DEBUG
void progress_t::tick(char sym, point_t p) {
#else
void progress_t::tick(char sym) {
#endif
  // only allow '.' to be supressed
  assert(sym == '.' || !silent && (sym == 'o' || sym == '*'));

  ++cnts[sym];

#ifdef DEBUG
  (void)p; // FIXME
#endif
  if (!silent)
    fprintf(o, "%c", sym); fflush(o);
}

void progress_t::newl() const {
  if (!silent)
    fprintf(o, "\n");
}

progress_t& progress_t::operator=(progress_t const& p) {
  this->cnts = p.cnts;
  this->o = p.o;
  this->silent = p.silent;
  return *this;
}

progress_t::progress_t(progress_t const&): cnts(), o(), silent() {
  assert(false);
}
