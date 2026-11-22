// Phase 1 AI-Native SIMD 向量内建原语端到端回归测试: @fma / @dot / @l2sq

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

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

fn scalarDotF32(a: []const f32, b: []const f32): f32 {
    let n: usize = a.len;
    if (b.len < n) {
        n = b.len;
    }
    let sum: f32 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum += a[i] * b[i];
        i += 1;
    }
    return sum;
}

fn scalarDotF64(a: []const f64, b: []const f64): f64 {
    let n: usize = a.len;
    if (b.len < n) {
        n = b.len;
    }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum += a[i] * b[i];
        i += 1;
    }
    return sum;
}

fn scalarL2SqF32(a: []const f32, b: []const f32): f32 {
    let n: usize = a.len;
    if (b.len < n) {
        n = b.len;
    }
    let sum: f32 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const d: f32 = a[i] - b[i];
        sum += d * d;
        i += 1;
    }
    return sum;
}

fn scalarL2SqF64(a: []const f64, b: []const f64): f64 {
    let n: usize = a.len;
    if (b.len < n) {
        n = b.len;
    }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const d: f64 = a[i] - b[i];
        sum += d * d;
        i += 1;
    }
    return sum;
}

fn testFma() {
    // f32 标量精度与零/负数边界
    const a32: f32 = 2.5;
    const b32: f32 = -4.0;
    const c32: f32 = 1.25;
    const r32: f32 = @fma(a32, b32, c32);
    assert(absF32(r32 - (-8.75)) < 1.0e-6, "fma f32 negative");

    const z32: f32 = @fma(0.0 as f32, 12345.6 as f32, -0.5 as f32);
    assert(absF32(z32 - (-0.5)) < 1.0e-6, "fma f32 zero");

    // f64 标量精度与硬件单舍入优势验证:
    // (1 + 2^-27) * (1 - 2^-27) - 1 = -2^-54 (普通 a*b+c 会因中间舍入归零，FMA 精确保留)
    const eps27: f64 = 1.0 / 134217728.0;
    const a64: f64 = 1.0 + eps27;
    const b64: f64 = 1.0 - eps27;
    const c64: f64 = -1.0;
    const r64: f64 = @fma(a64, b64, c64);
    const expected_fma: f64 = -(eps27 * eps27);
    assert(absF64(r64 - expected_fma) == 0.0, "fma f64 exact single-rounding");

    const n64: f64 = @fma(-3.5 as f64, -2.0 as f64, -7.0 as f64);
    assert(absF64(n64) < 1.0e-12, "fma f64 cancel to zero");

    // f64 与字面量混合
    const mix64: f64 = @fma(a64, 2.0, 3.0);
    assert(absF64(mix64 - (2.0 * a64 + 3.0)) < 1.0e-12, "fma f64 literal mix");
}

fn testDotAndL2SqF32() {
    let bufA: [1024]f32 = undefined;
    let bufB: [1024]f32 = undefined;
    let i: usize = 0;
    while (i < 1024) {
        const fi: f32 = @floatFromInt(i % 17);
        bufA[i] = (fi * 0.004) - 0.03;
        bufB[i] = (fi * -0.003) + 0.02;
        i += 1;
    }

    // 长度 0、1、7（纯尾循环）、8（整 SIMD 块）、19（SIMD+尾循环）、1024（大向量）
    const lens: [6]usize = [0, 1, 7, 8, 19, 1024];
    let k: usize = 0;
    while (k < 6) {
        const n: usize = lens[k];
        const sa: []const f32 = bufA[0..n];
        const sb: []const f32 = bufB[0..n];

        const d_simd: f32 = @dot(sa, sb);
        const d_ref: f32 = scalarDotF32(sa, sb);
        assert(absF32(d_simd - d_ref) < 1.0e-5, "dot f32 mismatch");

        const l2_simd: f32 = @l2sq(sa, sb);
        const l2_ref: f32 = scalarL2SqF32(sa, sb);
        assert(absF32(l2_simd - l2_ref) < 1.0e-5, "l2sq f32 mismatch");

        // 相同向量 l2sq 严格为 0
        const l2_self: f32 = @l2sq(sa, sa);
        assert(l2_self == 0.0, "l2sq f32 self must be 0");

        k += 1;
    }

    // 长度不等时按 min(a.len, b.len) 安全处理
    const d_unequal: f32 = @dot(bufA[0..19], bufB[0..7]);
    const d_unequal_ref: f32 = scalarDotF32(bufA[0..7], bufB[0..7]);
    assert(absF32(d_unequal - d_unequal_ref) < 1.0e-5, "dot f32 unequal len");

    // 正交向量精确验证
    const o1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const o2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    assert(@dot(o1[0..], o2[0..]) == 0.0, "dot f32 orthogonal");
    assert(@l2sq(o1[0..], o2[0..]) == 2.0, "l2sq f32 orthogonal");
}

fn testDotAndL2SqF64() {
    let bufA: [1024]f64 = undefined;
    let bufB: [1024]f64 = undefined;
    let i: usize = 0;
    while (i < 1024) {
        const fi: f64 = @floatFromInt(i);
        bufA[i] = (fi * 0.013) - 3.5;
        bufB[i] = (fi * -0.007) + 1.25;
        i += 1;
    }

    const lens: [6]usize = [0, 1, 7, 8, 19, 1024];
    let k: usize = 0;
    while (k < 6) {
        const n: usize = lens[k];
        const sa: []const f64 = bufA[0..n];
        const sb: []const f64 = bufB[0..n];

        const d_simd: f64 = @dot(sa, sb);
        const d_ref: f64 = scalarDotF64(sa, sb);
        assert(absF64(d_simd - d_ref) < 1.0e-8, "dot f64 mismatch");

        const l2_simd: f64 = @l2sq(sa, sb);
        const l2_ref: f64 = scalarL2SqF64(sa, sb);
        assert(absF64(l2_simd - l2_ref) < 1.0e-8, "l2sq f64 mismatch");

        const l2_self: f64 = @l2sq(sa, sa);
        assert(l2_self == 0.0, "l2sq f64 self must be 0");

        k += 1;
    }

    // 长度不等时按 min(a.len, b.len) 安全处理
    const l2_unequal: f64 = @l2sq(bufA[0..7], bufB[0..19]);
    const l2_unequal_ref: f64 = scalarL2SqF64(bufA[0..7], bufB[0..7]);
    assert(absF64(l2_unequal - l2_unequal_ref) < 1.0e-10, "l2sq f64 unequal len");

    // 正交向量精确验证 (9 维覆盖 8-lane SIMD 块 + 1 维尾循环)
    const o1: [9]f64 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.0];
    const o2: [9]f64 = [0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 3.0, 0.0];
    assert(@dot(o1[0..], o2[0..]) == 0.0, "dot f64 orthogonal");
    assert(@l2sq(o1[0..], o2[0..]) == 15.0, "l2sq f64 orthogonal");
}

fn main() {
    testFma();
    testDotAndL2SqF32();
    testDotAndL2SqF64();
    @print("PASS: test_builtin_simd_dot_fma");
}
