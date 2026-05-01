#ifndef POINT_HH
#define POINT_HH

#include <string>
#include <compare>
#include <cstdint>
#include <inplace_vector>

/// \file
/// For point_t (measurement point).

/// Measurement point, represented as a list of option values.
/// Trivially copyable thanks to inplace_vector (no heap allocation).
struct point_t {
  /// Max number of flags supported.
  static constexpr size_t max_flags = 256;
  /// Set size to number of flags in #conf, and initialize all to 0.
  explicit point_t();
#if 0
  /// Set to space separated option string.
  explicit point_t(std::string flags_str);
#endif
  /// Destructor.
  ~point_t() = default;
  /// Defaulted comparison operators (lexicographic on val).
  auto operator<=>(point_t const&) const = default;
  bool operator==(point_t const&) const = default;
  /// Number of non-zero values.
  [[nodiscard]] unsigned popcnt() const;
  /// To string operator. Print flags space separated.
  [[nodiscard]] std::string to_string() const;
#if 0
  void set_rand();
#endif
  /// Type for storing list of flag values.
  using val_t = std::inplace_vector<uint8_t, max_flags>;
  /// For each config flag, store its value.
  val_t val;
};

static_assert(std::is_class_v<point_t>);
static_assert(std::is_standard_layout_v<point_t>);
static_assert(std::is_trivially_copyable_v<point_t>);
static_assert(!std::is_polymorphic_v<point_t>);

static_assert(std::is_default_constructible_v<point_t>);
static_assert(!std::is_trivially_default_constructible_v<point_t>);
static_assert(!std::is_nothrow_default_constructible_v<point_t>);
static_assert(std::is_copy_constructible_v<point_t>);
static_assert(std::is_trivially_copy_constructible_v<point_t>);
static_assert(std::is_nothrow_copy_constructible_v<point_t>);
static_assert(std::is_move_constructible_v<point_t>);
static_assert(std::is_trivially_move_constructible_v<point_t>);
static_assert(std::is_nothrow_move_constructible_v<point_t>);
static_assert(std::is_copy_assignable_v<point_t>);
static_assert(std::is_trivially_copy_assignable_v<point_t>);
static_assert(std::is_nothrow_copy_assignable_v<point_t>);
static_assert(std::is_move_assignable_v<point_t>);
static_assert(std::is_trivially_move_assignable_v<point_t>);
static_assert(std::is_nothrow_move_assignable_v<point_t>);
static_assert(std::is_destructible_v<point_t>);
static_assert(std::is_trivially_destructible_v<point_t>);
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



#endif // POINT_HH
POINT_HH
