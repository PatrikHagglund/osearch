#include "print.hh"
#include "assume.hh"

#include <cstdio>

/** \file print.cc Progress printouts.
 */

/** To be written */

progress_t progress;

void progress_t::print_symbols() const {
  *o << "Progress indicators used. For a given option string:\n"
        " .  request measurement result\n"
        " o  compile\n"
        " *  measure\n";
}

#ifdef DEBUG
void progress_t::tick(char sym, point_t const &p) {
#else
void progress_t::tick(char sym) {
#endif
  // only allow '.' to be supressed
  GNUC_BUILTIN_ASSUME(sym == '.' || (!silent && (sym == 'o' || sym == '*')));

  ++cnts[sym];

#ifdef DEBUG
  (void)p; // FIXME
#endif
  if (!silent) {
    *o << sym;
    o->flush();
  }
}

void progress_t::newl() const {
  if (!silent) {
    *o << "\n";
  }
}
