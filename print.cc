#include "print.hh"
#include "assume.hh"

/// \file
/// Progress printouts, using progress_t and its progress_t::tick() method.

/// Global variable for progress printouts.
progress_t progress;

/// Print explanation of progress symbols used.
void progress_t::print_symbols() const {
  *o << "Progress indicators used. For a given option string:\n"
        " .  request measurement result\n"
        " o  compile\n"
        " *  measure\n";
}

/// Print out a progress tick.
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

/// Print out a progress newline.
void progress_t::newl() const {
  if (!silent) {
    *o << "\n";
  }
}
