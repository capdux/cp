// Origin: Pure-CP Stdlib (AI Inference GGML-Style Q8_0 & Q4_0 Block Quantization and GEMV) | Created: 2026-11-22
// Zero-heap-allocation GGML-style blockwise quantization (Q8_0 & Q4_0, 32 elements per block)
// and integer-accumulated GEMV inference kernels (`W_q8 * x_q8` and `W_q4 * x_q8`).

const std = @import("std");

/// Number of scalar elements per GGML quantization block.
pub const QUANT_BLOCK_SIZE: usize = 32;

/// Q8_0 quantization block: 1 block scale + 32 symmetric int8 values (`[-127..+127]`).
pub struct BlockQ8_0 {
    scale: f64 = 0.0;
    qs: [32]i8 = [0; 32];
}

/// Q4_0 quantization block: 1 block scale + 16 bytes packing 32 4-bit offset integers (`[-8..+7]`).
/// Layout matches GGML: low nibble `qs[j] & 0x0F` encodes element `j`, high nibble `qs[j] >> 4` encodes `j + 16`.
pub struct BlockQ4_0 {
    scale: f64 = 0.0;
    qs: [16]u8 = [0x88; 16];
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Quantizes `src` into Q8_0 blocks (32 elements/block).
/// Requires `src.len > 0`, `src.len % 32 == 0`, finite values, and `blocks_out.len >= src.len / 32`.
pub fn quantizeRowQ8_0(src: []const f64, blocks_out: *[]BlockQ8_0): bool {
    if (src.len == 0 || (src.len % QUANT_BLOCK_SIZE) != 0) return false;
    const n_blocks = src.len / QUANT_BLOCK_SIZE;
    if (blocks_out.len < n_blocks) return false;

    let ci: usize = 0;
    while (ci < src.len) : (ci += 1) {
        if (!isFiniteF64(src[ci])) return false;
    }

    let b: usize = 0;
    while (b < n_blocks) : (b += 1) {
        const base = b * QUANT_BLOCK_SIZE;
        let amax: f64 = 0.0;
        let j: usize = 0;
        while (j < QUANT_BLOCK_SIZE) : (j += 1) {
            const av = @abs(src[base + j]);
            if (av > amax) amax = av;
        }

        let blk = BlockQ8_0{};
        if (amax > 0.0) {
            const scale = amax / 127.0;
            const inv_scale = 127.0 / amax;
            blk.scale = scale;
            let k: usize = 0;
            while (k < QUANT_BLOCK_SIZE) : (k += 1) {
                let scaled = src[base + k] * inv_scale;
                if (scaled > 127.0) scaled = 127.0;
                if (scaled < -127.0) scaled = -127.0;
                const rounded: f64 = @round(scaled);
                blk.qs[k] = rounded as i8;
            }
        }
        blocks_out[b] = blk;
    }
    return true;
}

/// Dequantizes Q8_0 blocks back into `f64` values written to `dst`.
/// Returns `false` if `dst.len < blocks.len * 32` or any block scale is non-finite.
pub fn dequantizeRowQ8_0(blocks: []const BlockQ8_0, dst: *[]f64): bool {
    if (blocks.len == 0) return true;
    if (blocks.len > std.math.maxInt(usize) / QUANT_BLOCK_SIZE) return false;
    if (dst.len < blocks.len * QUANT_BLOCK_SIZE) return false;

    let b: usize = 0;
    while (b < blocks.len) : (b += 1) {
        const sc = blocks[b].scale;
        if (!isFiniteF64(sc)) return false;
        const base = b * QUANT_BLOCK_SIZE;
        let j: usize = 0;
        while (j < QUANT_BLOCK_SIZE) : (j += 1) {
            dst[base + j] = (blocks[b].qs[j] as f64) * sc;
        }
    }
    return true;
}

/// Integer-accumulated dot product over two Q8_0 block vectors.
/// Computes pure `i32` inner product per 32-element block, then scales by `a[b].scale * b[b].scale`.
/// Returns `null` on length mismatch or non-finite scales.
pub fn dotQ8_0(a: []const BlockQ8_0, b: []const BlockQ8_0): f64? {
    if (a.len != b.len) return null;
    if (a.len == 0) return 0.0;

    let total: f64 = 0.0;
    let bi: usize = 0;
    while (bi < a.len) : (bi += 1) {
        const sa = a[bi].scale;
        const sb = b[bi].scale;
        if (!isFiniteF64(sa) || !isFiniteF64(sb)) return null;

        let sumi: i32 = 0;
        let j: usize = 0;
        while (j < QUANT_BLOCK_SIZE) : (j += 1) {
            sumi += (a[bi].qs[j] as i32) * (b[bi].qs[j] as i32);
        }
        total += (sumi as f64) * sa * sb;
    }
    return total;
}

/// Q8_0 matrix-vector multiplication `y = W * x` (`rows` rows, `blocks_per_row` Q8_0 blocks per row).
pub fn gemvQ8_0(
    weight_blocks: []const BlockQ8_0,
    x_blocks: []const BlockQ8_0,
    rows: usize,
    blocks_per_row: usize,
    y_out: *[]f64,
): bool {
    if (rows == 0) return true;
    if (blocks_per_row == 0) return false;
    if (y_out.len < rows || x_blocks.len < blocks_per_row) return false;
    if (rows > std.math.maxInt(usize) / blocks_per_row) return false;
    if (weight_blocks.len < rows * blocks_per_row) return false;

    const x_row = x_blocks[0..blocks_per_row];
    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const w_row = weight_blocks[r * blocks_per_row .. (r + 1) * blocks_per_row];
        const d_opt = dotQ8_0(w_row, x_row);
        if (d_opt == null) return false;
        y_out[r] = d_opt ?? 0.0;
    }
    return true;
}

/// Quantizes `src` into Q4_0 blocks (32 elements/block packed into 16 bytes, ~8x compression vs f32).
/// Requires `src.len > 0`, `src.len % 32 == 0`, finite values, and `blocks_out.len >= src.len / 32`.
pub fn quantizeRowQ4_0(src: []const f64, blocks_out: *[]BlockQ4_0): bool {
    if (src.len == 0 || (src.len % QUANT_BLOCK_SIZE) != 0) return false;
    const n_blocks = src.len / QUANT_BLOCK_SIZE;
    if (blocks_out.len < n_blocks) return false;

    let ci: usize = 0;
    while (ci < src.len) : (ci += 1) {
        if (!isFiniteF64(src[ci])) return false;
    }

    let b: usize = 0;
    while (b < n_blocks) : (b += 1) {
        const base = b * QUANT_BLOCK_SIZE;
        let amax: f64 = 0.0;
        let j: usize = 0;
        while (j < QUANT_BLOCK_SIZE) : (j += 1) {
            const av = @abs(src[base + j]);
            if (av > amax) amax = av;
        }

        let blk = BlockQ4_0{};
        if (amax > 0.0) {
            const scale = amax / 7.0;
            const inv_scale = 7.0 / amax;
            blk.scale = scale;

            let k: usize = 0;
            while (k < 16) : (k += 1) {
                let s0 = src[base + k] * inv_scale;
                if (s0 > 7.0) s0 = 7.0;
                if (s0 < -8.0) s0 = -8.0;
                const r0: f64 = @round(s0);
                const q0: u8 = ((r0 as i32) + 8) as u8;

                let s1 = src[base + k + 16] * inv_scale;
                if (s1 > 7.0) s1 = 7.0;
                if (s1 < -8.0) s1 = -8.0;
                const r1: f64 = @round(s1);
                const q1: u8 = ((r1 as i32) + 8) as u8;

                blk.qs[k] = (q0 & 0x0F) | ((q1 & 0x0F) << 4);
            }
        }
        blocks_out[b] = blk;
    }
    return true;
}

/// Dequantizes Q4_0 blocks back into `f64` values written to `dst`.
/// Returns `false` if `dst.len < blocks.len * 32` or any block scale is non-finite.
pub fn dequantizeRowQ4_0(blocks: []const BlockQ4_0, dst: *[]f64): bool {
    if (blocks.len == 0) return true;
    if (blocks.len > std.math.maxInt(usize) / QUANT_BLOCK_SIZE) return false;
    if (dst.len < blocks.len * QUANT_BLOCK_SIZE) return false;

    let b: usize = 0;
    while (b < blocks.len) : (b += 1) {
        const sc = blocks[b].scale;
        if (!isFiniteF64(sc)) return false;
        const base = b * QUANT_BLOCK_SIZE;
        let j: usize = 0;
        while (j < 16) : (j += 1) {
            const packed_byte = blocks[b].qs[j];
            const v0: i32 = ((packed_byte & 0x0F) as i32) - 8;
            const v1: i32 = ((packed_byte >> 4) as i32) - 8;
            dst[base + j] = (v0 as f64) * sc;
            dst[base + j + 16] = (v1 as f64) * sc;
        }
    }
    return true;
}

/// Mixed-precision GGML `W_q4 * x_q8` matrix-vector multiplication (`y_out[r] = dot(W_q4[r], x_q8)`).
/// Unpacks 4-bit weight nibbles and accumulates integer `i32` dot products per 32-element block.
pub fn gemvQ4_0_Q8_0(
    w_q4: []const BlockQ4_0,
    x_q8: []const BlockQ8_0,
    rows: usize,
    blocks_per_row: usize,
    y_out: *[]f64,
): bool {
    if (rows == 0) return true;
    if (blocks_per_row == 0) return false;
    if (y_out.len < rows || x_q8.len < blocks_per_row) return false;
    if (rows > std.math.maxInt(usize) / blocks_per_row) return false;
    if (w_q4.len < rows * blocks_per_row) return false;

    // Pre-check x_q8 scales
    let xi: usize = 0;
    while (xi < blocks_per_row) : (xi += 1) {
        if (!isFiniteF64(x_q8[xi].scale)) return false;
    }

    let r: usize = 0;
    while (r < rows) : (r += 1) {
        let acc: f64 = 0.0;
        const row_base = r * blocks_per_row;
        let bi: usize = 0;
        while (bi < blocks_per_row) : (bi += 1) {
            const wb = w_q4[row_base + bi];
            const xb = x_q8[bi];
            if (!isFiniteF64(wb.scale)) return false;

            let sumi: i32 = 0;
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                const packed_byte = wb.qs[j];
                const w0: i32 = ((packed_byte & 0x0F) as i32) - 8;
                const w1: i32 = ((packed_byte >> 4) as i32) - 8;
                sumi += w0 * (xb.qs[j] as i32) + w1 * (xb.qs[j + 16] as i32);
            }
            acc += (sumi as f64) * wb.scale * xb.scale;
        }
        y_out[r] = acc;
    }
    return true;
}
