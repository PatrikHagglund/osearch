#ifndef POINT_HH
#define POINT_HH

#include "assume.hh" // GNUC_BUILTIN_ASSUME

#include <sstream>
#include <string>
#include <vector>

// point_t

struct point_t {
  explicit point_t();
  explicit point_t(std::string flags_str);
  bool operator==(point_t const& p) const;
  bool operator!=(point_t const& p) const;
  bool operator<(point_t const& p) const;
  std::string str() const;
  void set_rand();
  using val_t = std::vector<uint8_t>;
  // storage
  // for each config flag, store its value
  val_t val;
};

//static_assert(std::is_trivially_copyable<point_t>::value);


// obj_t

static constexpr int sign(int64_t i) { return i == 0 ? 0 : (i < 0 ? -1 : 1); }

struct obj_t {
  explicit obj_t() = default;
  constexpr explicit obj_t(int64_t v) : inf(false), val(v) {};
  explicit obj_t(const std::string& str)
    : inf(str == "inf"),
      val(inf ? 1 : int64_t(strtoul(str.c_str(), nullptr, 0))) {
    GNUC_BUILTIN_ASSUME(!str.empty());
  }
  constexpr obj_t operator-(obj_t const& obj) const {
    GNUC_BUILTIN_ASSUME((!inf || sign(val) == val) && (!obj.inf || sign(obj.val) == obj.val));
    return obj_t(inf || obj.inf, (obj.inf ? 0 : val) - (inf ? 0 : obj.val));
  }
  constexpr bool operator==(obj_t const &obj) const {
    return inf == obj.inf && val == obj.val;
  }
  constexpr bool operator<(obj_t const &obj) const { return 0 < (obj - *this).val; }
  constexpr bool operator>(obj_t const &obj) const {
    return !(*this < obj || *this == obj);
  }
  std::string str() const {
    if (inf) {
      return std::string(val < 0 ? "-" : "") + "inf";
    }
    std::ostringstream ss;
    ss << val;
    return ss.str();
  }

private:
  constexpr explicit obj_t(bool i, int64_t v) : inf(i), val(inf ? sign(v) : v) {}
  bool inf; // mark infinite values
  int64_t val; // value (sign for infinite values)
};


static_assert(std::is_class<obj_t>::value);
static_assert(std::is_trivial<obj_t>::value);
static_assert(std::is_standard_layout<obj_t>::value);
static_assert(std::is_trivially_copyable<obj_t>::value);
static_assert(!std::is_polymorphic<obj_t>::value);
static_assert(std::is_literal_type<obj_t>::value);
static_assert(std::is_pod<obj_t>::value);

static_assert(std::is_default_constructible<obj_t>::value);
static_assert(std::is_trivially_default_constructible<obj_t>::value);
static_assert(std::is_nothrow_default_constructible<obj_t>::value);
static_assert(std::is_copy_constructible<obj_t>::value);
static_assert(std::is_trivially_copy_constructible<obj_t>::value);
static_assert(std::is_nothrow_copy_constructible<obj_t>::value);
static_assert(std::is_move_constructible<obj_t>::value);
static_assert(std::is_trivially_move_constructible<obj_t>::value);
static_assert(std::is_nothrow_move_constructible<obj_t>::value);
static_assert(std::is_copy_assignable<obj_t>::value);
static_assert(std::is_trivially_copy_assignable<obj_t>::value);
static_assert(std::is_nothrow_copy_assignable<obj_t>::value);
static_assert(std::is_move_assignable<obj_t>::value);
static_assert(std::is_trivially_move_assignable<obj_t>::value);
static_assert(std::is_nothrow_move_assignable<obj_t>::value);
static_assert(std::is_destructible<obj_t>::value);
static_assert(std::is_trivially_destructible<obj_t>::value);
static_assert(std::is_nothrow_destructible<obj_t>::value);

// replacement for rand()
int my_rand();

#endif // POINT_HH
