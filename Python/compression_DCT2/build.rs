fn main() {
    prost_build::compile_protos(&["proto/bek_format.proto"], &["proto/"]).unwrap();
}
