// Origin: Pure-CP Stdlib (Multimodal ViT Patchify, 2D Sinusoidal PosEmb & 2D-RoPE using @dot/@fma) | Created: 2026-11-22
// Pure CP zero-allocation multimodal vision frontend kernels for ViT / CLIP / Qwen2-VL / Flux:
// RGB u8->f32 normalization, HWC patchify, SIMD @dot linear patch projection,
// MAE/ViT 2D sinusoidal positional embeddings, and norm-preserving 2D spatial RoPE.

const std = @import("std");

/// Per-channel RGB normalization parameters `(pixel / 255.0 - mean) / std`.
pub struct ImageNormParams {
    mean: [3]f32 = [0.485, 0.456, 0.406],
    std: [3]f32 = [0.229, 0.224, 0.225],

    /// OpenAI CLIP default RGB normalization statistics.
    pub static fn clipDefault(): ImageNormParams {
        return ImageNormParams{
            mean: [0.48145466, 0.4578275, 0.40821073],
            std: [0.26862954, 0.26130258, 0.27577711],
        };
    }

    /// Standard ImageNet-1K RGB normalization statistics.
    pub static fn imagenetDefault(): ImageNormParams {
        return ImageNormParams{
            mean: [0.485, 0.456, 0.406],
            std: [0.229, 0.224, 0.225],
        };
    }
}

// ============================================================================
// Internal Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
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
// 1. Hardware-FMA RGB u8 -> Normalized f32 Conversion
// ============================================================================

/// Normalizes interleaved RGB `u8` pixels (`len % 3 == 0`) into `f32` via
/// `(px / 255.0 - mean[c]) / std[c]` fused into a single `@fma` per channel.
pub fn normalizeRgbU8ToF32(
    rgb_u8: []const u8,
    params: ImageNormParams,
    out_f32: *[]f32,
): bool {
    if (rgb_u8.len == 0 || (rgb_u8.len % 3) != 0 || out_f32.len < rgb_u8.len) {
        return false;
    }
    let c: usize = 0;
    while (c < 3) : (c += 1) {
        if (!isFiniteF32(params.mean[c]) || !isFiniteF32(params.std[c]) || params.std[c] <= 0.0) {
            return false;
        }
    }

    const scale0: f32 = 1.0 / (255.0 * params.std[0]);
    const scale1: f32 = 1.0 / (255.0 * params.std[1]);
    const scale2: f32 = 1.0 / (255.0 * params.std[2]);
    const bias0: f32 = 0.0 - (params.mean[0] / params.std[0]);
    const bias1: f32 = 0.0 - (params.mean[1] / params.std[1]);
    const bias2: f32 = 0.0 - (params.mean[2] / params.std[2]);

    let i: usize = 0;
    while (i < rgb_u8.len) : (i += 3) {
        out_f32[i + 0] = @fma(rgb_u8[i + 0] as f32, scale0, bias0);
        out_f32[i + 1] = @fma(rgb_u8[i + 1] as f32, scale1, bias1);
        out_f32[i + 2] = @fma(rgb_u8[i + 2] as f32, scale2, bias2);
    }
    return true;
}

// ============================================================================
// 2. Zero-Allocation HWC Image Patchify
// ============================================================================

/// Slices an HWC `f32` image (`[height, width, channels]`) into a flat patch matrix
/// `[num_patches, patch_size * patch_size * channels]` and returns `num_patches`.
/// Requires `height % patch_size == 0` and `width % patch_size == 0`.
pub fn patchifyHwcF32(
    img_hwc: []const f32,
    height: usize,
    width: usize,
    channels: usize,
    patch_size: usize,
    out_patches: *[]f32,
): usize? {
    if (height == 0 || width == 0 || channels == 0 || patch_size == 0) {
        return null;
    }
    if ((height % patch_size) != 0 || (width % patch_size) != 0) {
        return null;
    }

    const grid_h: usize = height / patch_size;
    const grid_w: usize = width / patch_size;
    const num_patches: usize = checkedMul(grid_h, grid_w) ?? return null;
    const ps2: usize = checkedMul(patch_size, patch_size) ?? return null;
    const patch_dim: usize = checkedMul(ps2, channels) ?? return null;
    const total_elems: usize = checkedMul(num_patches, patch_dim) ?? return null;
    if (img_hwc.len < total_elems || out_patches.len < total_elems) {
        return null;
    }

    const row_stride: usize = width * channels;
    const patch_row_stride: usize = patch_size * channels;

    let py: usize = 0;
    while (py < grid_h) : (py += 1) {
        let px: usize = 0;
        while (px < grid_w) : (px += 1) {
            const patch_idx: usize = py * grid_w + px;
            const dst_patch_base: usize = patch_idx * patch_dim;

            let dy: usize = 0;
            while (dy < patch_size) : (dy += 1) {
                const img_y: usize = py * patch_size + dy;
                const src_base: usize = img_y * row_stride + (px * patch_size) * channels;
                const dst_base: usize = dst_patch_base + dy * patch_row_stride;

                let k: usize = 0;
                while (k < patch_row_stride) : (k += 1) {
                    out_patches[dst_base + k] = img_hwc[src_base + k];
                }
            }
        }
    }

    return num_patches;
}

// ============================================================================
// 3. SIMD @dot Linear Patch Embedding Projection
// ============================================================================

/// Projects flattened patches `[num_patches, patch_dim]` into `[num_patches, embed_dim]`
/// using weight matrix `[embed_dim, patch_dim]` via 8-lane SIMD `@dot` plus optional `bias`.
pub fn patchEmbedLinearF32(
    patches: []const f32,
    weight: []const f32,
    bias: []const f32,
    num_patches: usize,
    patch_dim: usize,
    embed_dim: usize,
    out_emb: *[]f32,
): bool {
    if (num_patches == 0 || patch_dim == 0 || embed_dim == 0) {
        return false;
    }
    if (bias.len != 0 && bias.len < embed_dim) {
        return false;
    }
    const p_elems: usize = checkedMul(num_patches, patch_dim) ?? return false;
    const w_elems: usize = checkedMul(embed_dim, patch_dim) ?? return false;
    const o_elems: usize = checkedMul(num_patches, embed_dim) ?? return false;
    if (patches.len < p_elems || weight.len < w_elems || out_emb.len < o_elems) {
        return false;
    }
    const has_bias: bool = bias.len >= embed_dim;

    let p: usize = 0;
    while (p < num_patches) : (p += 1) {
        const p_slice = patches[p * patch_dim .. (p + 1) * patch_dim];
        const out_base: usize = p * embed_dim;
        let e: usize = 0;
        while (e < embed_dim) : (e += 1) {
            const w_slice = weight[e * patch_dim .. (e + 1) * patch_dim];
            const dot_val: f32 = @dot(p_slice, w_slice);
            if (has_bias) {
                out_emb[out_base + e] = dot_val + bias[e];
            } else {
                out_emb[out_base + e] = dot_val;
            }
        }
    }
    return true;
}

// ============================================================================
// 4. MAE / ViT 2D Sinusoidal Positional Embeddings
// ============================================================================

/// Builds 2D sinusoidal positional embeddings `[grid_h * grid_w, embed_dim]` in `out_pos`.
/// Requires `embed_dim >= 4` and `embed_dim % 4 == 0`:
/// - first half `[0 .. embed_dim / 2)` encodes row coordinate `y`
/// - second half `[embed_dim / 2 .. embed_dim)` encodes column coordinate `x`.
pub fn build2dSinCosPosEmbF32(
    grid_h: usize,
    grid_w: usize,
    embed_dim: usize,
    out_pos: *[]f32,
): bool {
    if (grid_h == 0 || grid_w == 0 || embed_dim < 4 || (embed_dim % 4) != 0) {
        return false;
    }
    const num_patches: usize = checkedMul(grid_h, grid_w) ?? return false;
    const total_elems: usize = checkedMul(num_patches, embed_dim) ?? return false;
    if (out_pos.len < total_elems) {
        return false;
    }

    const half: usize = embed_dim / 2;
    const quarter: usize = embed_dim / 4;
    const q_f64: f64 = quarter as f64;

    let y: usize = 0;
    while (y < grid_h) : (y += 1) {
        const yf: f64 = y as f64;
        let x: usize = 0;
        while (x < grid_w) : (x += 1) {
            const xf: f64 = x as f64;
            const base: usize = (y * grid_w + x) * embed_dim;

            let k: usize = 0;
            while (k < quarter) : (k += 1) {
                const exp_term: f64 = (k as f64) / q_f64;
                const omega: f64 = 1.0 / std.math.pow(f64, 10000.0, exp_term);
                const ang_y: f64 = yf * omega;
                const ang_x: f64 = xf * omega;

                out_pos[base + 2 * k] = @sin(ang_y) as f32;
                out_pos[base + 2 * k + 1] = @cos(ang_y) as f32;
                out_pos[base + half + 2 * k] = @sin(ang_x) as f32;
                out_pos[base + half + 2 * k + 1] = @cos(ang_x) as f32;
            }
        }
    }
    return true;
}

// ============================================================================
// 5. Qwen2-VL / Flux Norm-Preserving 2D Spatial RoPE
// ============================================================================

/// Applies 2D spatial Rotary Positional Embedding (2D-RoPE) in-place to `vec`.
/// Requires `vec.len >= 4`, `vec.len % 4 == 0`, and finite `theta_base > 0.0`.
/// Rotates `[0 .. len/2)` by `row` and `[len/2 .. len)` by `col`, preserving exact L2 norm.
pub fn applyRope2dInPlaceF32(
    vec: *[]f32,
    row: usize,
    col: usize,
    theta_base: f32,
): bool {
    if (vec.len < 4 || (vec.len % 4) != 0 || !isFiniteF32(theta_base) || theta_base <= 0.0) {
        return false;
    }

    const half: usize = vec.len / 2;
    const quarter: usize = vec.len / 4;
    const half_f64: f64 = half as f64;
    const base_f64: f64 = theta_base as f64;
    const row_f64: f64 = row as f64;
    const col_f64: f64 = col as f64;

    let k: usize = 0;
    while (k < quarter) : (k += 1) {
        const exp_term: f64 = ((2 * k) as f64) / half_f64;
        const inv_freq: f64 = 1.0 / std.math.pow(f64, base_f64, exp_term);

        // Rotate row subspace [0 .. half)
        const ang_y: f64 = row_f64 * inv_freq;
        const cy: f64 = @cos(ang_y);
        const sy: f64 = @sin(ang_y);
        const y0: f64 = vec[2 * k] as f64;
        const y1: f64 = vec[2 * k + 1] as f64;
        vec[2 * k] = (y0 * cy - y1 * sy) as f32;
        vec[2 * k + 1] = (y0 * sy + y1 * cy) as f32;

        // Rotate col subspace [half .. len)
        const ang_x: f64 = col_f64 * inv_freq;
        const cx: f64 = @cos(ang_x);
        const sx: f64 = @sin(ang_x);
        const x0: f64 = vec[half + 2 * k] as f64;
        const x1: f64 = vec[half + 2 * k + 1] as f64;
        vec[half + 2 * k] = (x0 * cx - x1 * sx) as f32;
        vec[half + 2 * k + 1] = (x0 * sx + x1 * cx) as f32;
    }
    return true;
}
