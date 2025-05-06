use cxx_enumext_test_suite::{
    ffi::{
        make_enum, make_enum_opaque, make_enum_shared, make_enum_shared_ref, make_enum_str,
        mul2_if_gt10, take_enum, take_mut_enum, take_optional,
    },
    OptionalI32, RustEnum, RustValue, SharedData,
};

fn print_enum(enm: &RustEnum) {
    match &enm {
        RustEnum::Empty => println!("The value is string Empty"),
        RustEnum::String(s) => println!("The value is string '{s}'"),
        RustEnum::Num(n) => println!("The value is {n}"),
        RustEnum::Bool(b) => println!("The value is {b}"),
        RustEnum::Shared(d) => println!("The value is Shared Data {d:?}"),
        _ => println!("The value is {enm:?}"),
    }
}

#[test]
fn test_pass_ffi() {
    let f = RustEnum::String(String::from("I'm a Rust String"));
    assert_eq!(take_enum(&f), 2);
    let f = RustEnum::Empty;
    assert_eq!(take_enum(&f), 0);
    let f = RustEnum::Num(42);
    assert_eq!(take_enum(&f), 1);

    let f = RustEnum::Shared(SharedData {
        size: 2,
        tags: vec!["rust_tag_1".into(), "rust_tag_2".into()],
    });
    assert_eq!(take_enum(&f), 4);
    {
        let shared = SharedData {
            size: 2,
            tags: vec![
                "rust_tag_1".into(),
                "rust_tag_2".into(),
                "rust_tag_3".into(),
            ],
        };
        let f = RustEnum::SharedRef(&shared);
        assert_eq!(take_enum(&f), 5);
    }

    let f = RustEnum::Opaque(Box::new(RustValue::new("A Hidden Rust String from Rust")));
    assert_eq!(take_enum(&f), 6);
    {
        let opaque = RustValue::new("A Hidden Rust String from Rust behind ref");
        let f = RustEnum::OpaqueRef(&opaque);

        assert_eq!(take_enum(&f), 7);
    }

    let f = RustEnum::Tuple(42, 1377);
    assert_eq!(take_enum(&f), 8);
    let f = RustEnum::Struct {
        val: 70,
        str: "Also a Rust String".into(),
    };
    assert_eq!(take_enum(&f), 9);
    let f = RustEnum::Unit1;
    assert_eq!(take_enum(&f), 10);
    let f = RustEnum::Unit2;
    assert_eq!(take_enum(&f), 11);
}

#[test]
fn test_from_ffi() {
    let mut f = make_enum();
    print_enum(&f);
    assert!(matches!(f, RustEnum::Num(1502)));
    take_mut_enum(&mut f);
    assert!(matches!(&f, RustEnum::Bool(false)));
    print_enum(&f);
    let f = make_enum_str();
    assert!(matches!(&f, RustEnum::String(s) if s == "String from c++"));
    print_enum(&f);
    let f = make_enum_shared();
    assert!(matches!(&f, RustEnum::Shared(s) if s.size == 4));
    print_enum(&f);
    let f = make_enum_shared_ref();
    assert!(matches!(&f, RustEnum::SharedRef(s) if s.size == 5));
    print_enum(&f);
    let f = make_enum_opaque();
    assert!(matches!(&f, RustEnum::Opaque(_)));
    print_enum(&f);
}

#[test]
fn test_pass_optional_ffi() {
    let optional = OptionalI32::Some(8);
    assert!(take_optional(&optional));
    let optional = OptionalI32::None;
    assert!(!take_optional(&optional));
}

#[test]
fn test_get_expected_ffi() {
    let result: Result<_, _> = mul2_if_gt10(12).into();
    println!("result is: {result:?}");
    assert!(matches!(result, Ok(24)));
    let result: Result<_, _> = mul2_if_gt10(8).into();
    println!("result is: {result:?}");
    assert!(matches!(result, Err(s) if s == "value too small"));
}
