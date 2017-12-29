#ifndef ASSUME_HH
#define ASSUME_HH

#ifdef __GNUC__
#define GNUC_BUILTIN_ASSUME(cond) if (!(cond)) __builtin_unreachable()
#endif

#ifdef __clang__
#define NONNULL(type) type _Nonnull
#define NONNULL_IMPLICIT(type) type _Nonnull
#define NULLABLE(type) type _Nullable
#else
#include <gsl/gsl>
#define NONNULL(type) gsl::not_null<type>
// no non-explicit not_null?
// https://github.com/Microsoft/GSL/issues/395
#define NONNULL_IMPLICIT(type) type
#define NULLABLE(type) type
#endif

#endif
