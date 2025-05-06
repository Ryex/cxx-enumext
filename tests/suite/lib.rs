pub mod data;

pub use data::ffi::SharedData;
pub use data::RustValue;

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

#[cxx_enumext::extern_type(cxx_name = "OptionalInt32")]
#[derive(Debug)]
pub type OptionalI32 = Optional<i32>;

#[cxx_enumext::extern_type]
#[derive(Debug)]
pub type I32StringResult = cxx_enumext::Expected<i32, String>;

#[cxx::bridge]
pub mod ffi {

    unsafe extern "C++" {
        include!("tests/suite/tests.h");

        type RustEnum<'a> = super::RustEnum<'a>;
        type I32StringResult = super::I32StringResult;
        type OptionalInt32 = super::OptionalI32;

        pub fn make_enum<'a>() -> RustEnum<'a>;
        pub fn make_enum_str<'a>() -> RustEnum<'a>;
        pub fn make_enum_shared<'a>() -> RustEnum<'a>;
        pub fn make_enum_shared_ref<'a>() -> RustEnum<'a>;
        pub fn make_enum_opaque<'a>() -> RustEnum<'a>;
        pub fn take_enum(enm: &RustEnum) -> i32;
        pub fn take_mut_enum(enm: &mut RustEnum) -> i32;

        pub fn take_optional(optional: &OptionalInt32) -> bool;
        pub fn mul2_if_gt10(value: i32) -> I32StringResult;

    }

    extern "Rust" {
        fn rust_println(msg: String);
    }
}

pub fn rust_println(msg: String) {
    println!("{msg}");
}
