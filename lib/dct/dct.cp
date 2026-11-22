// Origin: Pure-CP Stdlib (Zero-Alloc 1D/2D 8x8 Orthonormal DCT-II / IDCT-III, ZigZag & JPEG Quantization DSP Library) | Created: 2026-11-22

import std from "std";

pub const DCT_BLOCK_DIM: usize = 8;
pub const DCT_BLOCK_SIZE: usize = 64;

/// Standard JPEG 8x8 ZigZag scan order (maps zigzag index -> raster index).
pub const JPEG_ZIGZAG_8X8: [64]u8 = [
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
];

/// ITU-T T.81 Table K.1 standard luminance quantization matrix (quality = 50, raster order).
pub const JPEG_LUMA_QUANT_50: [64]u8 = [
    16, 11, 10, 16,  24,  40,  51,  61,
    12, 12, 14, 19,  26,  58,  60,  55,
    14, 13, 16, 24,  40,  57,  69,  56,
    14, 17, 22, 29,  51,  87,  80,  62,
    18, 22, 37, 56,  68, 109, 103,  77,
    24, 35, 55, 64,  81, 104, 113,  92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103,  99,
];

/// ITU-T T.81 Table K.2 standard chrominance quantization matrix (quality = 50, raster order).
pub const JPEG_CHROMA_QUANT_50: [64]u8 = [
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
];

const INV_SQRT_8: f64 = 0.35355339059327376220; // sqrt(1/8)
const SQRT_2_OVER_8: f64 = 0.5;                 // sqrt(2/8) = 0.5

/// Computes the 1D orthonormal DCT-II of arbitrary length `N >= 1`.
/// Normalization: `alpha(0) = sqrt(1/N)`, `alpha(k) = sqrt(2/N)` for `k >= 1`
/// (preserves Parseval energy `sum(x^2) == sum(X^2)` and guarantees `idct1d(dct1d(x)) == x`).
/// Returns `false` if `input.len == 0` or `output.len < input.len`.
pub fn dct1d(input: []const f64, output: *[]f64): bool {
    const n: usize = input.len;
    if (n == 0 || output.*.len < n) {
        return false;
    }

    const nf: f64 = n as f64;
    const scale0: f64 = std.math.sqrt(1.0 / nf);
    const scalek: f64 = std.math.sqrt(2.0 / nf);
    const factor: f64 = std.math.pi / (2.0 * nf);

    let k: usize = 0;
    while (k < n) {
        const kf: f64 = k as f64;
        let sum: f64 = 0.0;
        let idx: usize = 0;
        while (idx < n) {
            const angle: f64 = factor * ((2 * idx + 1) as f64) * kf;
            sum += input[idx] * std.math.cos(angle);
            idx += 1;
        }
        if (k == 0) {
            output.*[k] = sum * scale0;
        } else {
            output.*[k] = sum * scalek;
        }
        k += 1;
    }
    return true;
}

/// Computes the 1D orthonormal IDCT-III (exact inverse of `dct1d`) of arbitrary length `N >= 1`.
/// Returns `false` if `input.len == 0` or `output.len < input.len`.
pub fn idct1d(input: []const f64, output: *[]f64): bool {
    const n: usize = input.len;
    if (n == 0 || output.*.len < n) {
        return false;
    }

    const nf: f64 = n as f64;
    const scale0: f64 = std.math.sqrt(1.0 / nf);
    const scalek: f64 = std.math.sqrt(2.0 / nf);
    const factor: f64 = std.math.pi / (2.0 * nf);

    let idx: usize = 0;
    while (idx < n) {
        const two_n_plus_1: f64 = (2 * idx + 1) as f64;
        let sum: f64 = input[0] * scale0;
        let k: usize = 1;
        while (k < n) {
            const angle: f64 = factor * two_n_plus_1 * (k as f64);
            sum += input[k] * scalek * std.math.cos(angle);
            k += 1;
        }
        output.*[idx] = sum;
        idx += 1;
    }
    return true;
}

fn dct8Line(src: [8]f64): [8]f64 {
    let dst: [8]f64 = [0.0; 8];
    const factor: f64 = std.math.pi / 16.0;
    let k: usize = 0;
    while (k < 8) {
        const kf: f64 = k as f64;
        let sum: f64 = 0.0;
        let n: usize = 0;
        while (n < 8) {
            sum += src[n] * std.math.cos(factor * ((2 * n + 1) as f64) * kf);
            n += 1;
        }
        if (k == 0) {
            dst[k] = sum * INV_SQRT_8;
        } else {
            dst[k] = sum * SQRT_2_OVER_8;
        }
        k += 1;
    }
    return dst;
}

fn idct8Line(src: [8]f64): [8]f64 {
    let dst: [8]f64 = [0.0; 8];
    const factor: f64 = std.math.pi / 16.0;
    let n: usize = 0;
    while (n < 8) {
        const two_n_plus_1: f64 = (2 * n + 1) as f64;
        let sum: f64 = src[0] * INV_SQRT_8;
        let k: usize = 1;
        while (k < 8) {
            sum += src[k] * SQRT_2_OVER_8 * std.math.cos(factor * two_n_plus_1 * (k as f64));
            k += 1;
        }
        dst[n] = sum;
        n += 1;
    }
    return dst;
}

/// Computes the separable 2D 8x8 orthonormal DCT-II (row transform followed by column transform).
/// A constant block of value `c` yields `DC = 8 * c` at index 0 and `0` for all 63 AC coefficients.
pub fn dct8x8(input: [64]f64): [64]f64 {
    let tmp: [64]f64 = [0.0; 64];
    let out: [64]f64 = [0.0; 64];

    // Pass 1: 1D DCT-II across each of the 8 rows.
    let r: usize = 0;
    while (r < 8) {
        let row: [8]f64 = [0.0; 8];
        let c: usize = 0;
        while (c < 8) {
            row[c] = input[r * 8 + c];
            c += 1;
        }
        const trow: [8]f64 = dct8Line(row);
        c = 0;
        while (c < 8) {
            tmp[r * 8 + c] = trow[c];
            c += 1;
        }
        r += 1;
    }

    // Pass 2: 1D DCT-II down each of the 8 columns.
    let col: usize = 0;
    while (col < 8) {
        let vec: [8]f64 = [0.0; 8];
        let row_i: usize = 0;
        while (row_i < 8) {
            vec[row_i] = tmp[row_i * 8 + col];
            row_i += 1;
        }
        const tcol: [8]f64 = dct8Line(vec);
        row_i = 0;
        while (row_i < 8) {
            out[row_i * 8 + col] = tcol[row_i];
            row_i += 1;
        }
        col += 1;
    }

    return out;
}

/// Computes the separable 2D 8x8 orthonormal IDCT-III (exact inverse of `dct8x8`).
pub fn idct8x8(coeffs: [64]f64): [64]f64 {
    let tmp: [64]f64 = [0.0; 64];
    let out: [64]f64 = [0.0; 64];

    // Pass 1: 1D IDCT-III down each of the 8 columns.
    let col: usize = 0;
    while (col < 8) {
        let vec: [8]f64 = [0.0; 8];
        let row_i: usize = 0;
        while (row_i < 8) {
            vec[row_i] = coeffs[row_i * 8 + col];
            row_i += 1;
        }
        const tcol: [8]f64 = idct8Line(vec);
        row_i = 0;
        while (row_i < 8) {
            tmp[row_i * 8 + col] = tcol[row_i];
            row_i += 1;
        }
        col += 1;
    }

    // Pass 2: 1D IDCT-III across each of the 8 rows.
    let r: usize = 0;
    while (r < 8) {
        let row: [8]f64 = [0.0; 8];
        let c: usize = 0;
        while (c < 8) {
            row[c] = tmp[r * 8 + c];
            c += 1;
        }
        const trow: [8]f64 = idct8Line(row);
        c = 0;
        while (c < 8) {
            out[r * 8 + c] = trow[c];
            c += 1;
        }
        r += 1;
    }

    return out;
}

/// Reorders an 8x8 raster block into JPEG ZigZag scan order.
pub fn zigzagReorder8x8(block: [64]i32): [64]i32 {
    let out: [64]i32 = [0; 64];
    let i: usize = 0;
    while (i < 64) {
        out[i] = block[JPEG_ZIGZAG_8X8[i] as usize];
        i += 1;
    }
    return out;
}

/// Inverse of `zigzagReorder8x8`: restores a ZigZag-ordered array back to 8x8 raster order.
pub fn unzigzagReorder8x8(zz: [64]i32): [64]i32 {
    let out: [64]i32 = [0; 64];
    let i: usize = 0;
    while (i < 64) {
        out[JPEG_ZIGZAG_8X8[i] as usize] = zz[i];
        i += 1;
    }
    return out;
}

/// Quantizes 64 DCT coefficients using `qtable` (round-to-nearest; `q == 0` treated as `1`).
pub fn quantize8x8(coeffs: [64]f64, qtable: [64]u8): [64]i32 {
    let out: [64]i32 = [0; 64];
    let i: usize = 0;
    while (i < 64) {
        let q: f64 = qtable[i] as f64;
        if (qtable[i] == 0) {
            q = 1.0;
        }
        out[i] = std.math.round(coeffs[i] / q) as i32;
        i += 1;
    }
    return out;
}

/// Dequantizes 64 integer coefficients back to `f64` using `qtable` (`q == 0` treated as `1`).
pub fn dequantize8x8(quant: [64]i32, qtable: [64]u8): [64]f64 {
    let out: [64]f64 = [0.0; 64];
    let i: usize = 0;
    while (i < 64) {
        let q: f64 = qtable[i] as f64;
        if (qtable[i] == 0) {
            q = 1.0;
        }
        out[i] = (quant[i] as f64) * q;
        i += 1;
    }
    return out;
}

/// Scales a base 8x8 quantization table according to the IJG libjpeg quality factor (`1..100`).
/// `quality == 50` returns `base` verbatim; values are clamped to `[1, 255]`.
pub fn scaleQuantTable(base: [64]u8, quality: u8): [64]u8 {
    let q: u32 = quality as u32;
    if (q < 1) {
        q = 1;
    }
    if (q > 100) {
        q = 100;
    }

    let scale: u32 = 0;
    if (q < 50) {
        scale = (5000 as u32) / q;
    } else {
        scale = (200 as u32) - (2 as u32) * q;
    }

    let out: [64]u8 = [1; 64];
    let i: usize = 0;
    while (i < 64) {
        let v: u32 = ((base[i] as u32) * scale + 50) / 100;
        if (v < 1) {
            v = 1;
        }
        if (v > 255) {
            v = 255;
        }
        out[i] = v as u8;
        i += 1;
    }
    return out;
}
