/*
 * Copyright (c) Rachel Powers.
 *
 * This source code is licensed under both the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree and the Apache
 * License, Version 2.0 found in the LICENSE-APACHE file in the root directory
 * of this source tree.
 */

// cxx_enumext/include/rust/cxx_enumext.h

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

#ifndef RUST_CXX_ENUMEXT_H
#define RUST_CXX_ENUMEXT_H

#include <algorithm>
#include <cstdint> // IWYU pragma: keep
#include <cstring>
#include <functional>
#include <memory> // IWYU pragma: keep
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// If you're using enums and variants on windows, you need to pass also
// `/Zc:__cplusplus` as a compiler to make __cplusplus work correctly. If users
// ever report that they have a too old compiler to `/Zc:__cplusplus` we may
// fallback to the `_MSVC_LANG` define.
//
// Sources:
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
// https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-170
#include <variant>

namespace rust {
namespace enm {

// Adjusted from std::variant_alternative. Standard selects always the most
// specialized template specialization. See
// https://timsong-cpp.github.io/cppwp/n4140/temp.class.spec.match and
// https://timsong-cpp.github.io/cppwp/n4140/temp.class.order.
template <std::size_t I, typename... Ts> struct variant_alternative;

// Specialization for gracefully handling invalid indices.
template <std::size_t I> struct variant_alternative<I> {};

template <std::size_t I, typename First, typename... Remainder>
struct variant_alternative<I, First, Remainder...>
    : variant_alternative<I - 1, Remainder...> {};

template <typename First, typename... Remainder>
struct variant_alternative<0, First, Remainder...> {
  using type = First;
};

template <std::size_t I, typename... Ts>
using variant_alternative_t = typename variant_alternative<I, Ts...>::type;

template <bool... Values> constexpr size_t compile_time_count() noexcept {
  return 0 + (static_cast<std::size_t>(Values) + ...);
}

template <bool... Values>
struct count
    : std::integral_constant<std::size_t, compile_time_count<Values...>()> {};

template <bool... Values>
struct exactly_once : std::conditional_t<count<Values...>::value == 1,
                                         std::true_type, std::false_type> {};

template <std::size_t I, bool... Values> struct index_from_booleans;

template <std::size_t I> struct index_from_booleans<I> {};

template <std::size_t I, bool First, bool... Remainder>
struct index_from_booleans<I, First, Remainder...>
    : index_from_booleans<I + 1, Remainder...> {};

template <std::size_t I, bool... Remainder>
struct index_from_booleans<I, true, Remainder...>
    : std::integral_constant<std::size_t, I> {};

template <typename Type, typename... Ts>
struct index_from_type
    : index_from_booleans<0, std::is_same_v<std::decay_t<Type>, Ts>...> {
  static_assert(exactly_once<std::is_same_v<std::decay_t<Type>, Ts>...>::value,
                "Index must be unique");
};

template <typename... Ts> struct visitor_type;

template <typename... Ts> struct variant_base;

template <std::size_t I, typename... Ts>
constexpr decltype(auto) get(variant_base<Ts...> &);

template <std::size_t I, typename... Ts>
constexpr decltype(auto) get(const variant_base<Ts...> &);

template <std::size_t I, typename... Ts>
constexpr std::add_pointer_t<variant_alternative_t<I, Ts...>>
get_if(variant_base<Ts...> *variant);

template <std::size_t I, typename... Ts>
constexpr const std::add_pointer_t<variant_alternative_t<I, Ts...>>
get_if(const variant_base<Ts...> *variant);

template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor &&visitor, variant_base<Ts...> &);

template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor &&visitor, const variant_base<Ts...> &);

/// @brief A std::variant like tagged union with the same memory layout as a
/// Rust Enum.
///
/// The memory layout of the Rust enum is defined under
/// https://doc.rust-lang.org/reference/type-layout.html#reprc-enums-with-fields
template <typename... Ts> struct variant_base {
  static_assert(sizeof...(Ts) > 0,
                "variant_base must hold at least one alternative");

  /// @brief Delete the default constructor since we cannot be in an
  /// uninitialized state (if the first alternative throws). Corresponds to
  /// the (1) constructor in std::variant.
  variant_base() = delete;

  constexpr static bool all_copy_constructible_v =
      std::conjunction_v<std::is_copy_constructible<Ts>...>;

  /// @brief Copy constructor. Participates only in the resolution if all
  /// types are copy constructable. Corresponds to (2) constructor of
  /// std::variant.
  variant_base(const variant_base &other) {
    static_assert(
        all_copy_constructible_v,
        "Copy constructor requires that all types are copy constructable");

    m_Index = other.m_Index;
    visit(
        [this](const auto &value) {
          using type = std::decay_t<decltype(value)>;
          new (static_cast<void *>(m_Buff)) type(value);
        },
        other);
  };

  /// @brief Delete the move constructor since if we move this container it's
  /// unclear in which state it is. Corresponds to (3) constructor of
  /// std::variant.
  variant_base(variant_base &&other) = delete;

  template <typename T>
  constexpr static bool is_unique_v =
      exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value;

  template <typename T>
  constexpr static std::size_t index_from_type_v =
      index_from_type<T, Ts...>::value;

  /// @brief Converting constructor. Corresponds to (4) constructor of
  /// std::variant.
  template <typename T, typename D = std::decay_t<T>,
            typename = std::enable_if_t<is_unique_v<T> &&
                                        std::is_constructible_v<D, T>>>
  variant_base(T &&other) noexcept(std::is_nothrow_constructible_v<D, T>) {
    m_Index = index_from_type_v<D>;
    new (static_cast<void *>(m_Buff)) D(std::forward<T>(other));
  }

  /// @brief Participates in the resolution only if we can construct T from
  /// Args and if T is unique in Ts. Corresponds to (5) constructor of
  /// std::variant.
  template <typename T, typename... Args,
            typename = std::enable_if_t<is_unique_v<T>>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  explicit variant_base(
      [[maybe_unused]] std::in_place_type_t<T> type,
      Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : variant_base{std::in_place_index<index_from_type_v<T>>,
                     std::forward<Args>(args)...} {}

  template <std::size_t I>
  using type_from_index_t = variant_alternative_t<I, Ts...>;

  /// @brief Participates in the resolution only if the index is within range
  /// and if the type can be constructor from Args. Corresponds to (7) of
  /// std::variant.
  template <std::size_t I, typename... Args, typename T = type_from_index_t<I>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  explicit variant_base(
      [[maybe_unused]] std::in_place_index_t<I> index,
      Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    m_Index = I;
    new (static_cast<void *>(m_Buff)) T(std::forward<Args>(args)...);
  }

  template <typename... Rs>
  constexpr static bool all_same_v =
      std::conjunction_v<std::is_same<Rs, Ts>...>;

  /// @brief Converts the std::variant to our variant. Participates only in
  /// the resolution if all types in Ts are copy constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_copy_constructible_v>>
  variant_base(const std::variant<Rs...> &other) {
    m_Index = other.index();
    std::visit(
        [this](const auto &value) {
          using type = std::decay_t<decltype(value)>;
          new (static_cast<void *>(m_Buff)) type(value);
        },
        other);
  }

  constexpr static bool all_move_constructible_v =
      std::conjunction_v<std::is_move_constructible<Ts>...>;

  /// @brief Converts the std::variant to our variant. Participates only in
  /// the resolution if all types in Ts are move constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_move_constructible_v>>
  variant_base(std::variant<Rs...> &&other) {
    m_Index = other.index();
    std::visit(
        [this](auto &&value) {
          using type = std::decay_t<decltype(value)>;
          new (static_cast<void *>(m_Buff)) type(std::move(value));
        },
        other);
  }

  ~variant_base() { destroy(); }

  /// @brief Copy assignment. Statically fails if not every type in Ts is copy
  /// constructable. Corresponds to (1) assignment of std::variant.
  variant_base &operator=(const variant_base &other) {
    static_assert(
        all_copy_constructible_v,
        "Copy assignment requires that all types are copy constructable");

    visit([this](const auto &value) { *this = value; }, other);

    return *this;
  };

  /// @brief Deleted move assignment. Same as for the move constructor.
  /// Would correspond to (2) assignment of std::variant.
  variant_base &operator=(variant_base &&other) = delete;

  /// @brief Converting assignment. Corresponds to (3) assignment of
  /// std::variant.
  template <typename T, typename = std::enable_if_t<
                            is_unique_v<T> && std::is_constructible_v<T &&, T>>>
  variant_base &operator=(T &&other) {
    constexpr auto index = index_from_type_v<T>;

    if (m_Index == index) {
      if constexpr (std::is_nothrow_assignable_v<T, T &&>) {
        get<index>(*this) = std::forward<T>(other);
        return *this;
      }
    }
    this->emplace<std::decay_t<T>>(std::forward<T>(other));
    return *this;
  }

  /// @brief Converting assignment from std::variant. Participates only in the
  /// resolution if all types in Ts are copy constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_copy_constructible_v>>
  variant_base &operator=(const std::variant<Rs...> &other) {
    // TODO this is not really clean since we fail if std::variant has
    // duplicated types.
    std::visit(
        [this](const auto &value) {
          using type = decltype(value);
          emplace<std::decay_t<type>>(value);
        },
        other);
    return *this;
  }

  /// @brief Converting assignment from std::variant. Participates only in the
  /// resolution if all types in Ts are move constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_move_constructible_v>>
  variant_base &operator=(std::variant<Rs...> &&other) {
    // TODO this is not really clean since we fail if std::variant has
    // duplicated types.
    std::visit(
        [this](auto &&value) {
          using type = decltype(value);
          emplace<std::decay_t<type>>(std::move(value));
        },
        other);
    return *this;
  }

  /// @brief Emplace function. Participates in the resolution only if T is
  /// unique in Ts and if T can be constructed from Args. Offers strong
  /// exception guarantee. Corresponds to the (1) emplace function of
  /// std::variant.
  template <typename T, typename... Args,
            typename = std::enable_if_t<is_unique_v<T>>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  T &emplace(Args &&...args) {
    constexpr std::size_t index = index_from_type_v<T>;
    return this->emplace<index>(std::forward<Args>(args)...);
  }

  /// @brief Emplace function. Participates in the resolution only if T can be
  /// constructed from Args. Offers strong exception guarantee. Corresponds to
  /// the (2) emplace function of std::variant.
  ///
  /// The std::variant can have no valid state if the type throws during the
  /// construction. This is represented by the `valueless_by_exception` flag.
  /// The same approach is also used in absl::variant [2].
  /// In our case we can't accept valueless enums since we can't represent
  /// this in Rust. We must therefore provide a strong exception guarantee for
  /// all operations using `emplace`. Two famous implementations of never
  /// valueless variants are Boost/variant [3] and Boost/variant2 [4].
  /// Boost/variant2 uses two memory buffers - which would be not compatible
  /// with Rust Enum's memory layout. The Boost/variant backs up the old
  /// object and calls its d'tor before constructing the new object; It then
  /// copies the old data back to the buffer if the construction fails - which
  /// might contain garbage (since) the d'tor was already called.
  ///
  ///
  /// We take a similar approach to Boost/variant. Assuming that constructing
  /// or moving the new type can throw, we backup the old data, try to
  /// construct the new object in the final buffer, swap the buffers, such
  /// that the old object is back in its original place, destroy it and move
  /// the new object from the old buffer back to the final place.
  ///
  /// Sources
  ///
  /// [1]
  /// https://en.cppreference.com/w/cpp/utility/variant/valueless_by_exception
  /// [2]
  /// https://github.com/abseil/abseil-cpp/blob/master/absl/types/variant.h
  /// [3]
  /// https://www.boost.org/doc/libs/1_84_0/libs/variant2/doc/html/variant2.html
  /// [4]
  /// https://www.boost.org/doc/libs/1_84_0/doc/html/variant/design.html#variant.design.never-empty
  template <std::size_t I, typename... Args, typename T = type_from_index_t<I>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  T &emplace(Args &&...args) {
    if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
      destroy();
      new (static_cast<void *>(m_Buff)) T(std::forward<Args>(args)...);
    } else if constexpr (std::is_nothrow_move_constructible_v<T>) {
      // This operation may throw, but we know that the move does not.
      const T tmp{std::forward<Args>(args)...};

      // The operations below are safe.
      destroy();
      new (static_cast<void *>(m_Buff)) T(std::move(tmp));
    } else {
      // Backup the old data.
      alignas(Ts...) std::byte old_buff[std::max({sizeof(Ts)...})];
      std::memcpy(old_buff, m_Buff, sizeof(m_Buff));

      try {
        // Try to construct the new object
        new (static_cast<void *>(m_Buff)) T(std::forward<Args>(args)...);
      } catch (...) {
        // Restore the old buffer
        std::memcpy(m_Buff, old_buff, sizeof(m_Buff));
        throw;
      }
      // Fetch the old buffer and destroy it.
      std::swap_ranges(m_Buff, m_Buff + sizeof(m_Buff), old_buff);

      destroy();
      std::memcpy(m_Buff, old_buff, sizeof(m_Buff));
    }

    m_Index = I;
    return get<I>(*this);
  }

  constexpr std::size_t index() const noexcept { return m_Index; }
  void swap(variant_base &other) {
    // swap respective buffers, they should be the same size as they are the
    // same type
    std::swap_ranges(m_Buff, m_Buff + sizeof(m_Buff), other.m_Buff);
    // swap the index
    std::swap(m_Index, other.m_Index);
    // variants should be swaped
  }

  struct bad_rust_variant_access : std::runtime_error {
    bad_rust_variant_access(std::size_t index)
        : std::runtime_error{"The index should be " + std::to_string(index)} {}
  };

protected:
  template <std::size_t I> void throw_if_invalid() const {
    static_assert(I < (sizeof...(Ts)), "Invalid index");

    if (m_Index != I)
      throw bad_rust_variant_access(m_Index);
  }

  template <std::size_t I> bool is_valid() const {
    static_assert(I < (sizeof...(Ts)), "Invalid index");
    return m_Index == I;
  }

  void destroy() {
    visit(
        [](const auto &value) {
          using type = std::decay_t<decltype(value)>;
          value.~type();
        },
        *this);
  }

  // The underlying type is not fixed, but should be int - which we will
  // verify statically. See
  // https://timsong-cpp.github.io/cppwp/n4659/dcl.enum#7
  int m_Index;

  // std::aligned_storage is deprecated and may be replaced with the construct
  // below. See
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1413r3.pdf
  alignas(Ts...) std::byte m_Buff[std::max({sizeof(Ts)...})];

private:
  // The friend zone
  template <std::size_t I, typename... Rs>
  friend constexpr decltype(auto) get(variant_base<Rs...> &variant);

  template <std::size_t I, typename... Rs>
  friend constexpr decltype(auto) get(const variant_base<Rs...> &variant);

  template <std::size_t I, typename... Rs>
  friend constexpr std::add_pointer_t<variant_alternative_t<I, Rs...>>
  get_if(variant_base<Rs...> *variant);

  template <std::size_t I, typename... Rs>
  friend constexpr const std::add_pointer_t<variant_alternative_t<I, Rs...>>
  get_if(const variant_base<Rs...> *variant);

  template <typename... Rs> friend struct visitor_type;
};

template <typename First, typename... Remainder>
struct visitor_type<First, Remainder...> {
  template <typename Visitor, typename Variant>
  constexpr static decltype(auto) visit(Visitor &&visitor, Variant &&variant) {
    return visit(std::forward<Visitor>(visitor), variant.m_Index,
                 variant.m_Buff);
  }

  /// @brief The visit method which will pick the right type depending on the
  /// `index` value.
  template <typename Visitor>
  constexpr static auto visit(Visitor &&visitor, std::size_t index,
                              std::byte *data)
      -> decltype(visitor(*reinterpret_cast<First *>(data))) {
    if (index == 0) {
      return visitor(*reinterpret_cast<First *>(data));
    }
    if constexpr (sizeof...(Remainder) != 0) {
      return visitor_type<Remainder...>::visit(std::forward<Visitor>(visitor),
                                               --index, data);
    }
    throw std::out_of_range("invalid");
  }

  template <typename Visitor>
  constexpr static auto visit(Visitor &&visitor, std::size_t index,
                              const std::byte *data)
      -> decltype(visitor(*reinterpret_cast<const First *>(data))) {
    if (index == 0) {
      return visitor(*reinterpret_cast<const First *>(data));
    }
    if constexpr (sizeof...(Remainder) != 0) {
      return visitor_type<Remainder...>::visit(std::forward<Visitor>(visitor),
                                               --index, data);
    }
    throw std::out_of_range("invalid");
  }
};

/// @brief Applies the visitor to the variant. Corresponds to the (3)
/// std::visit definition.
template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor &&visitor,
                               variant_base<Ts...> &variant) {
  return visitor_type<Ts...>::visit(std::forward<Visitor>(visitor), variant);
}

/// @brief Applies the visitor to the variant. Corresponds to the (4)
/// std::visit definition.
template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor &&visitor,
                               const variant_base<Ts...> &variant) {
  return visitor_type<Ts...>::visit(std::forward<Visitor>(visitor), variant);
}

template <std::size_t I, typename... Ts>
constexpr decltype(auto) get(variant_base<Ts...> &variant) {
  variant.template throw_if_invalid<I>();
  return *reinterpret_cast<variant_alternative_t<I, Ts...> *>(variant.m_Buff);
}

template <std::size_t I, typename... Ts>
constexpr decltype(auto) get(const variant_base<Ts...> &variant) {
  variant.template throw_if_invalid<I>();
  return *reinterpret_cast<const variant_alternative_t<I, Ts...> *>(
      variant.m_Buff);
}

template <typename T, typename... Ts,
          typename = std::enable_if_t<
              exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value>>
constexpr const T &get(const variant_base<Ts...> &variant) {
  constexpr auto index = index_from_type<T, Ts...>::value;
  return get<index>(variant);
}

template <typename T, typename... Ts,
          typename = std::enable_if_t<
              exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value>>
constexpr T &get(variant_base<Ts...> &variant) {
  constexpr auto index = index_from_type<T, Ts...>::value;
  return get<index>(variant);
}

template <std::size_t I, typename... Ts>
constexpr std::add_pointer_t<variant_alternative_t<I, Ts...>>
get_if(variant_base<Ts...> *variant) {
  if (!variant->template is_valid<I>())
    return nullptr;
  return reinterpret_cast<variant_alternative_t<I, Ts...> *>(variant->m_Buff);
}

template <std::size_t I, typename... Ts>
constexpr const std::add_pointer_t<variant_alternative_t<I, Ts...>>
get_if(const variant_base<Ts...> *variant) {
  if (!variant->template is_valid<I>())
    return nullptr;
  return reinterpret_cast<const variant_alternative_t<I, Ts...> *>(
      variant->m_Buff);
}

template <typename T, typename... Ts,
          typename = std::enable_if_t<
              exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value>>
constexpr const std::add_pointer_t<T>
get_if(const variant_base<Ts...> *variant) {
  constexpr auto index = index_from_type<T, Ts...>::value;
  return get_if<index>(variant);
}

template <typename T, typename... Ts,
          typename = std::enable_if_t<
              exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value>>
constexpr std::add_pointer_t<T> get_if(variant_base<Ts...> *variant) {
  constexpr auto index = index_from_type<T, Ts...>::value;
  return get_if<index>(variant);
}

template <std::size_t I, typename... Ts>
constexpr bool holds_alternative(const variant_base<Ts...> &variant) {
  return variant.index() == I;
}

template <typename T, typename... Ts,
          typename = std::enable_if_t<
              exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value>>
constexpr bool holds_alternative(const variant_base<Ts...> &variant) {
  return variant.index() == index_from_type<T, Ts...>::value;
}

template <bool> struct copy_control;

template <> struct copy_control<true> {
  copy_control() = default;
  copy_control(const copy_control &other) = default;
  copy_control &operator=(const copy_control &) = default;
};

template <> struct copy_control<false> {
  copy_control() = default;
  copy_control(const copy_control &other) = delete;
  copy_control &operator=(const copy_control &) = delete;
};

template <typename... Ts>
using allow_copy =
    copy_control<std::conjunction_v<std::is_copy_constructible<Ts>...>>;

template <typename... Ts>
struct variant : public variant_base<Ts...>, private allow_copy<Ts...> {
  using base = variant_base<Ts...>;

  variant() = delete;
  variant(const variant &) = default;
  variant(variant &&) = delete;

  using base::base;

  variant &operator=(const variant &) = default;
  variant &operator=(variant &&) = delete;

  using base::operator=;
};

/// An empty type used for unit variants from Rust.
struct monostate {};

} // namespace enm
} // namespace rust

// =================================================
//
// std::optional like binding for Rust Option<T>
//
// =================================================

namespace rust {
namespace enm {

template <typename T> struct optional;

namespace detail {
template <class T> struct is_std_optional_impl : std::false_type {};
template <class T>
struct is_std_optional_impl<std::optional<T>> : std::true_type {};
template <class T>
using is_std_optional = is_std_optional_impl<std::decay_t<T>>;

template <class T> struct is_rust_optional_impl : std::false_type {};
template <class T>
struct is_rust_optional_impl<optional<T>> : std::true_type {};
template <class T>
using is_rust_optional = is_rust_optional_impl<std::decay_t<T>>;
} // namespace detail

struct bad_rust_optional_access : std::runtime_error {
  bad_rust_optional_access() : std::runtime_error{"Optional has no value"} {}
};

template <typename T> struct optional : public variant<monostate, T> {
  using base = variant<monostate, T>;

  optional() : base(monostate{}) {};
  optional(std::nullopt_t) : base(monostate{}) {};
  optional(const optional &) = default;
  optional(optional &&) = delete;

  using base::base;
  using base::operator=;

  using Some = T;
  using None = monostate;

  constexpr explicit operator bool() { return this->m_Index == 1; }
  constexpr bool has_value() const noexcept { return this->m_Index == 1; }
  constexpr bool is_some() const noexcept { return this->m_Index == 1; }
  constexpr bool is_none() const noexcept { return this->m_Index == 0; }

  /// @brief returns the contined value
  ///
  /// if `*this` contains a value returns a refrence. Otherwise throws
  /// bad_rust_optional_access
  /// @throws bad_rust_optional_access
  constexpr T &value() & {
    if (has_value()) {
      return *reinterpret_cast<T *>(this->m_Buff);
    } else {
      throw bad_rust_optional_access();
    }
  }

  /// @brief returns the contined value
  ///
  /// if `*this` contains a value returns a refrence. Otherwise throws
  /// bad_rust_optional_access
  /// @throws bad_rust_optional_access
  constexpr const T &value() const & {
    if (has_value()) {
      return *reinterpret_cast<const T *>(this->m_Buff);
    } else {
      throw bad_rust_optional_access();
    }
  }

  /// @brief resets the optional to an empty state
  ///
  /// if `has_value()` is true first calls the deconstructor
  constexpr void reset() noexcept {
    if (has_value()) {
      this->destroy();
      *this = monostate{};
    }
  }

  constexpr const T *operator->() const noexcept {
    reinterpret_cast<const T *>(this->m_Buff);
  }
  constexpr T *operator->() noexcept { reinterpret_cast<T *>(this->m_Buff); }
  constexpr const T &operator*() const & noexcept {
    *reinterpret_cast<const T *>(this->m_Buff);
  }
  constexpr T &operator*() & noexcept { *reinterpret_cast<T *>(this->m_Buff); }

  template <class U = std::remove_cv_t<T>>
  constexpr T value_or(U &&default_value) const & {
    return has_value() ? *reinterpret_cast<const T *>(this->m_Buff)
                       : static_cast<T>(std::forward<U>(default_value));
  }

  template <class F> constexpr auto and_then(F &&f) & {
    using result = std::invoke_result_t<F, T &>;
    static_assert(detail::is_std_optional<result>::value ||
                      detail::is_rust_optional<result>::value,
                  "F must return an std::optional or rust::variant::optional");
    return (has_value()) ? std::invoke(std::forward<F>(f), value()) : result{};
  }

  template <class F> constexpr auto and_then(F &&f) const & {
    using result = std::invoke_result_t<F, const T &>;
    static_assert(detail::is_std_optional<result>::value ||
                      detail::is_rust_optional<result>::value,
                  "F must return an std::optional or rust::variant::optional");
    return (has_value()) ? std::invoke(std::forward<F>(f), value()) : result{};
  }

  template <class F> constexpr auto transform(F &&f) & {
    using result =
        std::optional<std::remove_cv_t<std::invoke_result_t<F, T &>>>;
    return has_value() ? std::invoke(std::forward<F>(f), value())
                       : result(std::nullopt);
  }

  template <class F> constexpr auto transform(F &&f) const & {
    using result =
        std::optional<std::remove_cv_t<std::invoke_result_t<F, const T &>>>;
    return has_value() ? std::invoke(std::forward<F>(f), value())
                       : result(std::nullopt);
  }

  template <class F> constexpr T &or_else(F &&f) & {
    return has_value() ? *this : std::forward<F>(f)();
  }

  template <class F> constexpr const T &or_else(F &&f) const & {
    return has_value() ? *this : std::forward<F>(f)();
  }

  using IsRelocatable = ::std::true_type;
};

/// Compares two optional objects
template <class T, class U>
inline constexpr bool operator==(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return lhs.has_value == rhs.has_value() && (!lhs.has_value() || *lhs == *rhs);
}
template <class T, class U>
inline constexpr bool operator!=(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return lhs.has_value != rhs.has_value() && (!lhs.has_value() || *lhs != *rhs);
}
template <class T, class U>
inline constexpr bool operator<(const optional<T> &lhs,
                                const optional<U> &rhs) {
  return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
}
template <class T, class U>
inline constexpr bool operator>(const optional<T> &lhs,
                                const optional<U> &rhs) {
  return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
}
template <class T, class U>
inline constexpr bool operator<=(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return !lhs.has_value() || (rhs.has_value() || *lhs <= *rhs);
}
template <class T, class U>
inline constexpr bool operator>=(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return !rhs.has_value() || (lhs.has_value() || *lhs <= *rhs);
}

/// Compares an optional to a `nullopt`
template <class T>
inline constexpr bool operator==(const optional<T> &lhs,
                                 std::nullopt_t) noexcept {
  return !lhs.has_value();
}
template <class T>
inline constexpr bool operator==(std::nullopt_t,
                                 const optional<T> &rhs) noexcept {
  return !rhs.has_value();
}
template <class T>
inline constexpr bool operator!=(const optional<T> &lhs,
                                 std::nullopt_t) noexcept {
  return lhs.has_value();
}
template <class T>
inline constexpr bool operator!=(std::nullopt_t,
                                 const optional<T> &rhs) noexcept {
  return rhs.has_value();
}
template <class T>
inline constexpr bool operator<(const optional<T> &, std::nullopt_t) noexcept {
  return false;
}
template <class T>
inline constexpr bool operator<(std::nullopt_t,
                                const optional<T> &rhs) noexcept {
  return rhs.has_value();
}
template <class T>
inline constexpr bool operator<=(const optional<T> &lhs,
                                 std::nullopt_t) noexcept {
  return !lhs.has_value();
}
template <class T>
inline constexpr bool operator<=(std::nullopt_t, const optional<T> &) noexcept {
  return true;
}
template <class T>
inline constexpr bool operator>(const optional<T> &lhs,
                                std::nullopt_t) noexcept {
  return lhs.has_value();
}
template <class T>
inline constexpr bool operator>(std::nullopt_t, const optional<T> &) noexcept {
  return false;
}
template <class T>
inline constexpr bool operator>=(const optional<T> &, std::nullopt_t) noexcept {
  return true;
}
template <class T>
inline constexpr bool operator>=(std::nullopt_t,
                                 const optional<T> &rhs) noexcept {
  return !rhs.has_value();
}

/// Compares the optional with a value.
template <class T, class U>
inline constexpr bool operator==(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs == rhs : false;
}
template <class T, class U>
inline constexpr bool operator==(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs == *rhs : false;
}
template <class T, class U>
inline constexpr bool operator!=(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs != rhs : true;
}
template <class T, class U>
inline constexpr bool operator!=(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs != *rhs : true;
}
template <class T, class U>
inline constexpr bool operator<(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs < rhs : true;
}
template <class T, class U>
inline constexpr bool operator<(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs < *rhs : false;
}
template <class T, class U>
inline constexpr bool operator<=(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs <= rhs : true;
}
template <class T, class U>
inline constexpr bool operator<=(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs <= *rhs : false;
}
template <class T, class U>
inline constexpr bool operator>(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs > rhs : false;
}
template <class T, class U>
inline constexpr bool operator>(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs > *rhs : true;
}
template <class T, class U>
inline constexpr bool operator>=(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs >= rhs : false;
}
template <class T, class U>
inline constexpr bool operator>=(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs >= *rhs : true;
}

template <class T>
constexpr optional<std::decay_t<T>> make_optional(T &&value) {
  return optional<std::decay_t<T>>(std::forward<T>(value));
}

} // namespace enm
} // namespace rust

// =================================================
//
// std::expected like binding for Rust Result<T, E>
//
// =================================================

namespace rust {
namespace enm {

template <typename T, typename E> struct expected;

namespace detail {
template <typename T> struct is_rust_expected_impl : std::false_type {};
template <typename T, typename E>
struct is_rust_expected_impl<expected<T, E>> : std::true_type {};
template <typename T>
using is_rust_expected = is_rust_expected_impl<std::decay_t<T>>;
} // namespace detail

template <typename E> struct bad_rust_expected_access : std::runtime_error {
  bad_rust_expected_access(const E &err)
      : std::runtime_error{"Expected is the unexpected value"}, error(err) {}
  E error;
};

template <typename T, typename E> struct expected : public variant<T, E> {
  using base = variant<T, E>;

  expected() = delete;
  expected(const expected &) = default;
  expected(expected &&) = delete;

  using base::base;
  using base::operator=;

  using Ok = T;
  using Err = E;

  constexpr explicit operator bool() { return this->m_Index == 0; }
  constexpr bool has_value() const noexcept { return this->m_Index == 0; }

  /// @brief returns the expected value
  ///
  /// @throws bad_rust_expected_access<E> with a copy of the unexpected value
  constexpr T &value() & {
    if (has_value()) {
      return *reinterpret_cast<T *>(this->m_Buff);
    } else {
      throw bad_rust_expected_access(*reinterpret_cast<E *>(this->m_Buff));
    }
  }

  /// @brief returns the expected value
  ///
  /// @throws bad_rust_expected_access<E> with a copy of the unexpected value
  constexpr const T &value() const & {
    if (has_value()) {
      return *reinterpret_cast<const T *>(this->m_Buff);
    } else {
      throw bad_rust_expected_access(*reinterpret_cast<E *>(this->m_Buff));
    }
  }

  /// @brief returns the unexpected value
  ///
  /// if has_value() is `true`, the behavior is undefined
  constexpr E &error() & { return *reinterpret_cast<E *>(this->m_Buff); }

  /// @brief returns the unexpected value
  ///
  /// if has_value() is `true`, the behavior is undefined
  constexpr const E &error() const & {
    return *reinterpret_cast<const E *>(this->m_Buff);
  }

  constexpr const T *operator->() const noexcept {
    reinterpret_cast<const T *>(this->m_Buff);
  }
  constexpr T *operator->() noexcept { reinterpret_cast<T *>(this->m_Buff); }
  constexpr const T &operator*() const & noexcept {
    *reinterpret_cast<const T *>(this->m_Buff);
  }
  constexpr T &operator*() & noexcept { *reinterpret_cast<T *>(this->m_Buff); }

  /// @brief Returns the expected value if it exists, otherwise returns
  /// `default_value`
  template <class U = std::remove_cv_t<T>>
  constexpr T value_or(U &&default_value) const & {
    return has_value() ? *reinterpret_cast<const T *>(this->m_Buff)
                       : static_cast<T>(std::forward<U>(default_value));
  }

  /// @brief Returns the unexpected value if it exists, otherwise returns
  /// `default_value`
  template <class G = E> constexpr E error_or(G &&default_value) const & {
    return has_value() ? std::forward<G>(default_value) : error();
  }

  template <class F> constexpr auto and_then(F &&f) & {
    using result = std::invoke_result_t<F, T &>;
    static_assert(detail::is_rust_expected<result>::value,
                  "F must return a rust::variant::expected");
    return (has_value()) ? std::invoke(std::forward<F>(f), value())
                         : result(error());
  }

  template <class F> constexpr auto and_then(F &&f) const & {
    using result = std::invoke_result_t<F, const T &>;
    static_assert(detail::is_rust_expected<result>::value,
                  "F must return a rust::variant::expected");
    return (has_value()) ? std::invoke(std::forward<F>(f), value())
                         : result(error());
  }

  template <class F> constexpr auto transform(F &&f) & {
    using result = expected<std::remove_cv_t<std::invoke_result_t<F, T &>>, E>;
    return has_value() ? std::invoke(std::forward<F>(f), value())
                       : result(error());
  }

  template <class F> constexpr auto transform(F &&f) const & {
    using result =
        expected<std::remove_cv_t<std::invoke_result_t<F, const T &>>, E>;
    return has_value() ? std::invoke(std::forward<F>(f), value())
                       : result(error());
  }

  template <class F> constexpr T &or_else(F &&f) & {
    return has_value() ? *this : std::forward<F>(f)();
  }

  template <class F> constexpr const T &or_else(F &&f) const & {
    return has_value() ? *this : std::forward<F>(f)();
  }

  template <class F> constexpr auto transform_error(F &&f) & {
    using result = expected<std::remove_cv_t<std::invoke_result_t<F, T &>>, E>;
    return has_value() ? result(*this)
                       : std::invoke(std::forward<F>(f), error());
  }

  template <class F> constexpr auto transform_error(F &&f) const & {
    using result =
        expected<std::remove_cv_t<std::invoke_result_t<F, const T &>>, E>;
    return has_value() ? result(*this)
                       : std::invoke(std::forward<F>(f), error());
  }

  using IsRelocatable = ::std::true_type;
};

} // namespace enm
} // namespace rust

#endif
