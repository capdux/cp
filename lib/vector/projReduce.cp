// Origin: Pure-CP Stdlib (AI Embedding Dimensionality Reduction: Power-Iter PCA & JL Random Projection using @dot/@fma) | Created: 2026-11-22
// Pure CP zero-heap-allocation embedding dimensionality reduction & random projection kernels:
// mean centering, matrix-free power-iteration Top-K PCA with Modified Gram-Schmidt deflation,
// @dot 8-lane SIMD batch projection, and Achlioptas sparse {-1, 0, +1} Johnson-Lindenstrauss projection.

const std = @import("std");

// ============================================================================
// Internal Numeric, PRNG & Orthogonalization Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteSliceF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn nextSplitMix64(state: *u64): u64 {
    const c_add: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
    const c_m1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
    const c_m2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
    state.* = state.* +% c_add;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c_m1;
    z = (z ^ (z >> 27)) *% c_m2;
    return z ^ (z >> 31);
}

fn nextRandUnitF32(state: *u64): f32 {
    const r: u64 = nextSplitMix64(state) >> 40;
    const u: f32 = (r as f32) / 16777216.0;
    return (u * 2.0) - 1.0;
}

/// Orthogonalizes `vec[0..dim]` in-place against `prev_count` unit axes stored in `axes_flat`
/// (2-pass Modified Gram-Schmidt for f32 numerical stability).
fn orthogonalizeAgainstAxes(
    vec: *[]f32,
    axes_flat: []const f32,
    prev_count: usize,
    dim: usize,
) {
    let pass: usize = 0;
    while (pass < 2) : (pass += 1) {
        let p: usize = 0;
        while (p < prev_count) : (p += 1) {
            const prev_axis: []const f32 = axes_flat[p * dim .. (p + 1) * dim];
            const proj: f32 = @dot(vec[0..dim], prev_axis);
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                vec[d] = @fma((0.0 as f32) - proj, prev_axis[d], vec[d]);
            }
        }
    }
}

/// Normalizes `vec[0..dim]` to unit L2 norm in-place. Returns `false` if norm is ~0.
fn normalizeUnitInPlace(vec: *[]f32, dim: usize): bool {
    const norm_sq: f32 = @dot(vec[0..dim], vec[0..dim]);
    if (!isFiniteF32(norm_sq) || norm_sq <= 1.0e-20) return false;
    const inv_norm: f32 = 1.0 / @sqrt(norm_sq);
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        vec[d] = vec[d] * inv_norm;
    }
    return true;
}

// ============================================================================
// Public Dimensionality Reduction & Random Projection API
// ============================================================================

/// Computes the per-dimension arithmetic mean of a row-major `[n, dim]` `f32` matrix into `out_mean`.
/// Returns `false` on zero dimensions, overflow, undersized buffers, or non-finite inputs.
pub fn computeMeanF32(
    data: []const f32,
    n: usize,
    dim: usize,
    out_mean: *[]f32,
): bool {
    if (n == 0 || dim == 0 || out_mean.len < dim) return false;
    if (n > std.math.maxInt(usize) / dim) return false;
    const total: usize = n * dim;
    if (data.len < total) return false;

    let d0: usize = 0;
    while (d0 < dim) : (d0 += 1) {
        out_mean[d0] = 0.0;
    }

    let rem: []const f32 = data[0..total];
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const [row, next_rem] = rem.split(dim);
        rem = next_rem;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            const v: f32 = row[d];
            if (!isFiniteF32(v)) return false;
            out_mean[d] = out_mean[d] + v;
        }
    }

    const inv_n: f32 = 1.0 / (n as f32);
    let d1: usize = 0;
    while (d1 < dim) : (d1 += 1) {
        out_mean[d1] = out_mean[d1] * inv_n;
    }
    return true;
}

/// Subtracts `mean[0..dim]` from every row of the row-major `[n, dim]` matrix `data` in-place.
/// Returns `false` on zero dimensions, overflow, undersized buffers, or non-finite inputs.
pub fn subtractMeanInPlaceF32(
    data: *[]f32,
    n: usize,
    dim: usize,
    mean: []const f32,
): bool {
    if (n == 0 || dim == 0 || mean.len < dim) return false;
    if (n > std.math.maxInt(usize) / dim) return false;
    const total: usize = n * dim;
    if (data.len < total) return false;
    if (!isFiniteSliceF32(mean[0..dim])) return false;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const base: usize = i * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            const v: f32 = data[base + d];
            if (!isFiniteF32(v)) return false;
            data[base + d] = v - mean[d];
        }
    }
    return true;
}

/// Fits the Top-K orthonormal principal axes and explained variances (eigenvalues of `X^T X / n`)
/// from a mean-centered `[n, dim]` dataset using zero-allocation matrix-free power iteration
/// (`w = X^T (X v) / n`) and 2-pass Modified Gram-Schmidt deflation.
///
/// - Does NOT materialize a `dim x dim` covariance matrix; requires only a `dim`-length `scratch_vec`.
/// - Writes `k` orthonormal row vectors into `out_axes[0 .. k * dim]` and their sample variances
///   into `out_eigenvals[0 .. k]` in descending order.
/// - Returns `false` on invalid dimensions, `k > dim`, `max_iters == 0`, undersized buffers, or non-finite data.
pub fn fitTopKPrincipalAxesF32(
    centered_data: []const f32,
    n: usize,
    dim: usize,
    k: usize,
    max_iters: u32,
    seed: u64,
    out_axes: *[]f32,
    out_eigenvals: *[]f32,
    scratch_vec: *[]f32,
): bool {
    if (n == 0 || dim == 0 || k == 0 || k > dim || max_iters == 0) return false;
    if (n > std.math.maxInt(usize) / dim) return false;
    if (k > std.math.maxInt(usize) / dim) return false;
    const total_data: usize = n * dim;
    const total_axes: usize = k * dim;
    if (centered_data.len < total_data) return false;
    if (out_axes.len < total_axes || out_eigenvals.len < k || scratch_vec.len < dim) return false;
    if (!isFiniteSliceF32(centered_data[0..total_data])) return false;

    const inv_n: f32 = 1.0 / (n as f32);
    let rng: u64 = seed;

    let comp: usize = 0;
    while (comp < k) : (comp += 1) {
        const axis_base: usize = comp * dim;

        // Initialize candidate unit vector `scratch_vec[0..dim]` orthogonal to all previous axes.
        let init_ok: bool = false;
        let d0: usize = 0;
        while (d0 < dim) : (d0 += 1) {
            scratch_vec[d0] = nextRandUnitF32(&rng);
        }
        orthogonalizeAgainstAxes(scratch_vec, out_axes[0..axis_base], comp, dim);
        init_ok = normalizeUnitInPlace(scratch_vec, dim);

        // Deterministic canonical basis fallback if random init collapsed into previous span.
        if (!init_ok) {
            let basis: usize = 0;
            while (basis < dim && !init_ok) : (basis += 1) {
                let z: usize = 0;
                while (z < dim) : (z += 1) {
                    scratch_vec[z] = if (z == basis) 1.0 else 0.0;
                }
                orthogonalizeAgainstAxes(scratch_vec, out_axes[0..axis_base], comp, dim);
                init_ok = normalizeUnitInPlace(scratch_vec, dim);
            }
        }
        if (!init_ok) return false;

        // Copy initial unit vector into `out_axes[axis_base .. axis_base + dim]`.
        let c0: usize = 0;
        while (c0 < dim) : (c0 += 1) {
            out_axes[axis_base + c0] = scratch_vec[c0];
        }

        // Power iteration: w = (X^T (X v)) / n, deflated via Gram-Schmidt against prior axes.
        let it: u32 = 0;
        while (it < max_iters) : (it += 1) {
            let z: usize = 0;
            while (z < dim) : (z += 1) {
                scratch_vec[z] = 0.0;
            }

            const cur_v: []const f32 = out_axes[axis_base .. axis_base + dim];
            let rem: []const f32 = centered_data[0..total_data];
            let row_idx: usize = 0;
            while (row_idx < n) : (row_idx += 1) {
                const [row, next_rem] = rem.split(dim);
                rem = next_rem;
                const proj: f32 = @dot(row, cur_v);
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    scratch_vec[d] = @fma(proj, row[d], scratch_vec[d]);
                }
            }

            let ds: usize = 0;
            while (ds < dim) : (ds += 1) {
                scratch_vec[ds] = scratch_vec[ds] * inv_n;
            }

            orthogonalizeAgainstAxes(scratch_vec, out_axes[0..axis_base], comp, dim);
            if (!normalizeUnitInPlace(scratch_vec, dim)) {
                break;
            }

            const cos_change: f32 = absF32(@dot(cur_v, scratch_vec[0..dim]));
            let cp: usize = 0;
            while (cp < dim) : (cp += 1) {
                out_axes[axis_base + cp] = scratch_vec[cp];
            }
            if (cos_change > 0.9999995) {
                break;
            }
        }

        // Compute exact sample variance along the converged unit axis.
        const final_axis: []const f32 = out_axes[axis_base .. axis_base + dim];
        let var_sum: f32 = 0.0;
        let rem_v: []const f32 = centered_data[0..total_data];
        let r_i: usize = 0;
        while (r_i < n) : (r_i += 1) {
            const [row, next_rem] = rem_v.split(dim);
            rem_v = next_rem;
            const proj: f32 = @dot(row, final_axis);
            var_sum = @fma(proj, proj, var_sum);
        }
        out_eigenvals[comp] = var_sum * inv_n;
    }

    return true;
}

/// Projects a batch of `n` embeddings `[n, in_dim]` onto `out_dim` projection axes `[out_dim, in_dim]`
/// into `out_proj[0 .. n * out_dim]` using CP's native 8-lane SIMD `@dot` intrinsic.
/// Returns `false` on zero dimensions, overflow, undersized buffers, or non-finite results.
pub fn projectEmbeddingsF32(
    data: []const f32,
    n: usize,
    in_dim: usize,
    axes: []const f32,
    out_dim: usize,
    out_proj: *[]f32,
): bool {
    if (n == 0 || in_dim == 0 || out_dim == 0) return false;
    if (n > std.math.maxInt(usize) / in_dim) return false;
    if (out_dim > std.math.maxInt(usize) / in_dim) return false;
    if (n > std.math.maxInt(usize) / out_dim) return false;

    const total_data: usize = n * in_dim;
    const total_axes: usize = out_dim * in_dim;
    const total_out: usize = n * out_dim;
    if (data.len < total_data || axes.len < total_axes || out_proj.len < total_out) return false;

    let rem: []const f32 = data[0..total_data];
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const [row, next_rem] = rem.split(in_dim);
        rem = next_rem;
        const out_base: usize = i * out_dim;
        let j: usize = 0;
        while (j < out_dim) : (j += 1) {
            const axis: []const f32 = axes[j * in_dim .. (j + 1) * in_dim];
            const val: f32 = @dot(row, axis);
            if (!isFiniteF32(val)) return false;
            out_proj[out_base + j] = val;
        }
    }
    return true;
}

/// Applies Achlioptas's sparse `{-1, 0, +1}` deterministic Johnson-Lindenstrauss random projection
/// (P(+1) = 1/6, P(-1) = 1/6, P(0) = 2/3, scaled by `sqrt(3 / out_dim)`) from `in_dim` to `out_dim`
/// with zero stored projection matrix.
/// Preserves pairwise Euclidean distances in expectation (`E[||R x||^2] == ||x||^2`).
/// Returns `false` on zero dimensions, undersized buffers, or non-finite input values.
pub fn jlRandomProjectF32(
    vec: []const f32,
    in_dim: usize,
    out_dim: usize,
    seed: u64,
    out_proj: *[]f32,
): bool {
    if (in_dim == 0 || out_dim == 0) return false;
    if (vec.len < in_dim || out_proj.len < out_dim) return false;
    if (!isFiniteSliceF32(vec[0..in_dim])) return false;

    const scale: f32 = @sqrt(3.0 / (out_dim as f32));
    let state: u64 = seed;

    let j: usize = 0;
    while (j < out_dim) : (j += 1) {
        let acc: f32 = 0.0;
        let i: usize = 0;
        while (i < in_dim) : (i += 1) {
            const r: u64 = nextSplitMix64(&state) % (6 as u64);
            if (r == (0 as u64)) {
                acc += vec[i];
            } else if (r == (1 as u64)) {
                acc -= vec[i];
            }
        }
        out_proj[j] = acc * scale;
    }

    return true;
}
