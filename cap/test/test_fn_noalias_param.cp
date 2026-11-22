fn copy_bytes_noalias(noalias dst: [*]u8, noalias src: [*]const u8, len: usize) {
    let i: usize = 0;
    while (i < len) {
        unsafe {
            dst[i] = src[i];
        }
        i += 1;
    }
}

fn copy_first_byte(noalias dst: [*]u8, src: [*]const u8) {
    unsafe {
        dst[0] = src[0];
    }
}

fn copy_generic_noalias(comptime T: type, noalias dst: [*]T, noalias src: [*]const T) {
    unsafe {
        dst[0] = src[0];
    }
}

fn main() {
    let src: [4]u8 = [10, 20, 30, 40];
    let dst: [4]u8 = [0, 0, 0, 0];

    unsafe {
        copy_bytes_noalias(@ptrCast(&dst), @ptrCast(&src), 4);
    }
    @expect_eq(dst[0], 10);
    @expect_eq(dst[1], 20);
    @expect_eq(dst[2], 30);
    @expect_eq(dst[3], 40);

    let src2: [1]u8 = [99];
    let dst2: [1]u8 = [0];
    unsafe {
        copy_first_byte(@ptrCast(&dst2), @ptrCast(&src2));
    }
    @expect_eq(dst2[0], 99);

    let src3: [1]u32 = [123456];
    let dst3: [1]u32 = [0];
    unsafe {
        copy_generic_noalias(u32, @ptrCast(&dst3), @ptrCast(&src3));
    }
    @expect_eq(dst3[0], 123456);
}
