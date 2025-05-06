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
