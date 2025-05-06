use cxx_build::CFG;
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

fn main() {
    // Copy include files so that dependent crates can find them.

    let out_dir = Path::new(&env::var("OUT_DIR").expect("`OUT_DIR` not set!")).to_owned();

    let mut src_include_path = Path::new(".").to_owned();
    let mut dest_include_path = out_dir.clone();

    for include_path in &mut [&mut src_include_path, &mut dest_include_path] {
        include_path.push("include");
        include_path.push("rust");
    }

    drop(fs::create_dir_all(&dest_include_path));

    for header in &["cxx_enumext.h", "cxx_enumext_macros.h"] {
        drop(fs::copy(
            Path::join(&src_include_path, header),
            Path::join(&dest_include_path, header),
        ));
        println!("cargo:rerun-if-changed=include/rust/{}", header);
    }

    let out_headers = Path::join(&out_dir, "include");

    // export headers to others
    println!("cargo:include={}", out_headers.to_string_lossy());
    CFG.exported_header_dirs.push(&out_headers);

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/cxx_enumext.cpp");

    println!("cargo:rustc-cfg=built_with_cargo");

    let no_bridges: Vec<PathBuf> = vec![];
    cxx_build::bridges(no_bridges)
        .warnings(false)
        .cargo_warnings(false)
        .files(&vec!["src/cxx_enumext.cpp"])
        .flag_if_supported("-std=c++17")
        .include("include")
        .compile("cxx-enumext");
}
