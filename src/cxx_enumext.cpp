/*
 * Copyright (c) Rachel Powers.
 *
 * This source code is licensed under both the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree and the Apache
 * License, Version 2.0 found in the LICENSE-APACHE file in the root directory
 * of this source tree.
 */

// cxx_enumext/src/cxx_enumext.h

// This file incorporates work submitted as a pull request
// to the cxx github repository (https://github.com/dtolnay/cxx/pull/1328)
// and is covered by the following copyright and permission notice:
//
//  Copyright 2024 Dima Dorezyuk
//
//  Licensed under either of Apache License [1], Version 2.0 or MIT license [2]
//  at your option.
//
//  [1] https://github.com/dtolnay/cxx/blob/master/LICENSE-APACHE
//  [2] https://github.com/dtolnay/cxx/blob/master/LICENSE-MIT
//

#ifndef RUST_CXX_ENUMEXT_MACROS_H
#define RUST_CXX_ENUMEXT_MACROS_H

#include "rust/cxx_enumext.h"

namespace rust {
namespace enm {

/// @brief Below static_asserts for out variant.
///
/// We go quite overboard with the tests since the variant has some intricate
/// function resolution based on the types it can hold.
namespace detail {

/// @brief A type which can only be move-constructed/assigned but not copied.
///
/// If it's part of the variant's types the variant cannot not be copy
/// constructed. The variant can be constructed/assigned with an rvalue of this
/// type.
struct MoveType {
  MoveType() = default;
  MoveType(MoveType &&other) = default;
  MoveType(const MoveType &other) = delete;

  MoveType &operator=(const MoveType &other) = delete;
  MoveType &operator=(MoveType &&other) = default;
};

/// @brief A type which can only be copy-constructed/assigned but not moved.
///
/// If every type in the variant can be copy constructed, then the variant can
/// be copy constructed. The variant it can be constructed/assigned with a
/// lvalue of this type.
struct CopyType {
  CopyType() = default;
  CopyType(const CopyType &other) = default;
  CopyType(CopyType &&other) = delete;
  CopyType(int, std::string) {}

  CopyType &operator=(const CopyType &other) = default;
  CopyType &operator=(CopyType &&other) = delete;
};

/// @brief A type which can be copy and move constructed/assigned.
///
/// If every type in the variant can be copy constructed, then the variant can
/// be copy constructed. The variant it can be constructed/assigned with this
/// type.
struct CopyAndMoveType {
  CopyAndMoveType() = default;
  CopyAndMoveType(const CopyAndMoveType &other) = default;
  CopyAndMoveType(CopyAndMoveType &&other) = default;

  CopyAndMoveType &operator=(const CopyAndMoveType &other) = default;
  CopyAndMoveType &operator=(CopyAndMoveType &&other) = default;
};

/// @brief A type which can be neither copy nor move constructed/assigned.
///
/// If it's part of the variant's types the variant cannot not be copy
/// constructed. The variant can not constructed/assigned with any value of this
/// type - but you can emplace it.
struct NoCopyAndNoMoveType {
  NoCopyAndNoMoveType() = default;
  NoCopyAndNoMoveType(const NoCopyAndNoMoveType &other) = delete;
  NoCopyAndNoMoveType(NoCopyAndNoMoveType &&other) = delete;

  NoCopyAndNoMoveType &operator=(const NoCopyAndNoMoveType &other) = delete;
  NoCopyAndNoMoveType &operator=(NoCopyAndNoMoveType &&other) = delete;
};

// Checks with a variant containing all types.
using all_variant =
    variant<MoveType, CopyType, CopyAndMoveType, NoCopyAndNoMoveType>;

// Check of copy and move construction/assignment.
static_assert(!std::is_default_constructible_v<all_variant>);
static_assert(!std::is_constructible_v<all_variant, all_variant &&>);
static_assert(!std::is_constructible_v<all_variant, const all_variant &>);
static_assert(!std::is_copy_assignable_v<all_variant>);
static_assert(!std::is_move_assignable_v<all_variant>);

// Checks for converting construction/assignment. For every type we check
// construction by value, move, ref, const ref, in_place_type_t and
// in_place_index_t. The last two should always work.

// Checks for the first type (MoveType). We expect that everything requiring a
// copy will not work.
static_assert(std::is_constructible_v<all_variant, MoveType>);
static_assert(std::is_constructible_v<all_variant, MoveType &&>);
static_assert(!std::is_constructible_v<all_variant, MoveType &>);
static_assert(!std::is_constructible_v<all_variant, const MoveType &>);
static_assert(
    std::is_constructible_v<all_variant, std::in_place_type_t<MoveType>>);
static_assert(std::is_constructible_v<all_variant, std::in_place_index_t<0>>);

// Checks for the second type (CopyType). We expect that everything requiring a
// move will not work.
static_assert(!std::is_constructible_v<all_variant, CopyType>);
static_assert(!std::is_constructible_v<all_variant, CopyType &&>);
static_assert(std::is_constructible_v<all_variant, CopyType &>);
static_assert(std::is_constructible_v<all_variant, const CopyType &>);
static_assert(
    std::is_constructible_v<all_variant, std::in_place_type_t<CopyType>>);
static_assert(std::is_constructible_v<
              all_variant, std::in_place_type_t<CopyType>, int, std::string>);
static_assert(std::is_constructible_v<all_variant, std::in_place_index_t<1>>);

// Checks for the third type (CopyAndMoveType). We expect that everything works.
static_assert(std::is_constructible_v<all_variant, CopyAndMoveType>);
static_assert(std::is_constructible_v<all_variant, CopyAndMoveType &&>);
static_assert(std::is_constructible_v<all_variant, CopyAndMoveType &>);
static_assert(std::is_constructible_v<all_variant, const CopyAndMoveType &>);
static_assert(std::is_constructible_v<all_variant,
                                      std::in_place_type_t<CopyAndMoveType>>);
static_assert(std::is_constructible_v<all_variant, std::in_place_index_t<2>>);

// Checks for the fourth type (NoCopyAndNoMoveType). We expect that only
// in_place constructors work.
static_assert(!std::is_constructible_v<all_variant, NoCopyAndNoMoveType>);
static_assert(!std::is_constructible_v<all_variant, NoCopyAndNoMoveType &&>);
static_assert(!std::is_constructible_v<all_variant, NoCopyAndNoMoveType &>);
static_assert(
    !std::is_constructible_v<all_variant, const NoCopyAndNoMoveType &>);
static_assert(std::is_constructible_v<
              all_variant, std::in_place_type_t<NoCopyAndNoMoveType>>);
static_assert(std::is_constructible_v<all_variant, std::in_place_index_t<3>>);

// Checks with invalid input.
static_assert(!std::is_constructible_v<all_variant, std::in_place_index_t<4>>);
static_assert(!std::is_constructible_v<all_variant, std::in_place_type_t<int>>);

// Checks with the construction from std::variant. We can't move and we can't
// copy the types - therefore there is no safe way to construct our variant
// from std::variant.
using std_all_variant =
    std::variant<MoveType, CopyType, CopyAndMoveType, NoCopyAndNoMoveType>;

static_assert(!std::is_constructible_v<all_variant, const std_all_variant &>);
static_assert(!std::is_constructible_v<all_variant, std_all_variant &&>);
static_assert(!std::is_assignable_v<all_variant, const std_all_variant &>);
static_assert(!std::is_assignable_v<all_variant, std_all_variant &&>);

// Checks with a variant consisting of only movable types.
using move_variant = variant<MoveType, CopyAndMoveType>;

// Check of copy and move construction/assignment. Since we disallow move
// constructors/assignments none of the copy and move constructors should work.
static_assert(!std::is_default_constructible_v<move_variant>);
static_assert(!std::is_constructible_v<move_variant, move_variant &&>);
static_assert(!std::is_constructible_v<move_variant, const move_variant &>);
static_assert(!std::is_copy_assignable_v<move_variant>);
static_assert(!std::is_move_assignable_v<move_variant>);

// Checks with the construction from std::variant. Since we can move every type
// we should be able to move construct from std::variant. Copy constructors
// should not work since MoveType has no copy constructor.
using std_move_variant = std::variant<MoveType, CopyAndMoveType>;
static_assert(!std::is_constructible_v<move_variant, const std_move_variant &>);
static_assert(std::is_constructible_v<move_variant, std_move_variant &&>);
static_assert(!std::is_assignable_v<move_variant, const std_move_variant &>);
static_assert(std::is_assignable_v<move_variant, std_move_variant &&>);

// Checks with a variant consisting of only copyable types.
using copy_variant = variant<CopyType, CopyAndMoveType>;

// Check of copy and move construction/assignment. Copy constructor/assignment
// should work since every type can be copy constructed/assigned.
static_assert(!std::is_default_constructible_v<copy_variant>);
static_assert(!std::is_constructible_v<copy_variant, copy_variant &&>);
static_assert(std::is_constructible_v<copy_variant, const copy_variant &>);
static_assert(std::is_copy_assignable_v<copy_variant>);
static_assert(!std::is_move_assignable_v<copy_variant>);

// Checks with the construction from std::variant. Since we can copy every type
// we should be able to copy construct from std::variant. Move constructors
// should not work since CopyType has no move constructor.
using std_copy_variant = std::variant<CopyType, CopyAndMoveType>;
static_assert(std::is_constructible_v<copy_variant, const std_copy_variant &>);
static_assert(std::is_constructible_v<copy_variant, std_copy_variant &&>);
static_assert(std::is_assignable_v<copy_variant, const std_copy_variant &>);
static_assert(std::is_assignable_v<copy_variant, std_copy_variant &&>);

// Checks with a variant containing duplicate types: Constructors where just
// the type is specified should not work since there is no way of telling which
// index to use.
using duplicate_variant = variant<int, int>;
static_assert(!std::is_constructible_v<duplicate_variant, int>);
static_assert(!std::is_constructible_v<duplicate_variant,
                                       std::in_place_type_t<int>, int>);
static_assert(
    std::is_constructible_v<duplicate_variant, std::in_place_index_t<0>, int>);
static_assert(
    std::is_constructible_v<duplicate_variant, std::in_place_index_t<1>, int>);
static_assert(
    !std::is_constructible_v<duplicate_variant, std::in_place_index_t<2>, int>);

// We're using the std::reference_wrapper if the enum holds a reference. The
// standard however does not guarantee that the size of std::reference_wrapper
// is the same size as a pointer (required to be compatible with Rust's
// references). Therefore we check it at compile time.
//
// [1] https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper
// [2] https://doc.rust-lang.org/std/mem/fn.size_of.html#examples
static_assert(sizeof(std::reference_wrapper<all_variant>) ==
              sizeof(std::ptrdiff_t));

// Check that getting something works and actually returns the same type.
template <std::size_t I>
using copy_variant_alternative_t =
    variant_alternative_t<I, CopyType, CopyAndMoveType>;

static_assert(std::is_same_v<decltype(get<0>(std::declval<copy_variant>())),
                             const copy_variant_alternative_t<0> &>);

static_assert(std::is_same_v<decltype(get<1>(std::declval<copy_variant>())),
                             const copy_variant_alternative_t<1> &>);

static_assert(sizeof(monostate) == sizeof(std::uint8_t));

// Verify that enums are represented as ints. We kind of assume that the enums
// with more fields would be represented by the underlying type not smaller
// than this enum (which is our smallest...). If this fails, we would need
// to pass the enum-type to the variant.
enum a_enum { AA };

static_assert(sizeof(std::underlying_type_t<a_enum>) == sizeof(int));
} // namespace detail

} // namespace enm
} // namespace rust

#endif
