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
  unsigned popcnt() const;
  /// To string operator. Print flags space separated.
  std::string to_string() const;
#if 0
  void set_rand();
#endif
  /// Type for storing list of flag values.
  using val_t = std::vector<uint8_t>;
  /// For each config flag, store its value.
  val_t val;
};

static_assert(std::is_class<point_t>::value);
static_assert(std::is_standard_layout<point_t>::value);
static_assert(!std::is_trivially_copyable<point_t>::value);
static_assert(!std::is_polymorphic<point_t>::value);
//static_assert(!std::is_literal_type<point_t>::value);

static_assert(std::is_default_constructible<point_t>::value);
static_assert(!std::is_trivially_default_constructible<point_t>::value);
static_assert(!std::is_nothrow_default_constructible<point_t>::value);
static_assert(std::is_copy_constructible<point_t>::value);
static_assert(!std::is_trivially_copy_constructible<point_t>::value);
static_assert(!std::is_nothrow_copy_constructible<point_t>::value);
static_assert(std::is_move_constructible<point_t>::value);
static_assert(!std::is_trivially_move_constructible<point_t>::value);
static_assert(!std::is_nothrow_move_constructible<point_t>::value);
static_assert(std::is_copy_assignable<point_t>::value);
static_assert(!std::is_trivially_copy_assignable<point_t>::value);
static_assert(!std::is_nothrow_copy_assignable<point_t>::value);
static_assert(std::is_move_assignable<point_t>::value);
static_assert(!std::is_trivially_move_assignable<point_t>::value);
static_assert(!std::is_nothrow_move_assignable<point_t>::value);
static_assert(std::is_destructible<point_t>::value);
static_assert(!std::is_trivially_destructible<point_t>::value);
static_assert(std::is_nothrow_destructible<point_t>::value);


/// \file
/// \todo "Standard" type configuration checks?

static_assert(std::is_class<std::string>::value);
static_assert(std::is_standard_layout<std::string>::value);
static_assert(!std::is_trivially_copyable<std::string>::value);
static_assert(!std::is_polymorphic<std::string>::value);
//static_assert(!std::is_literal_type<std::string>::value);

static_assert(std::is_default_constructible<std::string>::value);
static_assert(!std::is_trivially_default_constructible<std::string>::value);
static_assert(std::is_nothrow_default_constructible<std::string>::value);
static_assert(std::is_copy_constructible<std::string>::value);
static_assert(!std::is_trivially_copy_constructible<std::string>::value);
static_assert(!std::is_nothrow_copy_constructible<std::string>::value);
static_assert(std::is_move_constructible<std::string>::value);
static_assert(!std::is_trivially_move_constructible<std::string>::value);
static_assert(std::is_nothrow_move_constructible<std::string>::value);
static_assert(std::is_copy_assignable<std::string>::value);
static_assert(!std::is_trivially_copy_assignable<std::string>::value);
static_assert(!std::is_nothrow_copy_assignable<std::string>::value);
static_assert(std::is_move_assignable<std::string>::value);
static_assert(!std::is_trivially_move_assignable<std::string>::value);
static_assert(std::is_nothrow_move_assignable<std::string>::value);
static_assert(std::is_destructible<std::string>::value);
static_assert(!std::is_trivially_destructible<std::string>::value);
static_assert(std::is_nothrow_destructible<std::string>::value);


static_assert(std::is_class<std::vector<uint8_t>>::value);
static_assert(std::is_standard_layout<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_copyable<std::vector<uint8_t>>::value);
static_assert(!std::is_polymorphic<std::vector<uint8_t>>::value);
//static_assert(!std::is_literal_type<std::vector<uint8_t>>::value);

static_assert(std::is_default_constructible<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_default_constructible<std::vector<uint8_t>>::value);
static_assert(std::is_nothrow_default_constructible<std::vector<uint8_t>>::value);
static_assert(std::is_copy_constructible<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_copy_constructible<std::vector<uint8_t>>::value);
static_assert(!std::is_nothrow_copy_constructible<std::vector<uint8_t>>::value);
static_assert(std::is_move_constructible<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_move_constructible<std::vector<uint8_t>>::value);
static_assert(std::is_nothrow_move_constructible<std::vector<uint8_t>>::value);
static_assert(std::is_copy_assignable<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_copy_assignable<std::vector<uint8_t>>::value);
static_assert(!std::is_nothrow_copy_assignable<std::vector<uint8_t>>::value);
static_assert(std::is_move_assignable<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_move_assignable<std::vector<uint8_t>>::value);
static_assert(std::is_nothrow_move_assignable<std::vector<uint8_t>>::value);
static_assert(std::is_destructible<std::vector<uint8_t>>::value);
static_assert(!std::is_trivially_destructible<std::vector<uint8_t>>::value);
static_assert(std::is_nothrow_destructible<std::vector<uint8_t>>::value);

#endif // POINT_HH
POINT_HH
