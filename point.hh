#ifndef POINT_HH
#define POINT_HH

#include <string>
#include <cstdint>
#include <vector>

/// \file
/// For point_t (measurement point).

/// Measurement point, represented as a list of option values.
/// \todo Try template over length of flags list. (To make this class trivially copyable.)
struct point_t {
  /// Set size to number of flags in #conf, and initialize all to 0.
  explicit point_t();
#if 0
  /// Set to space separated option string.
  explicit point_t(std::string flags_str);
#endif
  /// Destructor (needed to clean up #val).
  ~point_t() = default;
  /// Equal operator.
  bool operator==(point_t const& p) const;
  /// Not equal operator.
  bool operator!=(point_t const& p) const;
  /// Less than operator.
  /// \todo Replace with C++20 <=> operator.
  bool operator<(point_t const& p) const;
  /// Number of non-zero values.
  [[nodiscard]] unsigned popcnt() const;
  /// To string operator. Print flags space separated.
  [[nodiscard]] std::string to_string() const;
#if 0
  void set_rand();
#endif
  /// Type for storing list of flag values.
  using val_t = std::vector<uint8_t>;
  /// For each config flag, store its value.
  val_t val;
};

static_assert(std::is_class_v<point_t>);
static_assert(std::is_standard_layout_v<point_t>);
static_assert(!std::is_trivially_copyable_v<point_t>);
static_assert(!std::is_polymorphic_v<point_t>);
//static_assert(!std::is_literal_type<point_t>::value);

static_assert(std::is_default_constructible_v<point_t>);
static_assert(!std::is_trivially_default_constructible_v<point_t>);
static_assert(!std::is_nothrow_default_constructible_v<point_t>);
static_assert(std::is_copy_constructible_v<point_t>);
static_assert(!std::is_trivially_copy_constructible_v<point_t>);
static_assert(!std::is_nothrow_copy_constructible_v<point_t>);
static_assert(std::is_move_constructible_v<point_t>);
static_assert(!std::is_trivially_move_constructible_v<point_t>);
static_assert(!std::is_nothrow_move_constructible_v<point_t>);
static_assert(std::is_copy_assignable_v<point_t>);
static_assert(!std::is_trivially_copy_assignable_v<point_t>);
static_assert(!std::is_nothrow_copy_assignable_v<point_t>);
static_assert(std::is_move_assignable_v<point_t>);
static_assert(!std::is_trivially_move_assignable_v<point_t>);
static_assert(!std::is_nothrow_move_assignable_v<point_t>);
static_assert(std::is_destructible_v<point_t>);
static_assert(!std::is_trivially_destructible_v<point_t>);
static_assert(std::is_nothrow_destructible_v<point_t>);


/// \file
/// \todo "Standard" type configuration checks?

static_assert(std::is_class_v<std::string>);
static_assert(std::is_standard_layout_v<std::string>);
static_assert(!std::is_trivially_copyable_v<std::string>);
static_assert(!std::is_polymorphic_v<std::string>);
//static_assert(!std::is_literal_type<std::string>::value);

static_assert(std::is_default_constructible_v<std::string>);
static_assert(!std::is_trivially_default_constructible_v<std::string>);
static_assert(std::is_nothrow_default_constructible_v<std::string>);
static_assert(std::is_copy_constructible_v<std::string>);
static_assert(!std::is_trivially_copy_constructible_v<std::string>);
static_assert(!std::is_nothrow_copy_constructible_v<std::string>);
static_assert(std::is_move_constructible_v<std::string>);
static_assert(!std::is_trivially_move_constructible_v<std::string>);
static_assert(std::is_nothrow_move_constructible_v<std::string>);
static_assert(std::is_copy_assignable_v<std::string>);
static_assert(!std::is_trivially_copy_assignable_v<std::string>);
static_assert(!std::is_nothrow_copy_assignable_v<std::string>);
static_assert(std::is_move_assignable_v<std::string>);
static_assert(!std::is_trivially_move_assignable_v<std::string>);
static_assert(std::is_nothrow_move_assignable_v<std::string>);
static_assert(std::is_destructible_v<std::string>);
static_assert(!std::is_trivially_destructible_v<std::string>);
static_assert(std::is_nothrow_destructible_v<std::string>);


static_assert(std::is_class_v<std::vector<uint8_t>>);
static_assert(std::is_standard_layout_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_copyable_v<std::vector<uint8_t>>);
static_assert(!std::is_polymorphic_v<std::vector<uint8_t>>);
//static_assert(!std::is_literal_type<std::vector<uint8_t>>::value);

static_assert(std::is_default_constructible_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_default_constructible_v<std::vector<uint8_t>>);
static_assert(std::is_nothrow_default_constructible_v<std::vector<uint8_t>>);
static_assert(std::is_copy_constructible_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_copy_constructible_v<std::vector<uint8_t>>);
static_assert(!std::is_nothrow_copy_constructible_v<std::vector<uint8_t>>);
static_assert(std::is_move_constructible_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_move_constructible_v<std::vector<uint8_t>>);
static_assert(std::is_nothrow_move_constructible_v<std::vector<uint8_t>>);
static_assert(std::is_copy_assignable_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_copy_assignable_v<std::vector<uint8_t>>);
static_assert(!std::is_nothrow_copy_assignable_v<std::vector<uint8_t>>);
static_assert(std::is_move_assignable_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_move_assignable_v<std::vector<uint8_t>>);
static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
static_assert(std::is_destructible_v<std::vector<uint8_t>>);
static_assert(!std::is_trivially_destructible_v<std::vector<uint8_t>>);
static_assert(std::is_nothrow_destructible_v<std::vector<uint8_t>>);

#endif // POINT_HH
POINT_HH
