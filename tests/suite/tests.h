#pragma once

#include "rust/cxx.h"
#include "rust/cxx_enumext.h" // IWYU pragma: keep
#include "rust/cxx_enumext_macros.h"

#include "tests/suite/data.rs.h"

struct RustValue;
struct SharedData;

using SharedDataRef = std::reference_wrapper<SharedData>;
using RustValueRef = std::reference_wrapper<RustValue>;

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

CXX_DEFINE_OPTIONAL(OptionalInt32, int32_t)

CXX_DEFINE_EXPECTED(I32StringResult, int32_t, rust::string)

template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

RustEnum make_enum();
RustEnum make_enum_str();
RustEnum make_enum_shared();
RustEnum make_enum_shared_ref();
RustEnum make_enum_opaque();

int32_t take_enum(const RustEnum &enm);
int32_t take_mut_enum(RustEnum &);

bool take_optional(const OptionalInt32 &optional);
I32StringResult mul2_if_gt10(int32_t value);
