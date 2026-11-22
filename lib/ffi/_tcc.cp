// Source: Absorbed from /data/dev/bun/src/runtime/ffi/ | Synced: 2026-11-22T12:53:50Z
// lib/ffi/_tcc.cp
// TinyCC In-Memory C Compiler C-FFI Bindings
// Absorbed from Bun src/tcc_sys/tcc.cp

pub @opaque struct TCCState {}

pub enum TccOutputFormat: i32 {
    Memory = 1,
    Exe = 2,
    Dll = 3,
    Obj = 4,
    Preprocess = 5,
}

pub @c {
    fn tcc_new(): (*TCCState)?;
    fn tcc_delete(s: *TCCState);
    fn tcc_set_lib_path(s: *TCCState, path: [*c]const u8);
    fn tcc_set_options(s: *TCCState, str: [*c]const u8): i32;
    fn tcc_add_include_path(s: *TCCState, pathname: [*c]const u8): i32;
    fn tcc_add_sysinclude_path(s: *TCCState, pathname: [*c]const u8): i32;
    fn tcc_define_symbol(s: *TCCState, sym: [*c]const u8, value: [*c]const u8);
    fn tcc_undefine_symbol(s: *TCCState, sym: [*c]const u8);
    fn tcc_add_file(s: *TCCState, filename: [*c]const u8): i32;
    fn tcc_compile_string(s: *TCCState, buf: [*c]const u8): i32;
    fn tcc_set_output_type(s: *TCCState, output_type: i32): i32;
    fn tcc_add_library_path(s: *TCCState, pathname: [*c]const u8): i32;
    fn tcc_add_library(s: *TCCState, libraryname: [*c]const u8): i32;
    fn tcc_add_symbol(s: *TCCState, name: [*c]const u8, val: *const !?): i32;
    fn tcc_relocate(s1: *TCCState): i32;
    fn tcc_get_symbol(s: *TCCState, name: [*c]const u8): *!?;
}
