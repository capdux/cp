// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc AWQ Activation-Aware INT4 Group-Quantized GEMV Kernel using @dot/@fma) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation AWQ (Activation-Aware Weight Quantization, MLSys 2024) engine:
//   - Computes per-channel salient smoothing scales `s_j = mean(|x_j|)^alpha` from calibration activations
//     (`computeAwqChannelScales`) with NaN/Inf sanitization and numerical clamping.
//   - Group-wise asymmetric INT4 weight packing (2 x 4-bit unsigned nibbles per byte, per-group `scale`
//     and `zero_point`) with AWQ channel pre-scaling (`awqQuantizeMatrix`, `awqDequantizeRow`).
//   - Fused zero-heap GEMV kernel (`awqGemvF32`): pre-scales input activation by `1 / s_j` into caller
//     scratch and executes tiled 8-lane SIMD `@dot` + hardware `@fma` group accumulation.
//   - Row-wise quantization error & cosine fidelity diagnostics (`evaluateAwqQuantError`).
//   - Full defensive guards: invalid group_size, odd/zero dims, undersized buffers, NaN/Inf sanitization,
//     and zero-division safe constant/zero weight groups.

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const AWQ_MIN_CHANNEL_SCALE: f32 = 1.0e-4;
pub const AWQ_MAX_CHANNEL_SCALE: f32 = 1.0e4;

const AWQ_RANGE_EPS: f32 = 1.0e-12;
const AWQ_TILE_ELEMS: usize = 32;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn sanitizeF32(x: f32): f32 {
    if (isFiniteF32(x)) {
        return x;
    }
    return 0.0;
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn roundToU8Clamp15(x: f32): u8 {
    if (!isFiniteF32(x) || x <= 0.0) {
        return 0;
    }
    if (x >= 15.0) {
        return 15;
    }
    const r: i32 = @intFromFloat(@round(x));
    if (r <= 0) return 0;
    if (r >= 15) return 15;
    return r as u8;
}

fn clampScaleF32(s: f32): f32 {
    if (!isFiniteF32(s) || s <= AWQ_RANGE_EPS) {
        return 1.0;
    }
    if (s < AWQ_MIN_CHANNEL_SCALE) {
        return AWQ_MIN_CHANNEL_SCALE;
    }
    if (s > AWQ_MAX_CHANNEL_SCALE) {
        return AWQ_MAX_CHANNEL_SCALE;
    }
    return s;
}

fn checkMulOverflow(a: usize, b: usize): bool {
    if (a == 0 || b == 0) {
        return false;
    }
    return a > std.math.maxInt(usize) / b;
}

fn safeCosineF32(a: []const f32, b: []const f32): f32 {
    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    if (!isFiniteF32(na) || !isFiniteF32(nb) || na <= AWQ_RANGE_EPS || nb <= AWQ_RANGE_EPS) {
        if (na <= AWQ_RANGE_EPS && nb <= AWQ_RANGE_EPS) {
            return 1.0;
        }
        return 0.0;
    }
    const denom = @sqrt(na) * @sqrt(nb);
    if (!isFiniteF32(denom) || denom <= 0.0) {
        return 0.0;
    }
    const c = @dot(a, b) / denom;
    if (!isFiniteF32(c)) return 0.0;
    if (c > 1.0) return 1.0;
    if (c < -1.0) return -1.0;
    return c;
}

// ============================================================================
// 1. Public Data Models & Geometry Helpers
// ============================================================================

/// Per-group asymmetric INT4 quantization metadata.
pub struct AwqGroupMeta {
    scale: f32 = 0.0;
    zero_point: u8 = 0;
}

/// Quantization fidelity and error diagnostics produced by `evaluateAwqQuantError`.
pub struct AwqQuantErrorStats {
    rows: usize = 0;
    cols: usize = 0;
    mse: f32 = 0.0;
    max_abs_err: f32 = 0.0;
    min_row_cosine: f32 = 1.0;
    mean_row_cosine: f32 = 1.0;
}

/// Returns the number of quantization groups per row: `ceil(cols / group_size)`.
pub fn awqGroupsPerRow(cols: usize, group_size: usize): usize {
    if (cols == 0 || group_size == 0) {
        return 0;
    }
    return (cols + group_size - 1) / group_size;
}

/// Returns the packed INT4 byte count per row (`cols / 2`).
pub fn awqPackedBytesPerRow(cols: usize): usize {
    return cols / 2;
}

// ============================================================================
// 2. Activation-Aware Salient Channel Scale Computation
// ============================================================================

/// Computes AWQ per-channel smoothing scales `s_j = clamp(mean(|x_j|)^alpha, min, max)` over
/// calibration activations `activations` (row-major `[num_samples, cols]`).
///
/// - Sanitizes any `NaN`/`Inf` activation values to `0.0`.
/// - Channels with near-zero mean activation receive neutral scale `1.0`.
/// - Returns `false` on invalid `alpha` (outside `[0.0, 1.0]` or non-finite), zero dimensions,
///   overflow, or undersized buffers.
pub fn computeAwqChannelScales(
    activations: []const f32,
    num_samples: usize,
    cols: usize,
    alpha: f32,
    out_scales: *[]f32,
): bool {
    if (num_samples == 0 || cols == 0 || out_scales.len < cols) {
        return false;
    }
    if (!isFiniteF32(alpha) || alpha < 0.0 || alpha > 1.0) {
        return false;
    }
    if (checkMulOverflow(num_samples, cols) || activations.len < num_samples * cols) {
        return false;
    }

    let j: usize = 0;
    while (j < cols) : (j += 1) {
        out_scales[j] = 0.0;
    }

    const inv_samples: f32 = 1.0 / (num_samples as f32);
    let n: usize = 0;
    while (n < num_samples) : (n += 1) {
        const row_off = n * cols;
        j = 0;
        while (j < cols) : (j += 1) {
            const v = absF32(sanitizeF32(activations[row_off + j]));
            out_scales[j] = @fma(inv_samples, v, out_scales[j]);
        }
    }

    j = 0;
    while (j < cols) : (j += 1) {
        const mean_abs = out_scales[j];
        if (!isFiniteF32(mean_abs) || mean_abs <= AWQ_RANGE_EPS) {
            out_scales[j] = 1.0;
        } else if (alpha == 0.0) {
            out_scales[j] = 1.0;
        } else if (alpha == 0.5) {
            out_scales[j] = clampScaleF32(@sqrt(mean_abs));
        } else if (alpha == 1.0) {
            out_scales[j] = clampScaleF32(mean_abs);
        } else {
            out_scales[j] = clampScaleF32(std.math.pow(f32, mean_abs, alpha));
        }
    }

    return true;
}

// ============================================================================
// 3. AWQ Group-Wise Asymmetric INT4 Matrix Quantization & Row Dequantization
// ============================================================================

/// Quantizes `weights` (row-major `[rows, cols]`) into group-wise asymmetric INT4 packed nibbles
/// after applying per-channel AWQ scales `w'[r, j] = weights[r, j] * channel_scales[j]`.
///
/// - Requires `cols > 0`, `cols % 2 == 0`, `rows > 0`, `group_size > 0`.
/// - Packs even column `2k` into low 4 bits and odd column `2k + 1` into high 4 bits.
/// - Safely handles all-zero and constant weight groups without division by zero.
pub fn awqQuantizeMatrix(
    weights: []const f32,
    channel_scales: []const f32,
    rows: usize,
    cols: usize,
    group_size: usize,
    out_packed: *[]u8,
    out_groups: *[]AwqGroupMeta,
): bool {
    if (rows == 0 || cols == 0 || (cols % 2) != 0 || group_size == 0) {
        return false;
    }
    if (channel_scales.len < cols) {
        return false;
    }
    const groups_per_row = awqGroupsPerRow(cols, group_size);
    const bytes_per_row = awqPackedBytesPerRow(cols);
    if (checkMulOverflow(rows, cols) || checkMulOverflow(rows, bytes_per_row) || checkMulOverflow(rows, groups_per_row)) {
        return false;
    }
    if (weights.len < rows * cols || out_packed.len < rows * bytes_per_row || out_groups.len < rows * groups_per_row) {
        return false;
    }

    // Pre-zero packed output region
    let p: usize = 0;
    while (p < rows * bytes_per_row) : (p += 1) {
        out_packed[p] = 0;
    }

    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const w_row_off = r * cols;
        const p_row_off = r * bytes_per_row;
        const g_row_off = r * groups_per_row;

        let g: usize = 0;
        while (g < groups_per_row) : (g += 1) {
            const c_start = g * group_size;
            let c_end = c_start + group_size;
            if (c_end > cols) {
                c_end = cols;
            }

            // 1. Find [w_min, w_max] of AWQ-scaled weights in this group
            let w_min: f32 = 0.0;
            let w_max: f32 = 0.0;
            let c: usize = c_start;
            while (c < c_end) : (c += 1) {
                const s = clampScaleF32(channel_scales[c]);
                const ws = sanitizeF32(weights[w_row_off + c]) * s;
                if (c == c_start) {
                    w_min = ws;
                    w_max = ws;
                } else {
                    if (ws < w_min) w_min = ws;
                    if (ws > w_max) w_max = ws;
                }
            }

            // Include 0.0 in [lo, hi] so unsigned zero_point in [0..15] always represents 0.0 exactly
            const lo: f32 = if (w_min < 0.0) w_min else 0.0;
            const hi: f32 = if (w_max > 0.0) w_max else 0.0;
            const span: f32 = hi - lo;

            let g_scale: f32 = 0.0;
            let g_zp: u8 = 0;
            if (isFiniteF32(span) && span > AWQ_RANGE_EPS) {
                g_scale = span / 15.0;
                g_zp = roundToU8Clamp15((0.0 - lo) / g_scale);
            }

            out_groups[g_row_off + g] = AwqGroupMeta{
                scale: g_scale,
                zero_point: g_zp,
            };

            // 2. Quantize & pack nibbles in [c_start, c_end)
            const inv_scale: f32 = if (g_scale > AWQ_RANGE_EPS) (1.0 / g_scale) else 0.0;
            const zp_f: f32 = g_zp as f32;
            c = c_start;
            while (c < c_end) : (c += 1) {
                let q: u8 = 0;
                if (inv_scale > 0.0) {
                    const s = clampScaleF32(channel_scales[c]);
                    const ws = sanitizeF32(weights[w_row_off + c]) * s;
                    q = roundToU8Clamp15(@fma(ws, inv_scale, zp_f));
                }
                const byte_idx = p_row_off + (c / 2);
                if ((c & 1) == 0) {
                    out_packed[byte_idx] = (out_packed[byte_idx] & 0xF0) | (q & 0x0F);
                } else {
                    out_packed[byte_idx] = (out_packed[byte_idx] & 0x0F) | ((q & 0x0F) << 4);
                }
            }
        }
    }

    return true;
}

/// Dequantizes a single AWQ-packed row `row_idx` into `out_row[0..cols]`, applying inverse
/// channel scaling `((q - zp) * scale) / channel_scales[j]`.
pub fn awqDequantizeRow(
    packed_weights: []const u8,
    groups: []const AwqGroupMeta,
    channel_scales: []const f32,
    row_idx: usize,
    rows: usize,
    cols: usize,
    group_size: usize,
    out_row: *[]f32,
): bool {
    if (rows == 0 || cols == 0 || (cols % 2) != 0 || group_size == 0 || row_idx >= rows) {
        return false;
    }
    if (channel_scales.len < cols || out_row.len < cols) {
        return false;
    }
    const groups_per_row = awqGroupsPerRow(cols, group_size);
    const bytes_per_row = awqPackedBytesPerRow(cols);
    if (checkMulOverflow(rows, bytes_per_row) || checkMulOverflow(rows, groups_per_row)) {
        return false;
    }
    if (packed_weights.len < rows * bytes_per_row || groups.len < rows * groups_per_row) {
        return false;
    }

    const p_row_off = row_idx * bytes_per_row;
    const g_row_off = row_idx * groups_per_row;

    let g: usize = 0;
    while (g < groups_per_row) : (g += 1) {
        const meta = groups[g_row_off + g];
        const zp_i: i32 = meta.zero_point as i32;
        const c_start = g * group_size;
        let c_end = c_start + group_size;
        if (c_end > cols) {
            c_end = cols;
        }
        let c: usize = c_start;
        while (c < c_end) : (c += 1) {
            const packed_byte = packed_weights[p_row_off + (c / 2)];
            const q_u8: u8 = if ((c & 1) == 0) (packed_byte & 0x0F) else ((packed_byte >> 4) & 0x0F);
            const diff_f: f32 = ((q_u8 as i32) - zp_i) as f32;
            const s = clampScaleF32(channel_scales[c]);
            out_row[c] = (diff_f * meta.scale) / s;
        }
    }

    return true;
}

// ============================================================================
// 4. Fused Zero-Heap AWQ INT4 GEMV Kernel (`@dot` + `@fma`)
// ============================================================================

/// Executes fused zero-heap-allocation AWQ INT4 matrix-vector multiplication `out_y = W_awq * x`:
///   1. Pre-scales input activation `x` into `scaled_x_scratch[j] = sanitize(x[j]) / channel_scales[j]`.
///   2. For each row & quantization group, unpacks INT4 deltas `(q - zp)` into a 32-element stack tile
///      and accumulates `row_acc = @fma(group_scale, @dot(tile, scaled_x_slice), row_acc)`.
///
/// Returns `false` on invalid dimensions, overflow, or undersized buffers.
pub fn awqGemvF32(
    packed_weights: []const u8,
    groups: []const AwqGroupMeta,
    channel_scales: []const f32,
    x: []const f32,
    rows: usize,
    cols: usize,
    group_size: usize,
    scaled_x_scratch: *[]f32,
    out_y: *[]f32,
): bool {
    if (rows == 0 || cols == 0 || (cols % 2) != 0 || group_size == 0) {
        return false;
    }
    if (channel_scales.len < cols || x.len < cols || scaled_x_scratch.len < cols || out_y.len < rows) {
        return false;
    }
    const groups_per_row = awqGroupsPerRow(cols, group_size);
    const bytes_per_row = awqPackedBytesPerRow(cols);
    if (checkMulOverflow(rows, bytes_per_row) || checkMulOverflow(rows, groups_per_row)) {
        return false;
    }
    if (packed_weights.len < rows * bytes_per_row || groups.len < rows * groups_per_row) {
        return false;
    }

    // 1. Pre-scale activation vector x[j] / s_j once (amortized across all `rows`)
    let j: usize = 0;
    while (j < cols) : (j += 1) {
        const s = clampScaleF32(channel_scales[j]);
        scaled_x_scratch[j] = sanitizeF32(x[j]) / s;
    }

    // 2. Tiled SIMD @dot + @fma GEMV over AWQ-quantized rows
    let tile_buf: [32]f32 = [0.0; 32];
    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const p_row_off = r * bytes_per_row;
        const g_row_off = r * groups_per_row;
        let row_acc: f32 = 0.0;

        let g: usize = 0;
        while (g < groups_per_row) : (g += 1) {
            const meta = groups[g_row_off + g];
            if (!isFiniteF32(meta.scale) || meta.scale <= AWQ_RANGE_EPS) {
                continue;
            }
            const zp_i: i32 = meta.zero_point as i32;
            const c_start = g * group_size;
            let c_end = c_start + group_size;
            if (c_end > cols) {
                c_end = cols;
            }

            let tile_start = c_start;
            while (tile_start < c_end) {
                let tile_end = tile_start + AWQ_TILE_ELEMS;
                if (tile_end > c_end) {
                    tile_end = c_end;
                }
                const tile_len = tile_end - tile_start;
                let k: usize = 0;
                while (k < tile_len) : (k += 1) {
                    const col = tile_start + k;
                    const packed_byte = packed_weights[p_row_off + (col / 2)];
                    const q_u8: u8 = if ((col & 1) == 0) (packed_byte & 0x0F) else ((packed_byte >> 4) & 0x0F);
                    tile_buf[k] = ((q_u8 as i32) - zp_i) as f32;
                }
                const tile_dot: f32 = @dot(tile_buf[0..tile_len], scaled_x_scratch[tile_start..tile_end]);
                row_acc = @fma(meta.scale, tile_dot, row_acc);
                tile_start = tile_end;
            }
        }

        out_y[r] = if (isFiniteF32(row_acc)) row_acc else 0.0;
    }

    return true;
}

// ============================================================================
// 5. Quantization Fidelity & Error Diagnostics
// ============================================================================

/// Evaluates element-wise MSE, maximum absolute error, and per-row cosine fidelity between
/// `orig_weights` and the AWQ-quantized matrix using caller-provided `row_scratch[0..cols]`.
pub fn evaluateAwqQuantError(
    orig_weights: []const f32,
    packed_weights: []const u8,
    groups: []const AwqGroupMeta,
    channel_scales: []const f32,
    rows: usize,
    cols: usize,
    group_size: usize,
    row_scratch: *[]f32,
): AwqQuantErrorStats? {
    if (rows == 0 || cols == 0 || (cols % 2) != 0 || group_size == 0) {
        return null;
    }
    if (checkMulOverflow(rows, cols) || orig_weights.len < rows * cols || row_scratch.len < cols) {
        return null;
    }

    let sum_sq_err: f64 = 0.0;
    let max_abs: f32 = 0.0;
    let min_cos: f32 = 1.0;
    let sum_cos: f64 = 0.0;

    let r: usize = 0;
    while (r < rows) : (r += 1) {
        if (!awqDequantizeRow(packed_weights, groups, channel_scales, r, rows, cols, group_size, *row_scratch)) {
            return null;
        }
        const orig_row = orig_weights[r * cols .. (r + 1) * cols];
        let c: usize = 0;
        while (c < cols) : (c += 1) {
            const orig_v = sanitizeF32(orig_row[c]);
            const diff = absF32(orig_v - row_scratch[c]);
            if (diff > max_abs) {
                max_abs = diff;
            }
            const d64 = diff as f64;
            sum_sq_err += d64 * d64;
        }
        const cos = safeCosineF32(orig_row, row_scratch[0..cols]);
        if (cos < min_cos) {
            min_cos = cos;
        }
        sum_cos += cos as f64;
    }

    const total_elems = (rows * cols) as f64;
    return AwqQuantErrorStats{
        rows: rows,
        cols: cols,
        mse: (sum_sq_err / total_elems) as f32,
        max_abs_err: max_abs,
        min_row_cosine: min_cos,
        mean_row_cosine: (sum_cos / (rows as f64)) as f32,
    };
}
