#include "print.hh"
#include "getopts.hh"

/// \file
/// Progress printouts, using progress_t and its progress_t::tick() method.

/// Global variable for progress printouts.
progress_t progress;

/// Option -q: suppress progress output.
static void opt_q() { progress.silent = true; }
static int dummy_ = (opt_reg_t::append('q', opt_q, "q", "  [-q]",
                                        "  -q \t\tsuppress progress output\n"),
                     1);

/// Print explanation of progress symbols used.
void progress_t::print_symbols() const {
  if (silent) return;
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
  contract_assert(sym == '.' || sym == 'o' || sym == '*');

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
