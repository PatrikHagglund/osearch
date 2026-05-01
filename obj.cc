#include "obj.hh"
#include "assume.hh"

/// \file
/// For obj_t (objective function results).

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
