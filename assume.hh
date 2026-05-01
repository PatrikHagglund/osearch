#ifndef ASSUME_HH
#define ASSUME_HH

/// \file
/// Language extensions, such as #GNUC_BUILTIN_ASSUME and #NONNULL.

#ifdef USE_CONTRACTS
/// Use C++26 contract_assert for assumptions.
#define GNUC_BUILTIN_ASSUME(cond) contract_assert(cond)
#elif defined(__GNUC__)
static constexpr bool my_id(bool cond) { return cond; }
#if defined(__clang__) && 0
#define GNUC_BUILTIN_ASSUME(cond) __builtin_assume(my_id(cond))
#else
#define GNUC_BUILTIN_ASSUME(cond) if (my_id(!(cond))) __builtin_unreachable()
#endif
#else
/// Assert without any side effect. (Should be detected in UBSan or similar.)
#define GNUC_BUILTIN_ASSUME(cond)
#endif

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
