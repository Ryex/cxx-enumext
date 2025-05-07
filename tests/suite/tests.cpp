#include "tests/suite/lib.rs.h"

#include <atomic>
#include <functional>
#include <iostream>
#include <sstream>
RustEnum make_enum() { return RustEnum{RustEnum::Num(1502)}; }
RustEnum make_enum_str() {
  return RustEnum(RustEnum::String("String from c++"));
}
RustEnum make_enum_shared() {
  SharedData d;
  d.size = 4;
  d.tags = ::rust::Vec<::rust::String>({
      RustEnum::String("tag_a"),
      RustEnum::String("tag_b"),
      RustEnum::String("tag_c"),
      RustEnum::String("tag_d"),
  });

  return RustEnum(d);
}

RustEnum make_enum_shared_ref() {
  static std::atomic_bool once;
  static SharedData shared_s;
  if (!once) {
    once = true;
    shared_s = {5, ::rust::Vec<::rust::String>({
                       RustEnum::String("tag_a"),
                       RustEnum::String("tag_b"),
                       RustEnum::String("tag_c"),
                       RustEnum::String("tag_d"),
                       RustEnum::String("tag_e"),
                   })};
  }

  return RustEnum(std::reference_wrapper<SharedData>(shared_s));
}

RustEnum make_enum_opaque() { return RustEnum(new_rust_value()); }

int32_t take_enum(const RustEnum &enm) {
  int32_t ret = enm.index();
  std::ostringstream os;
  os << "The index of enum is " << enm.index();
  rust_println(rust::string(os.str()));

  const auto visitor = overload{
      [](const RustEnum::Empty) {
        std::ostringstream os;
        os << "The value of enum is ::rust::empty";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::String &v) {
        std::ostringstream os;
        os << "The value of enum is string '" << v << "'";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::Shared &d) {
        std::ostringstream os;
        os << "The value of enum is SharedData struct { " << std::endl
           << "\tsize: " << d.size << "," << std::endl
           << "\ttags: [";
        for (const auto &tag : d.tags) {
          os << std::endl << "\t\t\"" << tag << "\", ";
        }
        os << std::endl << "\t]," << std::endl << "}";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::SharedRef &d) {
        std::ostringstream os;
        os << "The value of enum is SharedDataRef struct { " << std::endl
           << "\tsize: " << d.get().size << "," << std::endl
           << "\ttags: [";
        for (const auto &tag : d.get().tags) {
          os << std::endl << "\t\t\"" << tag << "\", ";
        }
        os << std::endl << "\t]," << std::endl << "}";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::Opaque &d) {
        std::ostringstream os;
        os << "The value of enum is Opaque '" << d->read() << "'";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::OpaqueRef &d) {
        std::ostringstream os;
        os << "The value of enum is OpaqueRef '" << d.get().read() << "'";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::Tuple &v) {
        std::ostringstream os;
        os << "The value of enum is Tuple (" << v._0 << ", " << v._1 << ")";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::Struct &v) {
        std::ostringstream os;
        os << "The value of enum is Struct { \n\tval: " << v.val
           << ",\n\tstr: " << v.str << "\n}";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::Unit1) {
        std::ostringstream os;
        os << "The value of enum is Unit1";
        rust_println(rust::string(os.str()));
      },
      [](const RustEnum::Unit2) {
        std::ostringstream os;
        os << "The value of enum is Unit2";
        rust_println(rust::string(os.str()));
      },
      [](const auto &v) {
        std::ostringstream os;
        os << "The value of enum is " << v;
        rust_println(rust::string(os.str()));
      },
  };
  rust::enm::visit(visitor, enm);
  return ret;
}

int32_t take_mut_enum(RustEnum &enm) {
  int32_t ret = take_enum(enm);

  if (!::rust::enm::holds_alternative<RustEnum::Bool>(enm)) {
    enm = false;
  } else {
    enm = int64_t(111);
  }
  return ret;
}

bool take_optional(const OptionalInt32 &optional) {
  std::ostringstream os;
  if (optional.has_value()) {
    os << "The value of optional is " << optional.value();
  } else {
    os << "The value of optional is empty";
  }
  rust_println(rust::string(os.str()));
  return optional.has_value();
}

I32StringResult mul2_if_gt10(int32_t value) {
  if (value > 10) {
    I32StringResult ret{int32_t(value * 2)};
    return ret;
  } else {
    return I32StringResult{rust::String("value too small")};
  }
}

int32_t take_expected_void(ExpectedVoidInt result) {
  if (result.has_value())
    return 1000;
  return result.error();
}

ExpectedVoidInt make_expected_void() {
  return ExpectedVoidInt();
}
ExpectedVoidInt make_unexpected_void() {
  return ExpectedVoidInt(42);
}
