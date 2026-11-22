// Origin: Pure-CP Stdlib (AI-Native Tiled GEMM & Batched MatMul using @dot/@fma/@splitAtMut) | Created: 2026-11-22
// Pure CP zero-allocation cache-friendly tiled GEMM, batched multi-head QK^T MatMul,
// and zero-copy mutually-exclusive row-split GEMM powered by @dot, @fma, and @splitAtMut.

const std = @import("std");

const GEMM_TILE: usize = 16;

// ============================================================================
// Internal Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn minUsize(a: usize, b: usize): usize {
    if (a < b) {
        return a;
    }
    return b;
}

fn checkedMul(a: usize, b: usize): usize? {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > std.math.maxInt(usize) / b) {
        return null;
    }
    return a * b;
}

// ============================================================================
// 1. Transposed-B SIMD GEMM: C = alpha * (A * B^T) + beta * C
// ============================================================================

/// Computes `C = alpha * (A * B^T) + beta * C` where `A` is `[m, k]`, `B^T` is `[n, k]`
/// (row-contiguous), and `C` is `[m, n]`.
/// Uses cache-friendly 2D tiling over `(m, n)` with 8-lane SIMD `@dot` and hardware `@fma`.
/// Follows BLAS convention: when `beta == 0.0`, existing `C` elements are never read.
pub fn gemmTransposedBF32(
    a: []const f32,
    b_t: []const f32,
    m: usize,
    n: usize,
    k: usize,
    alpha: f32,
    beta: f32,
    c: *[]f32,
): bool {
    if (m == 0 || n == 0 || k == 0) {
        return false;
    }
    if (!isFiniteF32(alpha) || !isFiniteF32(beta)) {
        return false;
    }
    const mk: usize = checkedMul(m, k) ?? return false;
    const nk: usize = checkedMul(n, k) ?? return false;
    const mn: usize = checkedMul(m, n) ?? return false;
    if (a.len < mk || b_t.len < nk || c.len < mn) {
        return false;
    }

    const zero_beta: bool = beta == 0.0;
    let row0: usize = 0;
    while (row0 < m) : (row0 += GEMM_TILE) {
        const i_end: usize = minUsize(row0 + GEMM_TILE, m);
        let col0: usize = 0;
        while (col0 < n) : (col0 += GEMM_TILE) {
            const j_end: usize = minUsize(col0 + GEMM_TILE, n);

            let i: usize = row0;
            while (i < i_end) : (i += 1) {
                const a_row = a[i * k .. (i + 1) * k];
                const c_row_base: usize = i * n;
                let j: usize = col0;
                while (j < j_end) : (j += 1) {
                    const b_row = b_t[j * k .. (j + 1) * k];
                    const dot_val: f32 = @dot(a_row, b_row);
                    if (zero_beta) {
                        c[c_row_base + j] = alpha * dot_val;
                    } else {
                        c[c_row_base + j] = @fma(alpha, dot_val, beta * c[c_row_base + j]);
                    }
                }
            }
        }
    }
    return true;
}

// ============================================================================
// 2. Cache-Friendly Tiled ikj GEMM: C = alpha * (A * B) + beta * C
// ============================================================================

/// Computes `C = alpha * (A * B) + beta * C` where `A` is `[m, k]`, `B` is `[k, n]`,
/// and `C` is `[m, n]` in row-major layout.
/// Uses cache-friendly 3D tiled `i-p-j` traversal with sequential `B` and `C` row accesses
/// and hardware `@fma` accumulation.
pub fn gemmF32(
    a: []const f32,
    b: []const f32,
    m: usize,
    n: usize,
    k: usize,
    alpha: f32,
    beta: f32,
    c: *[]f32,
): bool {
    if (m == 0 || n == 0 || k == 0) {
        return false;
    }
    if (!isFiniteF32(alpha) || !isFiniteF32(beta)) {
        return false;
    }
    const mk: usize = checkedMul(m, k) ?? return false;
    const kn: usize = checkedMul(k, n) ?? return false;
    const mn: usize = checkedMul(m, n) ?? return false;
    if (a.len < mk || b.len < kn || c.len < mn) {
        return false;
    }

    // Prepare C according to BLAS beta semantics.
    if (beta == 0.0) {
        let idx: usize = 0;
        while (idx < mn) : (idx += 1) {
            c[idx] = 0.0;
        }
    } else if (beta != 1.0) {
        let idx: usize = 0;
        while (idx < mn) : (idx += 1) {
            c[idx] = c[idx] * beta;
        }
    }

    if (alpha == 0.0) {
        return true;
    }

    // 3D tiled ikj loop keeps B[p, col0..j_end] and C[i, col0..j_end] L1-cache resident.
    let row0: usize = 0;
    while (row0 < m) : (row0 += GEMM_TILE) {
        const i_end: usize = minUsize(row0 + GEMM_TILE, m);
        let p0: usize = 0;
        while (p0 < k) : (p0 += GEMM_TILE) {
            const p_end: usize = minUsize(p0 + GEMM_TILE, k);
            let col0: usize = 0;
            while (col0 < n) : (col0 += GEMM_TILE) {
                const j_end: usize = minUsize(col0 + GEMM_TILE, n);

                let i: usize = row0;
                while (i < i_end) : (i += 1) {
                    const a_row_base: usize = i * k;
                    const c_row_base: usize = i * n;
                    let p: usize = p0;
                    while (p < p_end) : (p += 1) {
                        const a_scaled: f32 = alpha * a[a_row_base + p];
                        if (a_scaled == 0.0) {
                            continue;
                        }
                        const b_row_base: usize = p * n;
                        let j: usize = col0;
                        while (j < j_end) : (j += 1) {
                            c[c_row_base + j] = @fma(a_scaled, b[b_row_base + j], c[c_row_base + j]);
                        }
                    }
                }
            }
        }
    }
    return true;
}

// ============================================================================
// 3. Batched Transposed-B GEMM (Multi-Head Attention Q * K^T)
// ============================================================================

/// Computes `batch` independent matrix products `C[h] = A[h] * B^T[h]` for `h in 0..batch`,
/// where each `A[h]` is `[m, k]`, `B^T[h]` is `[n, k]`, and `C[h]` is `[m, n]`.
pub fn batchedGemmTransposedBF32(
    a: []const f32,
    b_t: []const f32,
    batch: usize,
    m: usize,
    n: usize,
    k: usize,
    c: *[]f32,
): bool {
    if (batch == 0 || m == 0 || n == 0 || k == 0) {
        return false;
    }
    const mk: usize = checkedMul(m, k) ?? return false;
    const nk: usize = checkedMul(n, k) ?? return false;
    const mn: usize = checkedMul(m, n) ?? return false;
    const total_a: usize = checkedMul(batch, mk) ?? return false;
    const total_b: usize = checkedMul(batch, nk) ?? return false;
    const total_c: usize = checkedMul(batch, mn) ?? return false;

    if (a.len < total_a || b_t.len < total_b || c.len < total_c) {
        return false;
    }

    let h: usize = 0;
    while (h < batch) : (h += 1) {
        const a_slice = a[h * mk .. (h + 1) * mk];
        const b_slice = b_t[h * nk .. (h + 1) * nk];
        let c_slice = c[h * mn .. (h + 1) * mn];
        if (!gemmTransposedBF32(a_slice, b_slice, m, n, k, 1.0, 0.0, c_slice)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// 4. Zero-Copy Row-Split GEMM via @splitAtMut
// ============================================================================

/// Computes `C = A * B^T` (`[m, k] * [n, k]^T -> [m, n]`) by splitting `C` at `split_row`
/// into two mutually-exclusive mutable sub-slices via `@splitAtMut`.
/// Demonstrates zero-copy disjoint output partitioning compatible with parallel row workers.
pub fn splitGemmRowsF32(
    a: []const f32,
    b_t: []const f32,
    m: usize,
    n: usize,
    k: usize,
    split_row: usize,
    c: *[]f32,
): bool {
    if (m == 0 || n == 0 || k == 0 || split_row > m) {
        return false;
    }
    const mk: usize = checkedMul(m, k) ?? return false;
    const nk: usize = checkedMul(n, k) ?? return false;
    const mn: usize = checkedMul(m, n) ?? return false;
    if (a.len < mk || b_t.len < nk || c.len < mn) {
        return false;
    }

    let [c_top, c_bot] = @splitAtMut(c[0..mn], split_row * n);

    if (split_row > 0) {
        if (!gemmTransposedBF32(a[0 .. split_row * k], b_t[0..nk], split_row, n, k, 1.0, 0.0, c_top)) {
            return false;
        }
    }
    const bot_rows: usize = m - split_row;
    if (bot_rows > 0) {
        if (!gemmTransposedBF32(a[split_row * k .. mk], b_t[0..nk], bot_rows, n, k, 1.0, 0.0, c_bot)) {
            return false;
        }
    }
    return true;
}
