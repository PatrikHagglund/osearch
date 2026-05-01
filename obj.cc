#include "obj.hh"
#include "assume.hh"

/// \file
/// For obj_t (objective function results).

static_assert(std::is_class<obj_t>::value);
static_assert(std::is_standard_layout<obj_t>::value);
static_assert(std::is_trivially_copyable<obj_t>::value);
static_assert(!std::is_polymorphic<obj_t>::value);
// static_assert(std::is_literal_type<obj_t>::value);

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
