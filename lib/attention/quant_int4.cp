// Origin: Pure-CP Stdlib (AI Inference Per-Group INT4 Sym/Asym Quantization & GEMV) | Created: 2026-11-22
// Pure CP zero-allocation per-group INT4 weight quantization (Symmetric [-8, 7] & Asymmetric [0, 15]
// with zero_point, GPTQ/AWQ style) and fused scale-hoisted mixed-precision GEMV (`y = W_int4 * x_f32`).

const std = @import("std");

/// Quantization domain mode for per-group INT4 weights.
pub enum Int4QuantMode {
    /// Symmetric signed 4-bit integers `[-8, 7]` stored as offset-8 nibbles `[0, 15]`.
    Symmetric,
    /// Asymmetric unsigned 4-bit integers `[0, 15]` with per-group `zero_point` in `[0, 15]`.
    Asymmetric,
}

/// Per-group affine/symmetric quantization parameters.
pub struct Int4GroupParams {
    scale: f32 = 0.0,
    zero_point: u8 = 0,
    group_size: u16 = 0,
}

// ============================================================================
// 1. Nibble Bit-Packing Primitives
// ============================================================================

/// Packs two 4-bit unsigned nibbles (`lo` in bits `[3:0]`, `hi` in bits `[7:4]`) into one byte.
pub fn packInt4Pair(lo: u8, hi: u8): u8 {
    return (lo & 0x0F) | ((hi & 0x0F) << 4);
}

/// Extracts the low 4-bit nibble (even element index) from a packed byte.
pub fn unpackInt4Lo(byte: u8): u8 {
    return byte & 0x0F;
}

/// Extracts the high 4-bit nibble (odd element index) from a packed byte.
pub fn unpackInt4Hi(byte: u8): u8 {
    return (byte >> 4) & 0x0F;
}

// ============================================================================
// Internal Validation & Rounding Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn validateGroupSize(group_size: u16): bool {
    return group_size >= 2 && (group_size % 2) == 0;
}

fn quantizeScalarSym(val: f32, inv_scale: f32): u8 {
    let scaled: f32 = val * inv_scale;
    if (scaled > 7.0) {
        scaled = 7.0;
    }
    if (scaled < -8.0) {
        scaled = -8.0;
    }
    const rounded_f: f32 = @round(scaled);
    let clamped: i32 = rounded_f as i32;
    if (clamped > 7) {
        clamped = 7;
    }
    if (clamped < -8) {
        clamped = -8;
    }
    return (clamped + 8) as u8;
}

fn quantizeScalarAsym(val: f32, inv_scale: f32, zp: u8): u8 {
    let scaled: f32 = @round(val * inv_scale) + (zp as f32);
    if (scaled > 15.0) {
        scaled = 15.0;
    }
    if (scaled < 0.0) {
        scaled = 0.0;
    }
    return scaled as u8;
}

// ============================================================================
// 2. Per-Group Symmetric & Asymmetric INT4 Quantization
// ============================================================================

/// Quantizes `src` into per-group symmetric INT4 (`[-8, 7]` offset by +8 into `[0, 15]`).
/// Requires `group_size >= 2`, even `group_size`, `src.len > 0`, `src.len % group_size == 0`,
/// finite inputs, `out_packed.len >= src.len / 2`, and `out_params.len >= src.len / group_size`.
pub fn quantizeGroupInt4Sym(
    src: []const f32,
    group_size: u16,
    out_packed: *[]u8,
    out_params: *[]Int4GroupParams,
): bool {
    if (!validateGroupSize(group_size) || src.len == 0) {
        return false;
    }
    const gs: usize = group_size as usize;
    if ((src.len % gs) != 0) {
        return false;
    }
    const n_groups: usize = src.len / gs;
    const n_packed: usize = src.len / 2;
    if (out_packed.len < n_packed || out_params.len < n_groups) {
        return false;
    }

    // Atomic pre-check for NaN/Inf.
    let ci: usize = 0;
    while (ci < src.len) : (ci += 1) {
        if (!isFiniteF32(src[ci])) {
            return false;
        }
    }

    const pairs_per_group: usize = gs / 2;
    let g: usize = 0;
    while (g < n_groups) : (g += 1) {
        const base: usize = g * gs;
        let amax: f32 = 0.0;
        let j: usize = 0;
        while (j < gs) : (j += 1) {
            const av: f32 = @abs(src[base + j]);
            if (av > amax) {
                amax = av;
            }
        }

        const byte_base: usize = g * pairs_per_group;
        if (amax == 0.0) {
            out_params[g] = Int4GroupParams{
                scale: 0.0,
                zero_point: 8,
                group_size: group_size,
            };
            let p: usize = 0;
            while (p < pairs_per_group) : (p += 1) {
                out_packed[byte_base + p] = packInt4Pair(8, 8);
            }
        } else {
            const scale: f32 = amax / 7.0;
            const inv_scale: f32 = 7.0 / amax;
            out_params[g] = Int4GroupParams{
                scale: scale,
                zero_point: 8,
                group_size: group_size,
            };
            let p: usize = 0;
            while (p < pairs_per_group) : (p += 1) {
                const lo: u8 = quantizeScalarSym(src[base + p * 2], inv_scale);
                const hi: u8 = quantizeScalarSym(src[base + p * 2 + 1], inv_scale);
                out_packed[byte_base + p] = packInt4Pair(lo, hi);
            }
        }
    }
    return true;
}

/// Quantizes `src` into per-group asymmetric INT4 (`[0, 15]` with affine `zero_point` in `[0, 15]`).
/// Anchors `[min, max]` to include `0.0` so `zero_point` stays exact within `[0, 15]` without clipping.
pub fn quantizeGroupInt4Asym(
    src: []const f32,
    group_size: u16,
    out_packed: *[]u8,
    out_params: *[]Int4GroupParams,
): bool {
    if (!validateGroupSize(group_size) || src.len == 0) {
        return false;
    }
    const gs: usize = group_size as usize;
    if ((src.len % gs) != 0) {
        return false;
    }
    const n_groups: usize = src.len / gs;
    const n_packed: usize = src.len / 2;
    if (out_packed.len < n_packed || out_params.len < n_groups) {
        return false;
    }

    let ci: usize = 0;
    while (ci < src.len) : (ci += 1) {
        if (!isFiniteF32(src[ci])) {
            return false;
        }
    }

    const pairs_per_group: usize = gs / 2;
    let g: usize = 0;
    while (g < n_groups) : (g += 1) {
        const base: usize = g * gs;
        let r_min: f32 = 0.0;
        let r_max: f32 = 0.0;
        let j: usize = 0;
        while (j < gs) : (j += 1) {
            const v: f32 = src[base + j];
            if (v < r_min) {
                r_min = v;
            }
            if (v > r_max) {
                r_max = v;
            }
        }

        const range: f32 = r_max - r_min;
        const byte_base: usize = g * pairs_per_group;
        if (range == 0.0) {
            out_params[g] = Int4GroupParams{
                scale: 0.0,
                zero_point: 0,
                group_size: group_size,
            };
            let p: usize = 0;
            while (p < pairs_per_group) : (p += 1) {
                out_packed[byte_base + p] = 0;
            }
        } else {
            const scale: f32 = range / 15.0;
            const inv_scale: f32 = 15.0 / range;
            let zp_f: f32 = @round(-r_min * inv_scale);
            if (zp_f < 0.0) {
                zp_f = 0.0;
            }
            if (zp_f > 15.0) {
                zp_f = 15.0;
            }
            const zp: u8 = zp_f as u8;
            out_params[g] = Int4GroupParams{
                scale: scale,
                zero_point: zp,
                group_size: group_size,
            };
            let p: usize = 0;
            while (p < pairs_per_group) : (p += 1) {
                const lo: u8 = quantizeScalarAsym(src[base + p * 2], inv_scale, zp);
                const hi: u8 = quantizeScalarAsym(src[base + p * 2 + 1], inv_scale, zp);
                out_packed[byte_base + p] = packInt4Pair(lo, hi);
            }
        }
    }
    return true;
}

// ============================================================================
// 3. Per-Group Dequantization
// ============================================================================

/// Dequantizes packed per-group INT4 values back into `f32` elements written to `out_dst`.
pub fn dequantizeGroupInt4(
    packed_data: []const u8,
    params: []const Int4GroupParams,
    mode: Int4QuantMode,
    out_dst: *[]f32,
): bool {
    if (params.len == 0) {
        return false;
    }

    // Atomic validation of all group parameters and output buffer capacity.
    let total_elems: usize = 0;
    let g: usize = 0;
    while (g < params.len) : (g += 1) {
        const p = params[g];
        if (!validateGroupSize(p.group_size)) {
            return false;
        }
        if (!isFiniteF32(p.scale) || p.scale < 0.0) {
            return false;
        }
        if (mode == .Asymmetric && p.zero_point > 15) {
            return false;
        }
        const gs: usize = p.group_size as usize;
        if (total_elems > std.math.maxInt(usize) - gs) {
            return false;
        }
        total_elems += gs;
    }

    if (packed_data.len < (total_elems / 2) || out_dst.len < total_elems) {
        return false;
    }

    let elem_offset: usize = 0;
    let byte_offset: usize = 0;
    g = 0;
    while (g < params.len) : (g += 1) {
        const p = params[g];
        const gs: usize = p.group_size as usize;
        const pairs: usize = gs / 2;
        const zp: i32 = if (mode == .Symmetric) 8 else (p.zero_point as i32);

        if (p.scale == 0.0) {
            let j: usize = 0;
            while (j < gs) : (j += 1) {
                out_dst[elem_offset + j] = 0.0;
            }
        } else {
            let k: usize = 0;
            while (k < pairs) : (k += 1) {
                const b: u8 = packed_data[byte_offset + k];
                const q0: i32 = (unpackInt4Lo(b) as i32) - zp;
                const q1: i32 = (unpackInt4Hi(b) as i32) - zp;
                out_dst[elem_offset + k * 2] = (q0 as f32) * p.scale;
                out_dst[elem_offset + k * 2 + 1] = (q1 as f32) * p.scale;
            }
        }

        elem_offset += gs;
        byte_offset += pairs;
    }
    return true;
}

// ============================================================================
// 4. Fused Zero-Temp Per-Group INT4 GEMV (`y = W_int4 * x_f32`)
// ============================================================================

/// Computes `out_y = W * x` where `W` (`rows x cols`, row-major) is quantized in per-group INT4.
/// Performs scale-hoisted fused multiply-accumulate directly over packed nibbles without
/// materializing a dequantized `f32` weight matrix.
pub fn gemvInt4Group(
    x: []const f32,
    packed_weights: []const u8,
    group_params: []const Int4GroupParams,
    rows: usize,
    cols: usize,
    group_size: u16,
    mode: Int4QuantMode,
    out_y: *[]f32,
): bool {
    if (rows == 0 || cols == 0 || !validateGroupSize(group_size)) {
        return false;
    }
    const gs: usize = group_size as usize;
    if ((cols % gs) != 0) {
        return false;
    }
    if (x.len < cols || out_y.len < rows) {
        return false;
    }

    const groups_per_row: usize = cols / gs;
    const bytes_per_row: usize = cols / 2;
    if (rows > std.math.maxInt(usize) / groups_per_row) {
        return false;
    }
    if (rows > std.math.maxInt(usize) / bytes_per_row) {
        return false;
    }
    const total_groups: usize = rows * groups_per_row;
    const total_bytes: usize = rows * bytes_per_row;
    if (group_params.len < total_groups || packed_weights.len < total_bytes) {
        return false;
    }

    // Atomic validation of input vector and all group parameters.
    let xi: usize = 0;
    while (xi < cols) : (xi += 1) {
        if (!isFiniteF32(x[xi])) {
            return false;
        }
    }
    let gi: usize = 0;
    while (gi < total_groups) : (gi += 1) {
        const p = group_params[gi];
        if (p.group_size != group_size || !isFiniteF32(p.scale) || p.scale < 0.0) {
            return false;
        }
        if (mode == .Asymmetric && p.zero_point > 15) {
            return false;
        }
    }

    const pairs_per_group: usize = gs / 2;
    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const row_group_base: usize = r * groups_per_row;
        const row_byte_base: usize = r * bytes_per_row;
        let row_acc: f64 = 0.0;

        let g: usize = 0;
        while (g < groups_per_row) : (g += 1) {
            const p = group_params[row_group_base + g];
            if (p.scale == 0.0) {
                continue;
            }
            const zp: i32 = if (mode == .Symmetric) 8 else (p.zero_point as i32);
            const g_byte_base: usize = row_byte_base + g * pairs_per_group;
            const g_col_base: usize = g * gs;

            let group_dot: f64 = 0.0;
            let k: usize = 0;
            while (k < pairs_per_group) : (k += 1) {
                const b: u8 = packed_weights[g_byte_base + k];
                const q0: i32 = (unpackInt4Lo(b) as i32) - zp;
                const q1: i32 = (unpackInt4Hi(b) as i32) - zp;
                const x0: f64 = x[g_col_base + k * 2] as f64;
                const x1: f64 = x[g_col_base + k * 2 + 1] as f64;
                group_dot += (q0 as f64) * x0 + (q1 as f64) * x1;
            }
            // Scale hoisting: one scale multiply per group.
            row_acc += group_dot * (p.scale as f64);
        }

        out_y[r] = row_acc as f32;
    }
    return true;
}
