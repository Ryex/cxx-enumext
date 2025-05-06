# `cxx_enumext`

## Overview

`cxx-enumext` is a Rust crate that extends the [`cxx`](http://cxx.rs/) library to provide  
a mapping between Rust variant enums and c++ standards like `std::variant`, `std::optional`  
and `std::expected`. Unfortunately the memory modal and null pointer optimisations in play  
make direct mappings between the two extremely difficult. As such, `cxx-enumext` provides  
analogs which should be identical in usage.

`rust::enm::variant` is a analog for `std::variant` which can fully map a `#[repr(c)]` Rust  
Enum. The `#[cxx_enumext::extern_type]` macro will transform yur enums apropreatly as well as  
provide some sanity check static assertions about their contents for the current limitations of cxx
ffi.

`rust::enum::optional` is a derivation from `rust::enm::variant` which provides the interface of  
`std::optional`

`rust::enum::expected` is a derivation from `rust::enm::variant` which provides the interface of  
`std::expected`

## Quick tutorial

To use `cxx-enumext`, first start by adding `cxx` to your project. Then add the following to your
`Cargo.toml`:

```toml
[dependencies]
cxx-enumext = "0.1"
```

If your going to put other extern(d) types or shared types inside your enums declare them
in a seperate module so that you don't deal with the c++ compiler complaing
about incompleate types

```rust
//! my_crate/src/data.rs

#[cxx::bridge]
pub mod ffi {

    #[derive(Debug)]
    struct SharedData {
        size: i64,
        tags: Vec<String>,
    }

    extern "Rust" {
        type RustValue;
        fn read(&self) -> &String;

        fn new_rust_value() -> Box<RustValue>;
    }
}

fn new_rust_value() -> Box<RustValue> {
    Box::new(RustValue {
        value: "A Hidden Rust String".into(),
    })
}

#[derive(Default, Debug, Clone)]
pub struct RustValue {
    value: String,
}

impl RustValue {
    pub fn read(&self) -> &String {
        &self.value
    }
    pub fn new(val: &str) -> Self {
        RustValue {
            value: val.to_string(),
        }
    }
}
```

Now, simply declare your enum, optional, or expected

```rust
//! my_crate/src/enum.rs

use data::{RustValue, ffi::SharedData};

// enums -> variants are declared as is
// all variant types are supported
// (Unnamed struct, named structs, unit, single type wrappers)
#[derive(Debug)]
#[cxx_enumext::extern_type]
pub enum RustEnum<'a> {
    Empty,
    Num(i64),
    String(String),
    Bool(bool),
    Shared(SharedData),
    SharedRef(&'a SharedData),
    Opaque(Box<RustValue>),
    OpaqueRef(&'a RustValue),
    Tuple(i32, i32),
    Struct { val: i32, str: String },
    Unit1,
    Unit2,
}

// Declare type aliases for an Optional, "alias" can be for `Optional<T>` or
// `cxx_enumext::Optional<T>`, neither type truely exists but the name will be
// used to determine the enum variants to generate

// `cxx_name` and `namespace` are supported and function identically to `cxx`
#[cxx_enumext::extern_type(cxx_name = "OptionalInt32")]
#[derive(Debug)]
pub type OptionalI32 = Optional<i32>;


// Declare type aliases for an Expected, "alias" can be for `Expected<T,E>` or
// `cxx_enumext::Expected<T,E>`, neither type truely exists but the name will be
// used to determine the enum variants to generate
#[cxx_enumext::extern_type]
#[derive(Debug)]
pub type I32StringResult = cxx_enumext::Expected<i32, String>;


#[cxx::bridge]
pub mod ffi {

    unsafe extern "C++" {
        include!("my_crate/src/enum.h");

        type RustEnum<'a> = super::RustEnum<'a>;
        type I32StringResult = super::I32StringResult;
        type OptionalInt32 = super::OptionalI32;
    }
}
```

This will generate the `cxx::ExternType` impl as well as some non-exhaustive
static assertions to check that contained types are at least externable.

for `Optional` and `Expected` types some `std::convert::From<T>` impls will be cenerated
to convert from and to `Option` and `Result` types

Now, in your C++ file, make sure to `#include` the right headers:

```cpp
#include "rust/cxx.h"
#include "rust/cxx_enumext.h"
#include "rust/cxx_enumext_macros.h"
```

And add a call to the `CXX_DEFINE_VARIANT`, `CXXASYNC_DEFINE_OPTIONAL`,
and or `CXXASYNC_DEFINE_EXPECTED` macro as apropreate to define the C++ side of the types:

```cpp
// my_crate/src/enum.h

#include "rust/cxx.h"
#include "rust/cxx_enumext.h"
#include "rust/cxx_enumext_macros.h"

#include "my_crate/src/data.rs.h"

// The first argument is the name of the C++ type, and the second argument is a parentheses
// enclosed list of variant directives to devine the inner variants.
// They must be declared in the same order as the rust enum and specify the appropriate C++ type.
// An optional third (actualy variadic) argument is placed verbatim in the resulting struct body.
//
// This macro must be invoked in the correct namespace to define the type.
CXX_DEFINE_VARIANT(
    RustEnum,
    (UNIT(Empty) /*(Alias name)*/, TYPE(Num, int64_t) /*(Alias name, type)*/,
     TYPE(String, rust::string) /*by value rust type*/,
     TYPE(Bool, bool) /*primative type*/,
     TYPE(Shared, SharedData) /*shared type*/,
     TYPE(
         SharedRef,
         std::reference_wrapper<SharedData>) /* if your using a refrence wrap is
                                                in a `std::reference_wrapper`*/
     ,
     TYPE(Opaque, rust::box<RustValue>) /* boxed opaque rust type*/,
     TYPE(OpaqueRef,
          std::reference_wrapper<RustValue>) /*refren to opaque rust type*/,
     TUPLE(Tuple, int32_t, int32_t) /*(Alias name, [list, of, types]) => struct
                                       aliast_t{ T1 _0; T2 _1; ...};*/
     ,
     STRUCT(Struct, int32_t val;
            rust::string str;) /*(Alias name, body of struct for members) struct
                                  alist_t{body}; */
     ,
     UNIT(Unit1) /*More the one unit type supported, each is just a `struct
                    alias_t {};`*/
     ,
     UNIT(Unit2) /*NO TRAILING COMMA*/
     ),
    /* optional section injected into the type body (for declareing member
       function etc.) */
    /* DO NOT DECLARE EXTRA MEMBER VARIABLES, THIS WILL MAKE THE TYPE HAVE */
    /* AN INCORRECT MEMORY LAYOUT */
)


// The first argument is the name of the C++ type, and the second contained type
// An optional third (actualy variadic) argument is placed verbatim in the resulting struct body.
CXX_DEFINE_OPTIONAL(OptionalInt32, int32_t)

// The first argument is the name of the C++ type, and the second and third
// are the expected and unexpected types respectively.
// An optional forth (actualy variadic) argument is placed verbatim in the resulting struct body.
CXX_DEFINE_EXPECTED(I32StringResult, int32_t, rust::string)

```

You're all set! Now you can use the enum types on either side of your ffi.

## Installation notes

You will need a C++ compiler that implements c++17

## Refrence

### `rust::enm::variant`

Simplicited declaration

```c++

namespace rust {
namespace enm {

/// @brief The getf method which returns a pointer to T if the variant
// contains T, otherwise throws `bad_rust_variant_access`
template <std::size_t I, typename... Ts>
constexpr decltype(auto) get(variant_base<Ts...> &);

template <std::size_t I, typename... Ts>
constexpr decltype(auto) get(const variant_base<Ts...> &);

/// @brief The get_if method which returns a pointer to T if the variant
/// contains T other wise `nullptr`.
template <std::size_t I, typename... Ts>
constexpr std::add_pointer_t<variant_alternative_t<I, Ts...>>
get_if(variant_base<Ts...> *variant);

template <std::size_t I, typename... Ts>
constexpr const std::add_pointer_t<variant_alternative_t<I, Ts...>>
get_if(const variant_base<Ts...> *variant);

/// @brief The visit method which will pick the right type depending on the
/// `index` value.
template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor &&visitor, variant_base<Ts...> &);

template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor &&visitor, const variant_base<Ts...> &);


/// @brief The holds_alternative method which will return true if the
/// variant contains the type at index I
template <std::size_t I, typename... Ts>
constexpr bool holds_alternative(const variant_base<Ts...> &variant);

/// @brief The holds_alternative method which will return true if the
/// variant contains T
template <typename T, typename... Ts,
          typename = std::enable_if_t<
              exactly_once<std::is_same_v<Ts, std::decay_t<T>>...>::value>>
constexpr bool holds_alternative(const variant_base<Ts...> &variant);

template <typename... Ts> struct variant {
  /// @brief Converting constructor. Corresponds to (4) constructor of
  /// std::variant.
  template <typename T, typename D = std::decay_t<T>,
            typename = std::enable_if_t<is_unique_v<T> &&
                                        std::is_constructible_v<D, T>>>
  variant(T &&other) noexcept(std::is_nothrow_constructible_v<D, T>);

  /// @brief Participates in the resolution only if we can construct T from
  /// Args and if T is unique in Ts. Corresponds to (5) constructor of
  /// std::variant.
  template <typename T, typename... Args,
            typename = std::enable_if_t<is_unique_v<T>>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  explicit variant(std::in_place_type_t<T> type, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>);

  /// @brief Participates in the resolution only if the index is within range
  /// and if the type can be constructor from Args. Corresponds to (7) of
  /// std::variant.
  template <std::size_t I, typename... Args, typename T = type_from_index_t<I>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  explicit variant(
      [[maybe_unused]] std::in_place_index_t<I> index,
      Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>);

  /// @brief Converts the std::variant to our variant. Participates only in
  /// the resolution if all types in Ts are copy constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_copy_constructible_v>>
  variant(const std::variant<Rs...> &other);

  /// @brief Converts the std::variant to our variant. Participates only in
  /// the resolution if all types in Ts are move constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_move_constructible_v>>
  variant(std::variant<Rs...> &&other);

  /// @brief Copy assignment. Statically fails if not every type in Ts is copy
  /// constructable. Corresponds to (1) assignment of std::variant.
  variant_base &operator=(const variant_base &other);

  /// @brief Deleted move assignment. Same as for the move constructor.
  /// Would correspond to (2) assignment of std::variant.
  variant_base &operator=(variant_base &&other) = delete;

  /// @brief Converting assignment. Corresponds to (3) assignment of
  /// std::variant.
  template <typename T, typename = std::enable_if_t<
                            is_unique_v<T> && std::is_constructible_v<T &&, T>>>
  variant_base &operator=(T &&other);

  /// @brief Converting assignment from std::variant. Participates only in the
  /// resolution if all types in Ts are copy constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_copy_constructible_v>>
  variant_base &operator=(const std::variant<Rs...> &other);

  /// @brief Converting assignment from std::variant. Participates only in the
  /// resolution if all types in Ts are move constructable.
  template <typename... Rs, typename = std::enable_if_t<
                                all_same_v<Rs...> && all_move_constructible_v>>
  variant_base &operator=(std::variant<Rs...> &&other);

  /// @brief Emplace function. Participates in the resolution only if T is
  /// unique in Ts and if T can be constructed from Args. Offers strong
  /// exception guarantee. Corresponds to the (1) emplace function of
  /// std::variant.
  template <typename T, typename... Args,
            typename = std::enable_if_t<is_unique_v<T>>,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  T &emplace(Args &&...args);

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
  T &emplace(Args &&...args);

  constexpr std::size_t index() const noexcept;

  void swap(variant_base &other);


  struct bad_rust_variant_access : std::runtime_error {
    bad_rust_variant_access(std::size_t index)
        : std::runtime_error{"The index should be " + std::to_string(index)} {}
  };
}

} // namespace enm
} // namespace rust
```

### `rust::enm::Optional`

Simplicited declaration

```c++

namespace rust {
namespace enm {

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
  constexpr T &value() &;

  /// @brief returns the contined value
  ///
  /// if `*this` contains a value returns a refrence. Otherwise throws
  /// bad_rust_optional_access
  /// @throws bad_rust_optional_access
  constexpr const T &value() const &;

  /// @brief resets the optional to an empty state
  ///
  /// if `has_value()` is true first calls the deconstructor
  constexpr void reset() noexcept;

  constexpr const T *operator->() const noexcept;
  constexpr T *operator->() noexcept;

  constexpr const T &operator*() const & noexcept;
  constexpr T &operator*() & noexcept;

  template <class U = std::remove_cv_t<T>>
  constexpr T value_or(U &&default_value) const &;

  template <class F> constexpr auto and_then(F &&f) &;
  template <class F> constexpr auto and_then(F &&f) const &;
  template <class F> constexpr auto transform(F &&f) &;

  template <class F> constexpr auto transform(F &&f) const &;

  template <class F> constexpr T &or_else(F &&f) &;
  template <class F> constexpr const T &or_else(F &&f) const &;
};


/// Compares two optional objects
template <class T, class U>
inline constexpr bool operator==(const optional<T> &lhs,
                                 const optional<U> &rhs);
template <class T, class U>
inline constexpr bool operator!=(const optional<T> &lhs,
                                 const optional<U> &rhs);
template <class T, class U>
inline constexpr bool operator<(const optional<T> &lhs,
                                const optional<U> &rhs)
// ... and many more compare operators 
```

### `rust::enm::expected`

Simplicited declaration

```c++

namespace rust {
namespace enm {

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
  constexpr T &value() &;

  /// @brief returns the expected value
  ///
  /// @throws bad_rust_expected_access<E> with a copy of the unexpected value
  constexpr const T &value() const &;

  /// @brief returns the unexpected value
  ///
  /// if has_value() is `true`, the behavior is undefined
  constexpr E &error() &;

  /// @brief returns the unexpected value
  ///
  /// if has_value() is `true`, the behavior is undefined
  constexpr const E &error() const &;

  constexpr const T *operator->() const noexcept;
  constexpr T *operator->() noexcept;

  constexpr const T &operator*() const & noexcept;
  constexpr T &operator*() & noexcept;

  /// @brief Returns the expected value if it exists, otherwise returns
  /// `default_value`
  template <class U = std::remove_cv_t<T>>
  constexpr T value_or(U &&default_value) const &;

  /// @brief Returns the unexpected value if it exists, otherwise returns
  /// `default_value`
  template <class G = E> constexpr E error_or(G &&default_value) const &;

  template <class F> constexpr auto and_then(F &&f) &;
  template <class F> constexpr auto and_then(F &&f) const &;

  template <class F> constexpr auto transform(F &&f) &;
  template <class F> constexpr auto transform(F &&f) const &;

  template <class F> constexpr T &or_else(F &&f) &;
  template <class F> constexpr const T &or_else(F &&f) const &;

  template <class F> constexpr auto transform_error(F &&f) &;
  template <class F> constexpr auto transform_error(F &&f) const &;

};

} // namespace enm
} // namespace rust
```

## Code of conduct

`cxx-enumext` follows the same Code of Conduct as Rust itself. Reports can be made to the crate
authors.

## License

Licensed under either of Apache License, Version 2.0 or MIT license at your option.

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in
this project by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without
any additional terms or conditions.
