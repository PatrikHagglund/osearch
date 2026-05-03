#ifndef ASSUME_HH
#define ASSUME_HH

/// \file
/// Language extensions, such as #NONNULL and #CONSTEXPR_STR.

#ifdef __clang__
#define NONNULL(type) type _Nonnull
#define NULLABLE(type) type _Nullable
#else
#include <gsl/gsl>
/// Pointer type attribute to mark as non-null.
/// \todo Check if some can be replaced with references.
#define NONNULL(type) gsl::not_null<type>
/// Opposite of NONNULL. (Currently unused.)
#define NULLABLE(type) type
#endif

#ifdef __clang__
#define CONSTEXPR_STR
#else
#define CONSTEXPR_STR constexpr
#endif

#endif
