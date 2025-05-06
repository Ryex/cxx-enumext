/*
 * Copyright (c) Rachel Powers.
 *
 * This source code is licensed under both the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree and the Apache
 * License, Version 2.0 found in the LICENSE-APACHE file in the root directory
 * of this source tree.
 */

// cxx_enumext/include/rust/cxx_enumext_macros.h

#pragma once

#define CXX_CAT(a, ...) CXX_PRIMITIVE_CAT(a, __VA_ARGS__)
#define CXX_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define CXX_CALL(X, Y) X Y

#define CXX_EAT(...)
#define CXX_EMPTY()
#define CXX_DEFER(id) id CXX_EMPTY()
#define CXX_OBSTRUCT(...) __VA_ARGS__ CXX_DEFER(CXX_EMPTY)()
#define CXX_EXPAND(...) __VA_ARGS__

#define CXX_EVAL(...) CXX_EVAL1(CXX_EVAL1(CXX_EVAL1(__VA_ARGS__)))
#define CXX_EVAL1(...) CXX_EVAL2(CXX_EVAL2(CXX_EVAL2(__VA_ARGS__)))
#define CXX_EVAL2(...) CXX_EVAL3(CXX_EVAL3(CXX_EVAL3(__VA_ARGS__)))
#define CXX_EVAL3(...) CXX_EVAL4(CXX_EVAL4(CXX_EVAL4(__VA_ARGS__)))
#define CXX_EVAL4(...) __VA_ARGS__

#define CXX_GET_ARG_22(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, \
                       a14, a15, a17, a18, a19, a20, a21, a22, n, ...)         \
  n
#define CXX_GET_ARG_N(n, ...)                                                  \
  CXX_CALL(CXX_TOKENPASTE(CXX_GET_ARG_, n), (__VA_ARGS__))
#define CXX_COUNT_ARGS(...)                                                    \
  CXX_EXPAND(CXX_GET_ARG_22("ignored", __VA_ARGS__, 20, 19, 18, 17, 16, 15,    \
                            14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// Concatenates `prefix` to the number of variadic arguments supplied (e.g.
// `prefix_1`, `prefix_2`, etc.)
#define CXX_DISPATCH_VARIADIC_IMPL(prefix, unused, a0, a1, a2, a3, a4, a5, a6, \
                                   a7, a8, a9, a10, a11, a12, a13, a14, a15,   \
                                   a16, a17, a18, a19, a20, ...)               \
  prefix##a20
#define CXX_DISPATCH_VARIADIC(prefix, ...)                                     \
  CXX_DEFER(CXX_DISPATCH_VARIADIC_IMPL)(prefix, __VA_ARGS__, 21, 20, 19, 18,   \
                                        17, 16, 15, 14, 13, 12, 11, 10, 9, 8,  \
                                        7, 6, 5, 4, 3, 2, 1)

#define CXX_LIST_WRAP_WITH_0(macro, arg, ...)
#define CXX_LIST_WRAP_WITH_1(macro, arg, var, ...) macro(var, arg)
#define CXX_LIST_WRAP_WITH_2(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_1)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_3(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_2)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_4(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_3)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_5(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_4)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_6(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_5)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_7(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_6)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_8(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_7)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_9(macro, arg, var, ...)                             \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_8)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_10(macro, arg, var, ...)                            \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_9)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_11(macro, arg, var, ...)                            \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_10)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_12(macro, arg, var, ...)                            \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_11)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_13(macro, arg, var, ...)                            \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_12)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_14(macro, arg, var, ...)                            \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_13)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH_15(macro, arg, var, ...)                            \
  macro(var, arg), CXX_DEFER(CXX_LIST_WRAP_WITH_14)(macro, arg, __VA_ARGS__)
#define CXX_LIST_WRAP_WITH(macro, arg, ...)                                    \
  CXX_DISPATCH_VARIADIC(CXX_LIST_WRAP_WITH_, __VA_ARGS__)(macro, arg,          \
                                                          __VA_ARGS__)

#define CXX_LIST_APPLY_0(macro)
#define CXX_LIST_APPLY_1(macro, var) macro(var)
#define CXX_LIST_APPLY_2(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_1)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_3(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_2)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_4(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_3)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_5(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_4)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_6(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_5)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_7(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_6)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_8(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_7)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_9(macro, var, ...)                                      \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_8)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_10(macro, var, ...)                                     \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_9)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_11(macro, var, ...)                                     \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_10)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_12(macro, var, ...)                                     \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_11)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_13(macro, var, ...)                                     \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_12)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_14(macro, var, ...)                                     \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_13)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_15(macro, var, ...)                                     \
  macro(var) CXX_DEFER(CXX_LIST_APPLY_14)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY(macro, ...)                                             \
  CXX_DISPATCH_VARIADIC(CXX_LIST_APPLY_, __VA_ARGS__)(macro, __VA_ARGS__)

#define CXX_LIST_APPLY_WITH_0(macro, arg)
#define CXX_LIST_APPLY_WITH_1(macro, arg, var) macro(var, arg)
#define CXX_LIST_APPLY_WITH_2(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_1)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_3(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_2)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_4(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_3)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_5(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_4)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_6(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_5)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_7(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_6)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_8(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_7)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_9(macro, arg, var, ...)                            \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_8)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_10(macro, arg, var, ...)                           \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_9)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_11(macro, arg, var, ...)                           \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_10)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_12(macro, arg, var, ...)                           \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_11)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_13(macro, arg, var, ...)                           \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_12)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_14(macro, arg, var, ...)                           \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_13)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH_15(macro, arg, var, ...)                           \
  macro(var, arg) CXX_DEFER(CXX_LIST_APPLY_WITH_14)(macro, arg, __VA_ARGS__)
#define CXX_LIST_APPLY_WITH(macro, arg, ...)                                   \
  CXX_DISPATCH_VARIADIC(CXX_LIST_APPLY_WITH_, __VA_ARGS__)(macro, arg,         \
                                                           __VA_ARGS__)

#define CXX_LIST_APPLY_INDEX_0(macro, var, ...)
#define CXX_LIST_APPLY_INDEX_1(macro, var, ...) macro(var, 0)
#define CXX_LIST_APPLY_INDEX_2(macro, var, ...)                                \
  macro(var, 1) CXX_DEFER(CXX_LIST_APPLY_INDEX_1)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_3(macro, var, ...)                                \
  macro(var, 2) CXX_DEFER(CXX_LIST_APPLY_INDEX_2)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_4(macro, var, ...)                                \
  macro(var, 3) CXX_DEFER(CXX_LIST_APPLY_INDEX_3)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_5(macro, var, ...)                                \
  macro(var, 4) CXX_DEFER(CXX_LIST_APPLY_INDEX_4)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_6(macro, var, ...)                                \
  macro(var, 5) CXX_DEFER(CXX_LIST_APPLY_INDEX_5)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_7(macro, var, ...)                                \
  macro(var, 6) CXX_DEFER(CXX_LIST_APPLY_INDEX_6)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_8(macro, var, ...)                                \
  macro(var, 7) CXX_DEFER(CXX_LIST_APPLY_INDEX_7)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_9(macro, var, ...)                                \
  macro(var, 8) CXX_DEFER(CXX_LIST_APPLY_INDEX_8)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_10(macro, var, ...)                               \
  macro(var, 9) CXX_DEFER(CXX_LIST_APPLY_INDEX_9)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_11(macro, var, ...)                               \
  macro(var, 10) CXX_DEFER(CXX_LIST_APPLY_INDEX_10)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_12(macro, var, ...)                               \
  macro(var, 11) CXX_DEFER(CXX_LIST_APPLY_INDEX_11)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_13(macro, var, ...)                               \
  macro(var, 12) CXX_DEFER(CXX_LIST_APPLY_INDEX_12)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_14(macro, var, ...)                               \
  macro(var, 13) CXX_DEFER(CXX_LIST_APPLY_INDEX_13)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX_15(macro, var, ...)                               \
  macro(var, 14) CXX_DEFER(CXX_LIST_APPLY_INDEX_14)(macro, __VA_ARGS__)
#define CXX_LIST_APPLY_INDEX(macro, ...)                                       \
  CXX_DISPATCH_VARIADIC(CXX_LIST_APPLY_INDEX_, __VA_ARGS__)(macro, __VA_ARGS__)

#define CXX_LIST_APPLY_INDEX_REV_0(macro, var, ...)
#define CXX_LIST_APPLY_INDEX_REV_1(macro, var, ...) macro(var, 0)
#define CXX_LIST_APPLY_INDEX_REV_2(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_1)(macro, __VA_ARGS__) macro(var, 1)
#define CXX_LIST_APPLY_INDEX_REV_3(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_2)(macro, __VA_ARGS__) macro(var, 2)
#define CXX_LIST_APPLY_INDEX_REV_4(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_3)(macro, __VA_ARGS__) macro(var, 3)
#define CXX_LIST_APPLY_INDEX_REV_5(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_4)(macro, __VA_ARGS__) macro(var, 4)
#define CXX_LIST_APPLY_INDEX_REV_6(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_5)(macro, __VA_ARGS__) macro(var, 5)
#define CXX_LIST_APPLY_INDEX_REV_7(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_6)(macro, __VA_ARGS__) macro(var, 6)
#define CXX_LIST_APPLY_INDEX_REV_8(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_7)(macro, __VA_ARGS__) macro(var, 7)
#define CXX_LIST_APPLY_INDEX_REV_9(macro, var, ...)                            \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_8)(macro, __VA_ARGS__) macro(var, 8)
#define CXX_LIST_APPLY_INDEX_REV_10(macro, var, ...)                           \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_9)(macro, __VA_ARGS__) macro(var, 9)
#define CXX_LIST_APPLY_INDEX_REV_11(macro, var, ...)                           \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_10)(macro, __VA_ARGS__) macro(var, 10)
#define CXX_LIST_APPLY_INDEX_REV_12(macro, var, ...)                           \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_11)(macro, __VA_ARGS__) macro(var, 11)
#define CXX_LIST_APPLY_INDEX_REV_13(macro, var, ...)                           \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_12)(macro, __VA_ARGS__) macro(var, 12)
#define CXX_LIST_APPLY_INDEX_REV_14(macro, var, ...)                           \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_13)(macro, __VA_ARGS__) macro(var, 13)
#define CXX_LIST_APPLY_INDEX_REV_15(macro, var, ...)                           \
  CXX_DEFER(CXX_LIST_APPLY_INDEX_REV_14)(macro, __VA_ARGS__) macro(var, 14)
#define CXX_LIST_APPLY_INDEX_REV(macro, ...)                                   \
  CXX_DISPATCH_VARIADIC(CXX_LIST_APPLY_INDEX_REV_, __VA_ARGS__)(macro,         \
                                                                __VA_ARGS__)

#define CXX_APPLY(macro, ...) macro(__VA_ARGS__)

#define CXX_VARIANT_TYPE_FROM_DEF(name, type, impl) type
#define CXX_VARIANT_NAME_FROM_DEF(name, type, impl) name
#define CXX_VARIANT_IMPL_FROM_DEF(name, type, impl) impl

#define CXX_VARIANT_COLLECT_TYPE(def, variant)                                 \
  variant##_impl::CXX_DEFER(CXX_VARIANT_TYPE_FROM_DEF)(                        \
      CXX_EXPAND(CXX_VARIANT_##def))

#define CXX_VARIANT_COLLECT_IMPL(def)                                          \
  CXX_DEFER(CXX_VARIANT_IMPL_FROM_DEF)                                         \
  CXX_DEFER(CXX_EXPAND)((CXX_VARIANT_##def))

#define CXX_VARIANT_USING(def, variant)                                        \
  using CXX_DEFER(CXX_VARIANT_NAME_FROM_DEF)(CXX_EXPAND(CXX_VARIANT_##def)) =  \
      variant##_impl::CXX_DEFER(CXX_VARIANT_TYPE_FROM_DEF)(                    \
          CXX_EXPAND(CXX_VARIANT_##def));

///=====================
/// Variant Type macros
/// ====================

/// Type (single element tuple)
#define CXX_VARIANT_TYPE(name, type) name, name##_t, using name##_t = type;

/// N element Tuple (supports up to 15 fields)
#define CXX_DEFINE_TUPLE_FIELD(type, index) type _##index;
#define CXX_VARIANT_TUPLE(name, ...)                                           \
  name, name##_t, struct name##_t {                                            \
    CXX_DEFER(CXX_LIST_APPLY_INDEX_REV)                                        \
    (CXX_DEFINE_TUPLE_FIELD, __VA_ARGS__)                                      \
  };

#define CXX_VARIANT_UNIT(name)                                                 \
  name, name##_t, struct name##_t {};

#define CXX_VARIANT_STRUCT(name, ...)                                          \
  name, name##_t, struct name##_t {                                            \
    __VA_ARGS__                                                                \
  };

#define CXX_IMPL_NAMESPACE(...)                                                \
  CXX_LIST_APPLY(CXX_VARIANT_COLLECT_IMPL, __VA_ARGS__)

#define CXX_VARIANT_TYPE_PACK(name, ...)                                       \
  CXX_LIST_WRAP_WITH(CXX_VARIANT_COLLECT_TYPE, name, __VA_ARGS__)

#define CXX_VARIANT_USING_STATMENTS(name, ...)                                 \
  CXX_LIST_APPLY_WITH(CXX_VARIANT_USING, name, __VA_ARGS__)

///=====================
/// Variant Define macro
/// ====================

#define CXX_DEFINE_VARIANT(name, variants, ...)                                \
  namespace name##_impl {                                                      \
    CXX_EVAL(CXX_CALL(CXX_IMPL_NAMESPACE, variants))                           \
  }                                                                            \
  struct name final                                                            \
      : public ::rust::enm::variant<CXX_EVAL(CXX_CALL(                     \
            CXX_VARIANT_TYPE_PACK, (name, CXX_DEFER(CXX_EXPAND) variants)))> { \
    using base = ::rust::enm::variant<CXX_EVAL(CXX_CALL(                   \
        CXX_VARIANT_TYPE_PACK, (name, CXX_DEFER(CXX_EXPAND) variants)))>;      \
                                                                               \
    name() = delete;                                                           \
    name(const name &) = default;                                              \
    name(name &&) = delete;                                                    \
    using base::base;                                                          \
    using base::operator=;                                                     \
                                                                               \
    using IsRelocatable = std::true_type;                                      \
                                                                               \
    CXX_EVAL(CXX_DEFER(CXX_VARIANT_USING_STATMENTS)(name,                      \
                                                    CXX_DEFER(CXX_EXPAND)      \
                                                        variants))             \
    __VA_ARGS__                                                                \
  };

#define CXX_DEFINE_OPTIONAL(name, type, ...)                                   \
  struct name final : public ::rust::enm::optional<type> {                 \
    using base = ::rust::enm::optional<type>;                              \
    using base::base;                                                          \
    using base::operator=;                                                     \
                                                                               \
    using base::Some;                                                          \
    using base::None;                                                          \
                                                                               \
    using IsRelocatable = std::true_type;                                      \
                                                                               \
    __VA_ARGS__                                                                \
  };

#define CXX_DEFINE_EXPECTED(name, expected_t, unexpected_t, ...)               \
  struct name final                                                            \
      : public ::rust::enm::expected<expected_t, unexpected_t> {           \
    using base = ::rust::enm::expected<expected_t, unexpected_t>;          \
    using base::base;                                                          \
    using base::operator=;                                                     \
                                                                               \
    using base::Ok;                                                            \
    using base::Err;                                                           \
                                                                               \
    using IsRelocatable = std::true_type;                                      \
                                                                               \
    __VA_ARGS__                                                                \
  };
