#ifndef OBJ_HH
#define OBJ_HH

#include "assume.hh" // GNUC_BUILTIN_ASSUME

#include <string>
#include <compare> // <=>

#include <cstdint> // int64_t

/// \file
/// For obj_t (objective function results).

/// Given a numeric value, return sign: -1, 0, or 1.
/// \todo Replace with C++20 <=> operator.
template <typename T> constexpr int sign_T(T val) {
    return (T(0) < val) - (val < T(0));
}
/// Given an int64_t value, return sign: -1, 0, or 1.
static constexpr int sign(int64_t val) { return sign_T<int64_t>(val); }

/// Type for objective function results. Integer type (int64_t) extended with "infinity" values.
struct obj_t {
  explicit obj_t() = default;
  constexpr explicit obj_t(int64_t v, bool is_inf = false) : inf(is_inf), val(v) {}
  constexpr obj_t operator-(obj_t const& obj) const {
    GNUC_BUILTIN_ASSUME((!inf || (sign(val) <=> val) == 0) && (!obj.inf || (sign(obj.val) <=> obj.val) == 0));
    return obj_t(inf || obj.inf, (obj.inf ? 0 : val) - (inf ? 0 : obj.val));
  }
  constexpr bool operator==(obj_t const &obj) const {
    return (inf <=> obj.inf) == 0 && (val <=> obj.val) == 0;
  }
  constexpr bool operator<(obj_t const &obj) const { return 0 < (obj - *this).val; }
  constexpr bool operator>(obj_t const &obj) const {
    return !(*this < obj || *this == obj);
  }
  // constexpr std::strong_ordering operator<=>(obj_t const &obj) const {
  //   if (inf)

  //   return val <=> obj.val;
  // }

  [[nodiscard]] CONSTEXPR_STR std::string to_string() const {
    return inf ? (std::string(val < 0 ? "-" : "") + "inf") : std::to_string(val);
  }

private:
  constexpr explicit obj_t(bool i, int64_t v) : inf(i), val(inf ? sign(v) : v) {}
  bool inf; // mark infinite values
  int64_t val; // value (sign for infinite values)
};

constexpr obj_t obj_t_inf = obj_t(0, /*is_inf*/ true);

static_assert(std::is_class_v<obj_t>);
static_assert(std::is_standard_layout_v<obj_t>);
static_assert(std::is_trivially_copyable_v<obj_t>);
static_assert(!std::is_polymorphic_v<obj_t>);

static_assert(std::is_default_constructible_v<obj_t>);
static_assert(std::is_trivially_default_constructible_v<obj_t>);
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
