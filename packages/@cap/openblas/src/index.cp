// @cap/openblas — OpenBLAS (v0.3.29) High-Performance BLAS Matrix Computation for CP
// License: BSD-3-Clause (Upstream: OpenMathLib/OpenBLAS)

import std from "std";

pub const CBLAS_ROW_MAJOR: i32 = 101;
pub const CBLAS_COL_MAJOR: i32 = 102;
pub const CBLAS_NO_TRANS: i32 = 111;
pub const CBLAS_TRANS: i32 = 112;
pub const CBLAS_CONJ_TRANS: i32 = 113;

pub @c {
    fn cblas_sgemm(
        order: i32,
        trans_a: i32,
        trans_b: i32,
        m: i32,
        n: i32,
        k: i32,
        alpha: f32,
        a: [*]const f32!,
        lda: i32,
        b: [*]const f32!,
        ldb: i32,
        beta: f32,
        c: [*]f32!,
        ldc: i32,
    );
    fn cblas_dgemm(
        order: i32,
        trans_a: i32,
        trans_b: i32,
        m: i32,
        n: i32,
        k: i32,
        alpha: f64,
        a: [*]const f64!,
        lda: i32,
        b: [*]const f64!,
        ldb: i32,
        beta: f64,
        c: [*]f64!,
        ldc: i32,
    );
    fn cblas_sdot(n: i32, x: [*]const f32!, inc_x: i32, y: [*]const f32!, inc_y: i32): f32;
    fn cblas_ddot(n: i32, x: [*]const f64!, inc_x: i32, y: [*]const f64!, inc_y: i32): f64;
    fn openblas_get_config(): [*:0]const u8;
}

pub err BlasError {
    InvalidDimensions,
    BufferTooSmall,
}

/// Returns the OpenBLAS build configuration string (e.g. target architecture and threading).
pub fn openblasGetConfig(): string {
    unsafe {
        return std.mem.span(openblas_get_config());
    }
}

/// Single-precision row-major matrix multiplication: `C = alpha * A * B + beta * C`.
/// `A` has shape `(m, k)`, `B` has shape `(k, n)`, `C` has shape `(m, n)`.
pub fn sgemm(
    m: usize,
    n: usize,
    k: usize,
    alpha: f32,
    a: []const f32,
    b: []const f32,
    beta: f32,
    c: []f32,
): void! {
    if (m == 0 || n == 0 || k == 0) {
        return BlasError.InvalidDimensions;
    }
    if (a.len < m * k || b.len < k * n || c.len < m * n) {
        return BlasError.BufferTooSmall;
    }
    const mi: i32 = @intCast(m);
    const ni: i32 = @intCast(n);
    const ki: i32 = @intCast(k);
    unsafe {
        cblas_sgemm(
            CBLAS_ROW_MAJOR,
            CBLAS_NO_TRANS,
            CBLAS_NO_TRANS,
            mi,
            ni,
            ki,
            alpha,
            a.ptr,
            ki,
            b.ptr,
            ni,
            beta,
            c.ptr,
            ni,
        );
    }
}

/// Double-precision row-major matrix multiplication: `C = alpha * A * B + beta * C`.
/// `A` has shape `(m, k)`, `B` has shape `(k, n)`, `C` has shape `(m, n)`.
pub fn dgemm(
    m: usize,
    n: usize,
    k: usize,
    alpha: f64,
    a: []const f64,
    b: []const f64,
    beta: f64,
    c: []f64,
): void! {
    if (m == 0 || n == 0 || k == 0) {
        return BlasError.InvalidDimensions;
    }
    if (a.len < m * k || b.len < k * n || c.len < m * n) {
        return BlasError.BufferTooSmall;
    }
    const mi: i32 = @intCast(m);
    const ni: i32 = @intCast(n);
    const ki: i32 = @intCast(k);
    unsafe {
        cblas_dgemm(
            CBLAS_ROW_MAJOR,
            CBLAS_NO_TRANS,
            CBLAS_NO_TRANS,
            mi,
            ni,
            ki,
            alpha,
            a.ptr,
            ki,
            b.ptr,
            ni,
            beta,
            c.ptr,
            ni,
        );
    }
}

/// Single-precision dot product of vectors `x` and `y` of equal length.
pub fn sdot(x: []const f32, y: []const f32): f32! {
    if (x.len == 0 || x.len != y.len) {
        return BlasError.InvalidDimensions;
    }
    const ni: i32 = @intCast(x.len);
    unsafe {
        return cblas_sdot(ni, x.ptr, 1, y.ptr, 1);
    }
}

/// Double-precision dot product of vectors `x` and `y` of equal length.
pub fn ddot(x: []const f64, y: []const f64): f64! {
    if (x.len == 0 || x.len != y.len) {
        return BlasError.InvalidDimensions;
    }
    const ni: i32 = @intCast(x.len);
    unsafe {
        return cblas_ddot(ni, x.ptr, 1, y.ptr, 1);
    }
}
