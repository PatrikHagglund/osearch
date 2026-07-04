/// \file
/// Compile-time tests for obj_t (see obj.hh).
///
/// Failures here are build errors. The runtime main() is a no-op and only
/// exists so CTest can register the file as a test; the real "test" is
/// that the translation unit compiles.

#include "obj.hh"

// --- construction & default value ---
static_assert(obj_t{}.is_finite());
static_assert(obj_t{42}.is_finite());
static_assert(!obj_t_inf.is_finite());

// --- equality ---
static_assert(obj_t{10} == obj_t{10});
static_assert(!(obj_t{10} == obj_t{11}));
static_assert(obj_t{} == obj_t{0});
static_assert(obj_t_inf == obj_t_inf);
static_assert(!(obj_t{0} == obj_t_inf));

// --- total order among finites ---
static_assert(obj_t{5} < obj_t{10});
static_assert(!(obj_t{10} < obj_t{5}));
static_assert(!(obj_t{5} < obj_t{5}));
static_assert(obj_t{10} > obj_t{5});
static_assert(obj_t{5} <= obj_t{5});
static_assert(obj_t{5} <= obj_t{10});
static_assert(obj_t{10} >= obj_t{5});
static_assert(obj_t{-5} < obj_t{0});

// --- total order: finite < inf ---
static_assert(obj_t{1'000'000} < obj_t_inf);
static_assert(obj_t{0} < obj_t_inf);
static_assert(obj_t{-1'000'000} < obj_t_inf);
static_assert(obj_t_inf > obj_t{1'000'000});
static_assert(!(obj_t_inf < obj_t_inf));
static_assert(obj_t_inf >= obj_t_inf);
static_assert(obj_t_inf <= obj_t_inf);

// --- arithmetic (finite only) ---
static_assert((obj_t{10} - obj_t{3}) == obj_t{7});
static_assert((obj_t{0} - obj_t{0}) == obj_t{0});
static_assert((obj_t{-5} - obj_t{-2}) == obj_t{-3});
static_assert(-obj_t{7} == obj_t{-7});
static_assert(-obj_t{0} == obj_t{0});

// --- type traits (mirrors obj.hh; kept so this TU is self-contained) ---
static_assert(std::is_trivially_copyable_v<obj_t>);
static_assert(std::is_standard_layout_v<obj_t>);

int main() { return 0; }
