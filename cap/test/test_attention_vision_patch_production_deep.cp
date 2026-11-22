// Origin: Pure-CP Stdlib Test Suite (lib/attention/vision_patch.cp) | Created: 2026-11-22
// Production Deep Verification Suite for multimodal ViT/CLIP/Qwen2-VL vision frontend kernels:
// RGB u8->f32 FMA normalization, exact HWC patchify mapping, SIMD @dot patch linear embedding,
// 2D sinusoidal positional embedding uniqueness/norm, 2D-RoPE norm conservation & coordinate
// differentiation, and defensive dimension guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ImageNormParams,
    normalizeRgbU8ToF32,
    patchifyHwcF32,
    patchEmbedLinearF32,
    build2dSinCosPosEmbF32,
    applyRope2dInPlaceF32,
} from "cap:attention";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn testRgbU8NormalizationAccuracy()! {
    const rgb: [6]u8 = [0, 128, 255, 255, 0, 128];
    let out: [6]f32 = undefined;
    let out_s: []f32 = out[0..];

    const clip_p = cap.attention.ImageNormParams.clipDefault();
    expect_eq(cap.attention.normalizeRgbU8ToF32(rgb[0..], clip_p, out_s), true);

    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const ch: usize = i % 3;
        const expected: f64 = ((rgb[i] as f64) / 255.0 - (clip_p.mean[ch] as f64)) / (clip_p.std[ch] as f64);
        expect_eq(absF64((out[i] as f64) - expected) < 0.0001, true);
    }

    const imgnet_p = ImageNormParams.imagenetDefault();
    expect_eq(normalizeRgbU8ToF32(rgb[0..], imgnet_p, out_s), true);
    const exp0: f64 = (0.0 - 0.485) / 0.229;
    expect_eq(absF64((out[0] as f64) - exp0) < 0.0001, true);
}

fn testHwcPatchifyExactPixelMapping()! {
    // 4x4x2 image where pixel(y, x, c) = y*100 + x*10 + c.
    let img: [32]f32 = undefined;
    let y: usize = 0;
    while (y < 4) : (y += 1) {
        let x: usize = 0;
        while (x < 4) : (x += 1) {
            let c: usize = 0;
            while (c < 2) : (c += 1) {
                img[(y * 4 + x) * 2 + c] = ((y * 100 + x * 10 + c) as f32);
            }
        }
    }

    let patches: [32]f32 = undefined;
    let patches_s: []f32 = patches[0..];

    const np_opt = cap.attention.patchifyHwcF32(img[0..], 4, 4, 2, 2, patches_s);
    expect_eq(np_opt != null, true);
    if (np := np_opt) {
        expect_eq(np, 4);
    }

    // Patch 0 (py=0, px=0) covers (0,0), (0,1), (1,0), (1,1).
    expect_eq(patches[0], 0.0);    // (0,0,0)
    expect_eq(patches[1], 1.0);    // (0,0,1)
    expect_eq(patches[2], 10.0);   // (0,1,0)
    expect_eq(patches[4], 100.0);  // (1,0,0)
    expect_eq(patches[6], 110.0);  // (1,1,0)

    // Patch 3 (py=1, px=1) starts at offset 3*8 = 24 and covers (2,2), (2,3), (3,2), (3,3).
    expect_eq(patches[24], 220.0); // (2,2,0)
    expect_eq(patches[25], 221.0); // (2,2,1)
    expect_eq(patches[26], 230.0); // (2,3,0)
    expect_eq(patches[28], 320.0); // (3,2,0)
    expect_eq(patches[30], 330.0); // (3,3,0)
    expect_eq(patches[31], 331.0); // (3,3,1)
}

fn testPatchEmbedLinearSimdDotProjection()! {
    const num_patches: usize = 3;
    const patch_dim: usize = 12;
    const embed_dim: usize = 4;

    let patches: [36]f32 = undefined; // 3 * 12
    let weight: [48]f32 = undefined;  // 4 * 12
    const bias: [4]f32 = [0.5, -0.5, 1.0, -1.0];
    let out_emb: [12]f32 = undefined; // 3 * 4

    let idx: usize = 0;
    while (idx < 36) : (idx += 1) {
        patches[idx] = (((idx % 7) as f32) - 3.0) * 0.25;
    }
    idx = 0;
    while (idx < 48) : (idx += 1) {
        weight[idx] = (((idx % 5) as f32) - 2.0) * 0.3;
    }

    let out_s: []f32 = out_emb[0..];
    expect_eq(patchEmbedLinearF32(patches[0..], weight[0..], bias[0..], num_patches, patch_dim, embed_dim, out_s), true);

    let p: usize = 0;
    while (p < num_patches) : (p += 1) {
        let e: usize = 0;
        while (e < embed_dim) : (e += 1) {
            let acc: f64 = bias[e] as f64;
            let k: usize = 0;
            while (k < patch_dim) : (k += 1) {
                acc += (patches[p * patch_dim + k] as f64) * (weight[e * patch_dim + k] as f64);
            }
            expect_eq(absF64((out_emb[p * embed_dim + e] as f64) - acc) < 0.001, true);
        }
    }

    // Verify no-bias mode (empty slice).
    const no_bias: []const f32 = bias[0..0];
    expect_eq(patchEmbedLinearF32(patches[0..], weight[0..], no_bias, num_patches, patch_dim, embed_dim, out_s), true);
    let acc0: f64 = 0.0;
    let k: usize = 0;
    while (k < patch_dim) : (k += 1) {
        acc0 += (patches[k] as f64) * (weight[k] as f64);
    }
    expect_eq(absF64((out_emb[0] as f64) - acc0) < 0.001, true);
}

fn test2dSinCosPosEmbUniquenessAndBounds()! {
    const grid_h: usize = 3;
    const grid_w: usize = 4;
    const embed_dim: usize = 8;
    const num_patches: usize = 12;

    let pos: [96]f32 = undefined; // 12 * 8
    let pos_s: []f32 = pos[0..];

    expect_eq(cap.attention.build2dSinCosPosEmbF32(grid_h, grid_w, embed_dim, pos_s), true);

    // 1. Every entry in [-1, 1] and every patch embedding has exact squared norm == embed_dim / 2.
    let p: usize = 0;
    while (p < num_patches) : (p += 1) {
        let norm_sq: f64 = 0.0;
        let d: usize = 0;
        while (d < embed_dim) : (d += 1) {
            const v: f32 = pos[p * embed_dim + d];
            expect_eq(v >= -1.0001 && v <= 1.0001, true);
            norm_sq += (v as f64) * (v as f64);
        }
        expect_eq(absF64(norm_sq - 4.0) < 0.001, true);
    }

    // 2. All 12 grid positions produce pairwise distinct embeddings.
    let a: usize = 0;
    while (a < num_patches) : (a += 1) {
        let b: usize = a + 1;
        while (b < num_patches) : (b += 1) {
            const d2: f32 = @l2sq(pos[a * embed_dim .. (a + 1) * embed_dim], pos[b * embed_dim .. (b + 1) * embed_dim]);
            expect_eq(d2 > 0.01, true);
        }
    }
}

fn testRope2dNormPreservationAndCoordinateDistinction()! {
    const orig: [8]f32 = [1.2, -0.8, 0.5, 1.5, -0.7, 0.9, 1.1, -0.4];
    const orig_norm_sq: f32 = @dot(orig[0..], orig[0..]);

    // 1. (row=0, col=0) must be an exact identity transform.
    let v_zero: [8]f32 = orig;
    let v_zero_s: []f32 = v_zero[0..];
    expect_eq(applyRope2dInPlaceF32(v_zero_s, 0, 0, 10000.0), true);
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(absF64((v_zero[i] as f64) - (orig[i] as f64)) < 0.00001, true);
    }

    // 2. (row=3, col=5) vs (row=5, col=3): both preserve exact L2 norm and produce distinct vectors.
    let v_35: [8]f32 = orig;
    let v_53: [8]f32 = orig;
    let v_35_s: []f32 = v_35[0..];
    let v_53_s: []f32 = v_53[0..];

    expect_eq(cap.attention.applyRope2dInPlaceF32(v_35_s, 3, 5, 10000.0), true);
    expect_eq(applyRope2dInPlaceF32(v_53_s, 5, 3, 10000.0), true);

    const n35: f32 = @dot(v_35[0..], v_35[0..]);
    const n53: f32 = @dot(v_53[0..], v_53[0..]);
    expect_eq(absF64((n35 as f64) - (orig_norm_sq as f64)) < 0.0001, true);
    expect_eq(absF64((n53 as f64) - (orig_norm_sq as f64)) < 0.0001, true);

    const diff_35_53: f32 = @l2sq(v_35[0..], v_53[0..]);
    expect_eq(diff_35_53 > 0.1, true);
}

fn testInvalidDimensionsAndDefensiveGuards()! {
    const rgb_bad: [4]u8 = [1, 2, 3, 4];
    const rgb_ok: [3]u8 = [10, 20, 30];
    let out_f: [8]f32 = [0.0; 8];
    let out_s: []f32 = out_f[0..];

    // normalizeRgbU8ToF32 guards: len % 3 != 0, std <= 0, short out.
    expect_eq(normalizeRgbU8ToF32(rgb_bad[0..], ImageNormParams.clipDefault(), out_s), false);
    const bad_std = ImageNormParams{ mean: [0.5, 0.5, 0.5], std: [0.2, 0.0, 0.2] };
    expect_eq(normalizeRgbU8ToF32(rgb_ok[0..], bad_std, out_s), false);
    expect_eq(normalizeRgbU8ToF32(rgb_ok[0..], ImageNormParams.clipDefault(), out_f[0..2]), false);

    // patchifyHwcF32 guards: non-divisible height/width, zero patch_size.
    expect_eq(patchifyHwcF32(out_f[0..], 3, 2, 1, 2, out_s) == null, true);
    expect_eq(patchifyHwcF32(out_f[0..], 2, 2, 1, 0, out_s) == null, true);

    // build2dSinCosPosEmbF32 guards: embed_dim not multiple of 4, zero grid.
    expect_eq(build2dSinCosPosEmbF32(2, 2, 6, out_s), false);
    expect_eq(build2dSinCosPosEmbF32(0, 2, 4, out_s), false);

    // applyRope2dInPlaceF32 guards: vec.len % 4 != 0, non-positive/NaN theta_base.
    expect_eq(applyRope2dInPlaceF32(out_f[0..6], 1, 1, 10000.0), false);
    expect_eq(applyRope2dInPlaceF32(out_s, 1, 1, 0.0), false);
    expect_eq(applyRope2dInPlaceF32(out_s, 1, 1, std.math.nan(f32)), false);
}

fn main() {
    test("1. attention/vision_patch: CLIP & ImageNet RGB u8->f32 FMA normalization", () => {
        testRgbU8NormalizationAccuracy()?;
    });

    test("2. attention/vision_patch: HWC patchify exact pixel coordinate mapping", () => {
        testHwcPatchifyExactPixelMapping()?;
    });

    test("3. attention/vision_patch: SIMD @dot patch linear embedding projection", () => {
        testPatchEmbedLinearSimdDotProjection()?;
    });

    test("4. attention/vision_patch: 2D sinusoidal positional embedding uniqueness & norm", () => {
        test2dSinCosPosEmbUniquenessAndBounds()?;
    });

    test("5. attention/vision_patch: 2D-RoPE norm conservation & (row, col) distinction", () => {
        testRope2dNormPreservationAndCoordinateDistinction()?;
    });

    test("6. attention/vision_patch: invalid dimensions, std<=0 & theta/NaN guards", () => {
        testInvalidDimensionsAndDefensiveGuards()?;
    });
}
