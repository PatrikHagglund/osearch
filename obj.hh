#ifndef OBJ_HH
#define OBJ_HH

#include "compat.hh" // CONSTEXPR_STR

#include <compare> // std::strong_ordering
#include <cstdint> // int64_t
#include <string>

/// \file
/// For obj_t (objective function results).
///
/// An obj_t is either a finite int64 measurement or a "worst" sentinel
/// (obj_t_inf), used when a compile/run fails. The type has a total order:
/// every finite value is less than obj_t_inf. Arithmetic is only defined
/// between finite values.

/// Type for objective function results.
///
/// Represented as a finite int64 value, or a sentinel standing for
/// "infinitely bad" (larger than any finite value in the total order).
/// Use obj_t_inf for the sentinel.
struct obj_t {
  /// Default: finite 0. (Consequence: ``is_trivially_default_constructible``
  /// does not hold — we always produce a valid finite zero rather than
  /// indeterminate state.)
  constexpr obj_t() = default;
  /// Construct from a finite int64 value.
  constexpr explicit obj_t(int64_t v) : val_(v), finite_(true) {}

  /// True if this is a finite measurement.
  [[nodiscard]] constexpr bool is_finite() const { return finite_; }

  /// Total order: finite values ordered by val_; obj_t_inf is greater
  /// than every finite value and equal to itself.
  constexpr std::strong_ordering operator<=>(obj_t const& o) const {
    if (finite_ != o.finite_) {
      // finite < non-finite
      return finite_ ? std::strong_ordering::less
                     : std::strong_ordering::greater;
    }
    // Both finite, or both non-finite (val_ is unused when !finite_).
    return finite_ ? (val_ <=> o.val_) : std::strong_ordering::equal;
  }
  constexpr bool operator==(obj_t const& o) const {
    return finite_ == o.finite_ && (!finite_ || val_ == o.val_);
  }

  /// Subtraction is defined only for finite operands. Result is finite.
  constexpr obj_t operator-(obj_t const& o) const
    pre(finite_ && o.finite_)
  {
    return obj_t(val_ - o.val_);
  }
  /// Unary negation, defined only for finite values.
  constexpr obj_t operator-() const
    pre(finite_)
  {
    return obj_t(-val_);
  }

  [[nodiscard]] CONSTEXPR_STR std::string to_string() const {
    return finite_ ? std::to_string(val_) : std::string{"inf"};
  }

private:
  /// Tag-dispatched ctor for the sentinel; not part of the public API.
  struct sentinel_tag {};
  constexpr explicit obj_t(sentinel_tag) : val_(0), finite_(false) {}
  friend consteval obj_t make_obj_t_inf();

  int64_t val_{};
  bool finite_{true};
};

/// Helper used to construct obj_t_inf at compile time without exposing
/// the sentinel constructor.
consteval obj_t make_obj_t_inf() { return obj_t(obj_t::sentinel_tag{}); }

/// The "worst possible" obj_t value. Compares greater than every finite
/// obj_t; returned by measure() when compile/run fails.
inline constexpr obj_t obj_t_inf = make_obj_t_inf();

static_assert(std::is_class_v<obj_t>);
static_assert(std::is_standard_layout_v<obj_t>);
static_assert(std::is_trivially_copyable_v<obj_t>);
static_assert(!std::is_polymorphic_v<obj_t>);

static_assert(std::is_default_constructible_v<obj_t>);
static_assert(std::is_nothrow_default_constructible_v<obj_t>);
static_assert(std::is_copy_constructible_v<obj_t>);
static_assert(std::is_trivially_copy_constructible_v<obj_t>);
static_assert(std::is_nothrow_copy_constructible_v<obj_t>);
static_assert(std::is_move_constructible_v<obj_t>);
static_assert(std::is_trivially_move_constructible_v<obj_t>);
static_assert(std::is_nothrow_move_constructible_v<obj_t>);
static_assert(std::is_copy_assignable_v<obj_t>);
static_assert(std::is_trivially_copy_assignable_v<obj_t>);
static_assert(std::is_nothrow_copy_assignable_v<obj_t>);
static_assert(std::is_move_assignable_v<obj_t>);
static_assert(std::is_trivially_move_assignable_v<obj_t>);
static_assert(std::is_nothrow_move_assignable_v<obj_t>);
static_assert(std::is_destructible_v<obj_t>);
static_assert(std::is_trivially_destructible_v<obj_t>);
static_assert(std::is_nothrow_destructible_v<obj_t>);


#endif // OBJ_HH
