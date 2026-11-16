__zig__ {
    extern "c" fn puts(s: [*c]const u8) c_int;
}

fn test_c_ffi() {
    @print("Calling C puts function...");
    __zig__ {
        _ = puts("Hello from C FFI!");
    }
}

export fn main() {
    test_c_ffi();
}
