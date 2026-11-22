// Origin: Pure-CP Stdlib (AI Inference Block-Scaled FP8 E4M3 Quantization & Fused GEMV using @fma) | Created: 2026-11-22
// Pure CP zero-heap-allocation DeepSeek-V3 / H100 style block-scaled FP8 E4M3FN quantization,
// per-block dynamic amax scaling (with zero-block and tail-block safety), and scale-hoisted
// mixed-precision fused GEMV powered by hardware `@fma` and `slice.split`.

const std = @import("std");
import { f32ToFp8E4m3, fp8E4m3ToF32 } from "./fp16.cp";

/// Maximum finite magnitude representable in OCP / NVIDIA FP8 E4M3FN (`0x7E` = 448.0).
pub const FP8_E4M3_MAX_FINITE: f32 = 448.0;

/// Per-block dynamic scaling descriptor for FP8 E4M3 quantization.
pub struct Fp8BlockScale {
    scale: f32 = 0.0;
    inv_scale: f32 = 0.0;
    block_size: u16 = 0;

    /// Constructs an `Fp8BlockScale` from a block's absolute maximum `amax`.
    /// Safely handles `amax == 0.0` (zero block) with `scale = 0.0, inv_scale = 0.0`.
    /// Returns `null` if `block_size == 0`, `amax < 0.0`, or `amax` is non-finite.
    static fn fromAmax(amax: f32, block_size: u16): Fp8BlockScale? {
        if (block_size == 0 || !isFiniteF32(amax) || amax < 0.0) return null;
        if (amax == 0.0) {
            return Fp8BlockScale{
                scale: 0.0,
                inv_scale: 0.0,
                block_size: block_size,
            };
        }
        return Fp8BlockScale{
            scale: amax / FP8_E4M3_MAX_FINITE,
            inv_scale: FP8_E4M3_MAX_FINITE / amax,
            block_size: block_size,
        };
    }
}

// ============================================================================
// Internal Numeric & Block Geometry Helpers
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

fn blocksForLen(len: usize, block_size: usize): usize {
    return ((len - 1) / block_size) + 1;
}

// ============================================================================
// Public Block-Scaled FP8 E4M3 Quantization & Fused GEMV API
// ============================================================================

/// Computes the per-block absolute maximum (`amax = max(|v|)`) over `src` in blocks of `block_size`
/// (supporting non-divisible tail blocks).
/// Returns `false` on empty input, `block_size == 0`, undersized `out_amax`, or non-finite values.
pub fn computeBlockAmaxF32(
    src: []const f32,
    block_size: usize,
    out_amax: *[]f32,
): bool {
    if (src.len == 0 || block_size == 0) return false;
    const n_blocks: usize = blocksForLen(src.len, block_size);
    if (out_amax.len < n_blocks) return false;

    let rem: []const f32 = src;
    let b: usize = 0;
    while (b < n_blocks) : (b += 1) {
        const chunk_len: usize = if (rem.len < block_size) rem.len else block_size;
        const [chunk, next_rem] = rem.split(chunk_len);
        rem = next_rem;

        let amax: f32 = 0.0;
        let i: usize = 0;
        while (i < chunk_len) : (i += 1) {
            const v: f32 = chunk[i];
            if (!isFiniteF32(v)) return false;
            const av: f32 = absF32(v);
            if (av > amax) amax = av;
        }
        out_amax[b] = amax;
    }

    return true;
}

/// Quantizes `src` into block-scaled FP8 E4M3FN bytes (`out_q`) and per-block dynamic scales
/// (`out_scales[b] = amax_b / 448.0`), handling all-zero blocks safely without division by zero
/// and supporting arbitrary non-divisible tail blocks.
/// Returns `false` on empty input, `block_size == 0`, undersized outputs, or non-finite values.
pub fn quantizeBlockFp8E4m3(
    src: []const f32,
    block_size: usize,
    out_q: *[]u8,
    out_scales: *[]f32,
): bool {
    if (src.len == 0 || block_size == 0) return false;
    const n_blocks: usize = blocksForLen(src.len, block_size);
    if (out_q.len < src.len || out_scales.len < n_blocks) return false;

    let rem: []const f32 = src;
    let offset: usize = 0;
    let b: usize = 0;
    while (b < n_blocks) : (b += 1) {
        const chunk_len: usize = if (rem.len < block_size) rem.len else block_size;
        const [chunk, next_rem] = rem.split(chunk_len);
        rem = next_rem;

        let amax: f32 = 0.0;
        let i: usize = 0;
        while (i < chunk_len) : (i += 1) {
            const v: f32 = chunk[i];
            if (!isFiniteF32(v)) return false;
            const av: f32 = absF32(v);
            if (av > amax) amax = av;
        }

        if (amax == 0.0) {
            out_scales[b] = 0.0;
            let z: usize = 0;
            while (z < chunk_len) : (z += 1) {
                out_q[offset + z] = 0;
            }
        } else {
            const scale: f32 = amax / FP8_E4M3_MAX_FINITE;
            const inv_scale: f32 = FP8_E4M3_MAX_FINITE / amax;
            out_scales[b] = scale;
            let j: usize = 0;
            while (j < chunk_len) : (j += 1) {
                out_q[offset + j] = f32ToFp8E4m3(chunk[j] * inv_scale);
            }
        }
        offset += chunk_len;
    }

    return true;
}

/// Dequantizes block-scaled FP8 E4M3FN bytes `q` with per-block `scales` into `out_dst`.
/// Supports non-divisible tail blocks and all-zero blocks (`scale == 0.0`).
/// Returns `false` on empty input, `block_size == 0`, negative/non-finite scale, or undersized buffers.
pub fn dequantizeBlockFp8E4m3(
    q: []const u8,
    scales: []const f32,
    block_size: usize,
    out_dst: *[]f32,
): bool {
    if (q.len == 0 || block_size == 0) return false;
    const n_blocks: usize = blocksForLen(q.len, block_size);
    if (scales.len < n_blocks || out_dst.len < q.len) return false;

    let rem: []const u8 = q;
    let offset: usize = 0;
    let b: usize = 0;
    while (b < n_blocks) : (b += 1) {
        const chunk_len: usize = if (rem.len < block_size) rem.len else block_size;
        const [chunk, next_rem] = rem.split(chunk_len);
        rem = next_rem;

        const scale: f32 = scales[b];
        if (!isFiniteF32(scale) || scale < 0.0) return false;

        if (scale == 0.0) {
            let z: usize = 0;
            while (z < chunk_len) : (z += 1) {
                out_dst[offset + z] = 0.0;
            }
        } else {
            let j: usize = 0;
            while (j < chunk_len) : (j += 1) {
                const decoded: f32 = fp8E4m3ToF32(chunk[j]);
                if (!isFiniteF32(decoded)) return false;
                out_dst[offset + j] = decoded * scale;
            }
        }
        offset += chunk_len;
    }

    return true;
}

/// Scale-hoisted mixed-precision fused GEMV (`out_y = W_fp8 * x`) without materializing a
/// temporary dequantized FP32 weight matrix.
///
/// - `w_q` stores `[rows, cols]` row-major FP8 E4M3FN bytes quantized per-row in blocks of `block_size`.
/// - `w_scales` stores `[rows, ceil(cols / block_size)]` per-block dynamic scales.
/// - Accumulates block inner products via hardware `@fma` and hoists the block scale multiplication
///   outside the inner loop (`row_acc = @fma(block_acc, w_scale, row_acc)`).
/// - Returns `false` on zero dims, overflow, undersized buffers, negative/non-finite scales, or NaN/Inf.
pub fn gemvBlockFp8E4m3(
    x: []const f32,
    w_q: []const u8,
    w_scales: []const f32,
    rows: usize,
    cols: usize,
    block_size: usize,
    out_y: *[]f32,
): bool {
    if (rows == 0 || cols == 0 || block_size == 0) return false;
    if (x.len < cols || out_y.len < rows) return false;
    if (rows > std.math.maxInt(usize) / cols) return false;
    const total_w: usize = rows * cols;
    if (w_q.len < total_w) return false;

    const blocks_per_row: usize = blocksForLen(cols, block_size);
    if (rows > std.math.maxInt(usize) / blocks_per_row) return false;
    const total_scales: usize = rows * blocks_per_row;
    if (w_scales.len < total_scales) return false;
    if (!isFiniteSliceF32(x[0..cols])) return false;

    let w_rem: []const u8 = w_q[0..total_w];
    let s_rem: []const f32 = w_scales[0..total_scales];

    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const [row_q, next_w_rem] = w_rem.split(cols);
        const [row_scales, next_s_rem] = s_rem.split(blocks_per_row);
        w_rem = next_w_rem;
        s_rem = next_s_rem;

        let row_acc: f32 = 0.0;
        let q_rem: []const u8 = row_q;
        let col_base: usize = 0;
        let b: usize = 0;
        while (b < blocks_per_row) : (b += 1) {
            const chunk_len: usize = if (q_rem.len < block_size) q_rem.len else block_size;
            const [q_chunk, next_q] = q_rem.split(chunk_len);
            q_rem = next_q;

            const scale: f32 = row_scales[b];
            if (!isFiniteF32(scale) || scale < 0.0) return false;

            if (scale > 0.0) {
                let block_acc: f32 = 0.0;
                let j: usize = 0;
                while (j < chunk_len) : (j += 1) {
                    const w_val: f32 = fp8E4m3ToF32(q_chunk[j]);
                    if (!isFiniteF32(w_val)) return false;
                    block_acc = @fma(x[col_base + j], w_val, block_acc);
                }
                row_acc = @fma(block_acc, scale, row_acc);
            }
            col_base += chunk_len;
        }

        if (!isFiniteF32(row_acc)) return false;
        out_y[r] = row_acc;
    }

    return true;
}
