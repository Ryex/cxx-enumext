use cxx_build::CFG;

fn main() {
    CFG.include_prefix = "tests/suite";
    let sources = vec!["lib.rs", "data.rs"];
    let mut build = cxx_build::bridges(sources);
    build.file("tests.cpp");
    build.std("c++17");
    build.flag_if_supported("-std=c++17");
    build.flag_if_supported("/std:c++17");
    build.compile("cxx-enum-ext-test-suite");

    println!("cargo:rerun-if-changed=tests.cpp");
    println!("cargo:rerun-if-changed=tests.h");
}
