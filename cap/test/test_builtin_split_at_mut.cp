// Phase 2 AI-Native @splitAtMut 零拷贝互斥分片 + noalias 借用优化端到端回归测试

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        @panic(msg);
    }
}

fn absF32(x: f32): f32 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

// 纯 CP 独占可变切片 (*[]f32) + 只读切片 ([]const f32) 共存函数：
// 编译器自动为 out / x 注入 Zig `noalias` 提示以解锁 LLVM SIMD 向量化与 LICM
fn saxpyNoalias(out: *[]f32, x: []const f32, alpha: f32) {
    let n: usize = out.len;
    if (x.len < n) {
        n = x.len;
    }
    let i: usize = 0;
    while (i < n) {
        out[i] = @fma(alpha, x[i], out[i]);
        i += 1;
    }
}

fn testSplitAtMutU8AndI32() {
    // 1. u8 切片原地互斥修改
    let raw_u8: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    let s_u8: []u8 = raw_u8[0..];
    let [lu, ru] = @splitAtMut(s_u8, 3);
    assert(lu.len == 3, "u8 left len");
    assert(ru.len == 5, "u8 right len");
    lu[0] = 10;
    lu[2] = 30;
    ru[0] = 40;
    ru[4] = 80;
    assert(raw_u8[0] == 10 && raw_u8[2] == 30 && raw_u8[3] == 40 && raw_u8[7] == 80, "u8 in-place mutation");

    // 2. i32 切片及边界：mid == 0, mid == len, mid > len 安全钳位
    let raw_i32: [6]i32 = [10, 20, 30, 40, 50, 60];
    let s_i32: []i32 = raw_i32[0..];

    let [z_l, z_r] = @splitAtMut(s_i32, 0);
    assert(z_l.len == 0 && z_r.len == 6, "i32 mid == 0");
    z_r[0] = 11;
    assert(raw_i32[0] == 11, "i32 mid == 0 right write");

    let [e_l, e_r] = @splitAtMut(s_i32, 6);
    assert(e_l.len == 6 && e_r.len == 0, "i32 mid == len");
    e_l[5] = 66;
    assert(raw_i32[5] == 66, "i32 mid == len left write");

    let [c_l, c_r] = @splitAtMut(s_i32, 999);
    assert(c_l.len == 6 && c_r.len == 0, "i32 mid > len clamp");
    c_l[1] = 22;
    assert(raw_i32[1] == 22, "i32 clamp write");
}

fn testSplitAtMutTensorDotFma() {
    // 将一块连续 32-float 缓冲区零拷贝拆成不重叠的 Q (16) 和 K (16) 视图
    let arena_buf: [32]f32 = undefined;
    let buf: []f32 = arena_buf[0..];
    let [q, k] = @splitAtMut(buf, 16);
    assert(q.len == 16 && k.len == 16, "f32 tensor split len");

    // 同一作用域内独立并发写入 left(q) 与 right(k)，零借用冲突
    let i: usize = 0;
    while (i < 16) {
        const fi: f32 = @floatFromInt(i + 1);
        q[i] = fi * 0.5;
        k[i] = 2.0;
        i += 1;
    }

    // 结合 @dot 计算 Q·K = sum_{i=1..16} (0.5 * i * 2.0) = sum_{i=1..16} i = 136.0
    const score_before: f32 = @dot(q, k);
    assert(absF32(score_before - 136.0) < 1.0e-5, "splitAtMut + @dot before saxpy");

    // 结合 noalias 函数 + @fma 原地更新 q = 1.5 * k + q = q + 3.0
    saxpyNoalias(q, k, 1.5);

    // 更新后 Q·K = 136.0 + 16 * (3.0 * 2.0) = 232.0
    const score_after: f32 = @dot(q, k);
    assert(absF32(score_after - 232.0) < 1.0e-5, "splitAtMut + noalias saxpy + @dot after");

    // 验证底层连续缓冲区已零拷贝原地更新
    assert(absF32(arena_buf[0] - 3.5) < 1.0e-5, "arena_buf[0] zero-copy");
    assert(absF32(arena_buf[16] - 2.0) < 1.0e-5, "arena_buf[16] untouched k");
}

fn main() {
    testSplitAtMutU8AndI32();
    testSplitAtMutTensorDotFma();
    @print("PASS: test_builtin_split_at_mut");
}
