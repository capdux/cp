// Origin: Pure-CP Stdlib Test Suite (lib/attention/mrope.cp) | Created: 2026-11-23
// Production Deep Verification Suite for 3D Multimodal RoPE (M-RoPE — Qwen2-VL / Qwen2.5-VL):
//   1. Qwen2-VL default config validation & inverse frequency spectrum verification
//   2. 1D text degeneration (`pos_t == pos_h == pos_w`) exact equivalence with standard 1D RoPE
//   3. 2D image grid position builder (`appendImageGrid`) & 2D spatial translation relativity
//   4. 3D video spatiotemporal grid (`appendVideoGrid`) & subsequent text offset continuity
//   5. Strict L2 norm conservation across single-head, multi-head GQA & full sequence kernels
//   6. Interleaved vs contiguous section layout exact frequency-to-axis binding verification
//   7. Error boundaries (`InvalidHeadDim`, `InvalidSections`, `DimMismatch`, `SequenceOverflow`) & facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MROPE_MAX_HEAD_DIM,
    MROPE_MAX_SEQ_LEN,
    MropeError,
    MropeConfig,
    MropePos3d,
    MropeSegmentKind,
    MropePosBuilder,
    computeMropeInvFreqs,
    applyMropeHeadInPlaceF32,
    applyMropeMultiHeadInPlaceF32,
    applyMropeSequenceInPlaceF32,
    RopeConfig,
    ropePrecomputeTables,
    ropeApplyInPlace,
} from "cap:attention";

fn approxEq(a: f32, b: f32, tol: f32): bool {
    const d: f32 = if (a > b) a - b else b - a;
    return d <= tol;
}

fn dotF32(a: []const f32, b: []const f32): f32 {
    return @dot(a, b);
}

fn l2NormSqF32(v: []const f32): f32 {
    return @dot(v, v);
}

test("M-RoPE: Qwen2-VL config validation & inv_freqs spectrum") {
    const qwen = MropeConfig.qwen2VlDefaults();
    expect_eq(qwen.head_dim, @as(usize, 128));
    expect_eq(qwen.section_t, @as(usize, 16));
    expect_eq(qwen.section_h, @as(usize, 24));
    expect_eq(qwen.section_w, @as(usize, 24));
    expect_true(approxEq(qwen.theta, 1000000.0, 1.0e-2));
    expect_true(!qwen.interleaved);
    expect_true(qwen.isValid());

    // Invalid configs
    const odd_dim = MropeConfig{ head_dim: 13, section_t: 2, section_h: 2, section_w: 2 };
    expect_true(!odd_dim.isValid());
    const bad_sec = MropeConfig{ head_dim: 16, section_t: 3, section_h: 3, section_w: 3 };
    expect_true(!bad_sec.isValid());
    const bad_theta = MropeConfig{ head_dim: 8, section_t: 1, section_h: 1, section_w: 2, theta: 1.0 };
    expect_true(!bad_theta.isValid());

    // Precompute inv_freqs for Qwen2-VL defaults
    let inv_buf: [64]f32 = [0.0; 64];
    let inv_s: []f32 = inv_buf[0..];
    let ok: bool = true;
    computeMropeInvFreqs(qwen, *inv_s) catch {
        ok = false;
    };
    expect_true(ok);
    // inv_freqs[0] = theta^0 = 1.0, strictly decreasing across pairs
    expect_true(approxEq(inv_buf[0], 1.0, 1.0e-6));
    expect_true(inv_buf[1] < inv_buf[0]);
    expect_true(inv_buf[31] < inv_buf[1]);
    expect_true(inv_buf[63] > 0.0 && inv_buf[63] < inv_buf[31]);
}

test("M-RoPE: 1D text degeneration (pos_t==pos_h==pos_w) matches standard 1D RoPE") {
    const head_dim: usize = 12;
    const pos_val: usize = 7;
    const cfg_contig = MropeConfig{
        head_dim: head_dim,
        section_t: 2,
        section_h: 2,
        section_w: 2,
        theta: 10000.0,
        interleaved: false,
    };
    const cfg_inter = MropeConfig{
        head_dim: head_dim,
        section_t: 2,
        section_h: 2,
        section_w: 2,
        theta: 10000.0,
        interleaved: true,
    };

    let inv_buf: [6]f32 = [0.0; 6];
    let inv_s: []f32 = inv_buf[0..];
    computeMropeInvFreqs(cfg_contig, *inv_s) catch {
        expect_true(false);
    };

    // Reference standard 1D RoPE from rope.cp
    const rope_cfg = RopeConfig{ head_dim: head_dim, base_theta: 10000.0, ntk_scale: 1.0 };
    let cos_tbl: [48]f64 = [0.0; 48]; // 8 * 6
    let sin_tbl: [48]f64 = [0.0; 48];
    let cos_s: []f64 = cos_tbl[0..];
    let sin_s: []f64 = sin_tbl[0..];
    expect_true(ropePrecomputeTables(rope_cfg, 8, *cos_s, *sin_s));

    let ref_vec: [12]f64 = [0.5, -1.2, 0.3, 0.9, -0.4, 1.5, 0.8, -0.6, 1.1, -0.2, 0.7, -1.3];
    let ref_s: []f64 = ref_vec[0..];
    expect_true(ropeApplyInPlace(*ref_s, pos_val, head_dim, cos_tbl[0..], sin_tbl[0..]));

    // M-RoPE contiguous & interleaved at textPos(7)
    let v_contig: [12]f32 = [0.5, -1.2, 0.3, 0.9, -0.4, 1.5, 0.8, -0.6, 1.1, -0.2, 0.7, -1.3];
    let v_inter: [12]f32 = [0.5, -1.2, 0.3, 0.9, -0.4, 1.5, 0.8, -0.6, 1.1, -0.2, 0.7, -1.3];
    let vc_s: []f32 = v_contig[0..];
    let vi_s: []f32 = v_inter[0..];

    const p1d = MropePos3d.textPos(pos_val as i32);
    applyMropeHeadInPlaceF32(*vc_s, p1d, inv_buf[0..], cfg_contig) catch {
        expect_true(false);
    };
    applyMropeHeadInPlaceF32(*vi_s, p1d, inv_buf[0..], cfg_inter) catch {
        expect_true(false);
    };

    let i: usize = 0;
    while (i < head_dim) : (i += 1) {
        expect_true(approxEq(v_contig[i], ref_vec[i] as f32, 1.0e-5));
        expect_true(approxEq(v_inter[i], ref_vec[i] as f32, 1.0e-5));
    }
}

test("M-RoPE: 2D image grid position builder & spatial translation relativity") {
    let builder = MropePosBuilder.init();
    builder.appendTextTokens(3) catch {
        expect_true(false);
    };
    builder.appendImageGrid(2, 3) catch {
        expect_true(false);
    };
    builder.appendTextTokens(2) catch {
        expect_true(false);
    };

    // 3 text + 6 image + 2 text = 11 tokens
    expect_eq(builder.len(), @as(usize, 11));
    // Image started at base=3, max(2, 3)=3 -> post-image text starts at 6, +2 -> cursor=8
    expect_eq(builder.nextTextPos(), @as(i32, 8));

    // Check text token 2: (2, 2, 2)
    if (p2 := builder.getPos(2)) {
        expect_eq(p2.pos_t, @as(i32, 2));
        expect_eq(p2.pos_h, @as(i32, 2));
        expect_eq(p2.pos_w, @as(i32, 2));
    } else {
        expect_true(false);
    }

    // Check image patch (h=1, w=2) at index 3 + 1*3 + 2 = 8: (3, 4, 5)
    if (p_img := builder.getPos(8)) {
        expect_eq(p_img.pos_t, @as(i32, 3));
        expect_eq(p_img.pos_h, @as(i32, 4));
        expect_eq(p_img.pos_w, @as(i32, 5));
    } else {
        expect_true(false);
    }
    if (k_img := builder.getKind(8)) {
        expect_true(k_img == MropeSegmentKind.image);
    } else {
        expect_true(false);
    }

    // Check first post-image text token at index 9: (6, 6, 6)
    if (p9 := builder.getPos(9)) {
        expect_eq(p9.pos_t, @as(i32, 6));
        expect_eq(p9.pos_h, @as(i32, 6));
        expect_eq(p9.pos_w, @as(i32, 6));
    } else {
        expect_true(false);
    }

    // Verify 2D spatial translation relativity:
    // <M-RoPE(q, (t, h+1, w+2)), M-RoPE(k, (t, h, w))> is invariant to shifting (h, w) by (+5, +7)
    const cfg = MropeConfig{ head_dim: 12, section_t: 2, section_h: 2, section_w: 2, theta: 10000.0 };
    let inv_buf: [6]f32 = [0.0; 6];
    let inv_s: []f32 = inv_buf[0..];
    computeMropeInvFreqs(cfg, *inv_s) catch {
        expect_true(false);
    };

    let q1: [12]f32 = [0.3, -0.7, 1.1, 0.2, -0.5, 0.9, 0.4, 0.8, -0.6, 1.2, -0.1, 0.5];
    let k1: [12]f32 = [-0.4, 0.6, 0.8, -0.9, 1.0, -0.2, 0.7, -0.3, 0.5, 0.1, -0.8, 0.4];
    let q2: [12]f32 = q1;
    let k2: [12]f32 = k1;
    let q1_s: []f32 = q1[0..];
    let k1_s: []f32 = k1[0..];
    let q2_s: []f32 = q2[0..];
    let k2_s: []f32 = k2[0..];

    applyMropeHeadInPlaceF32(*q1_s, MropePos3d.visionPos(3, 2, 4), inv_buf[0..], cfg) catch { expect_true(false); };
    applyMropeHeadInPlaceF32(*k1_s, MropePos3d.visionPos(3, 1, 2), inv_buf[0..], cfg) catch { expect_true(false); };
    applyMropeHeadInPlaceF32(*q2_s, MropePos3d.visionPos(3, 7, 11), inv_buf[0..], cfg) catch { expect_true(false); };
    applyMropeHeadInPlaceF32(*k2_s, MropePos3d.visionPos(3, 6, 9), inv_buf[0..], cfg) catch { expect_true(false); };

    const dot1: f32 = dotF32(q1[0..], k1[0..]);
    const dot2: f32 = dotF32(q2[0..], k2[0..]);
    expect_true(approxEq(dot1, dot2, 1.0e-4));
}

test("M-RoPE: 3D video spatiotemporal grid & post-video text offset continuity") {
    let builder = MropePosBuilder.init();
    builder.appendTextTokens(2) catch { expect_true(false); };
    // Video grid: T=2 frames, H=2, W=3 -> 12 tokens, base=2, span=max(2,2,3)=3 -> next=5
    builder.appendVideoGrid(2, 2, 3) catch { expect_true(false); };
    builder.appendTextTokens(2) catch { expect_true(false); };

    expect_eq(builder.len(), @as(usize, 16));
    expect_eq(builder.nextTextPos(), @as(i32, 7));

    // Frame t=1, h=1, w=2 is at index 2 + 1*(2*3) + 1*3 + 2 = 13 -> (2+1, 2+1, 2+2) = (3, 3, 4)
    if (pv := builder.getPos(13)) {
        expect_eq(pv.pos_t, @as(i32, 3));
        expect_eq(pv.pos_h, @as(i32, 3));
        expect_eq(pv.pos_w, @as(i32, 4));
    } else {
        expect_true(false);
    }
    if (kv := builder.getKind(13)) {
        expect_true(kv == MropeSegmentKind.video);
    } else {
        expect_true(false);
    }

    // First post-video text token at index 14 -> (5, 5, 5)
    if (pt := builder.getPos(14)) {
        expect_eq(pt.pos_t, @as(i32, 5));
        expect_eq(pt.pos_h, @as(i32, 5));
        expect_eq(pt.pos_w, @as(i32, 5));
    } else {
        expect_true(false);
    }
    expect_true(builder.getPos(16) == null);
}

test("M-RoPE: strict L2 norm conservation across single-head, multi-head GQA & sequence") {
    const cfg = MropeConfig{
        head_dim: 8,
        section_t: 1,
        section_h: 1,
        section_w: 2,
        theta: 1000000.0,
        interleaved: false,
    };
    let inv_buf: [4]f32 = [0.0; 4];
    let inv_s: []f32 = inv_buf[0..];
    computeMropeInvFreqs(cfg, *inv_s) catch { expect_true(false); };

    // Build a 3-token multimodal sequence: [text(0), vision(1, 2, 3), text(4)]
    const positions: [3]MropePos3d = [
        MropePos3d.textPos(0),
        MropePos3d.visionPos(1, 2, 3),
        MropePos3d.textPos(4),
    ];

    // [seq_len=3, num_heads=2, head_dim=8] = 48 floats
    let seq_buf: [48]f32 = [0.0; 48];
    let i: usize = 0;
    while (i < 48) : (i += 1) {
        const sign: f32 = if ((i % 2) == 0) 1.0 else -1.0;
        seq_buf[i] = sign * (0.25 + ((i % 7) as f32) * 0.35);
    }
    let ref_seq: [48]f32 = seq_buf;

    // Record L2 norms before rotation for all 6 heads
    let norms_before: [6]f32 = [0.0; 6];
    let h_idx: usize = 0;
    while (h_idx < 6) : (h_idx += 1) {
        const off: usize = h_idx * 8;
        norms_before[h_idx] = l2NormSqF32(seq_buf[off .. off + 8]);
    }

    // Apply full sequence kernel
    let seq_s: []f32 = seq_buf[0..];
    applyMropeSequenceInPlaceF32(*seq_s, 3, 2, positions[0..], inv_buf[0..], cfg) catch {
        expect_true(false);
    };

    // Compare against per-token multi-head kernel (GQA-compatible)
    let t: usize = 0;
    while (t < 3) : (t += 1) {
        const tok_off: usize = t * 16;
        let tok_s: []f32 = ref_seq[tok_off .. tok_off + 16];
        applyMropeMultiHeadInPlaceF32(*tok_s, 2, positions[t], inv_buf[0..], cfg) catch {
            expect_true(false);
        };
    }

    // Verify exact parity and strict L2 norm conservation
    let k: usize = 0;
    while (k < 48) : (k += 1) {
        expect_true(approxEq(seq_buf[k], ref_seq[k], 1.0e-6));
    }
    let h_chk: usize = 0;
    while (h_chk < 6) : (h_chk += 1) {
        const off: usize = h_chk * 8;
        const norm_after: f32 = l2NormSqF32(seq_buf[off .. off + 8]);
        expect_true(approxEq(norms_before[h_chk], norm_after, 1.0e-4));
    }
}

test("M-RoPE: interleaved layout vs contiguous layout frequency-axis binding") {
    // head_dim = 12 (6 pairs), sections (1, 2, 3):
    //   Contiguous order:  [T, H, H, W, W, W]
    //   Interleaved order: [T, H, W, H, W, W]
    const cfg_contig = MropeConfig{
        head_dim: 12,
        section_t: 1,
        section_h: 2,
        section_w: 3,
        theta: 10000.0,
        interleaved: false,
    };
    const cfg_inter = MropeConfig{
        head_dim: 12,
        section_t: 1,
        section_h: 2,
        section_w: 3,
        theta: 10000.0,
        interleaved: true,
    };

    let inv_buf: [6]f32 = [0.0; 6];
    let inv_s: []f32 = inv_buf[0..];
    computeMropeInvFreqs(cfg_inter, *inv_s) catch { expect_true(false); };

    // Use unit-first-half vector [1,1,1,1,1,1, 0,0,0,0,0,0] so after rotation:
    //   vec[i] = cos(coord_i * inv_freqs[i]), vec[i+6] = sin(coord_i * inv_freqs[i])
    let v_c: [12]f32 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let v_i: [12]f32 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let vc_s: []f32 = v_c[0..];
    let vi_s: []f32 = v_i[0..];

    const pos = MropePos3d.visionPos(2, 5, 9);
    applyMropeHeadInPlaceF32(*vc_s, pos, inv_buf[0..], cfg_contig) catch { expect_true(false); };
    applyMropeHeadInPlaceF32(*vi_s, pos, inv_buf[0..], cfg_inter) catch { expect_true(false); };

    // Pair 0 is T in both, Pair 1 is H in both -> identical
    expect_true(approxEq(v_c[0], v_i[0], 1.0e-6));
    expect_true(approxEq(v_c[1], v_i[1], 1.0e-6));

    // Pair 2: Contiguous uses H (pos_h=5), Interleaved uses W (pos_w=9) -> distinct
    const exp_c2: f32 = @cos(5.0 * (inv_buf[2] as f64)) as f32;
    const exp_i2: f32 = @cos(9.0 * (inv_buf[2] as f64)) as f32;
    expect_true(approxEq(v_c[2], exp_c2, 1.0e-5));
    expect_true(approxEq(v_i[2], exp_i2, 1.0e-5));
    expect_true(!approxEq(v_c[2], v_i[2], 1.0e-4));

    // Pair 3: Contiguous uses W (pos_w=9), Interleaved uses H (pos_h=5)
    const exp_c3: f32 = @cos(9.0 * (inv_buf[3] as f64)) as f32;
    const exp_i3: f32 = @cos(5.0 * (inv_buf[3] as f64)) as f32;
    expect_true(approxEq(v_c[3], exp_c3, 1.0e-5));
    expect_true(approxEq(v_i[3], exp_i3, 1.0e-5));
}

test("M-RoPE: error boundaries (InvalidHeadDim, InvalidSections, DimMismatch, SequenceOverflow) & facade parity") {
    // 1. InvalidHeadDim (odd & exceeding MROPE_MAX_HEAD_DIM)
    let inv_buf: [8]f32 = [0.0; 8];
    let inv_s: []f32 = inv_buf[0..];
    const bad_dim_cfg = MropeConfig{ head_dim: MROPE_MAX_HEAD_DIM + 2, section_t: 40, section_h: 40, section_w: 49 };
    let got_invalid_dim: bool = false;
    computeMropeInvFreqs(bad_dim_cfg, *inv_s) catch |e| {
        got_invalid_dim = (e == MropeError.InvalidHeadDim);
    };
    expect_true(got_invalid_dim);

    // 2. InvalidSections
    const bad_sec_cfg = MropeConfig{ head_dim: 8, section_t: 2, section_h: 2, section_w: 2 };
    let got_invalid_sec: bool = false;
    computeMropeInvFreqs(bad_sec_cfg, *inv_s) catch |e| {
        got_invalid_sec = (e == MropeError.InvalidSections);
    };
    expect_true(got_invalid_sec);

    // 3. DimMismatch on head_vec
    const ok_cfg = MropeConfig{ head_dim: 8, section_t: 1, section_h: 1, section_w: 2 };
    computeMropeInvFreqs(ok_cfg, *inv_s) catch { expect_true(false); };
    let wrong_head: [6]f32 = [0.0; 6];
    let wh_s: []f32 = wrong_head[0..];
    let got_dim_mismatch: bool = false;
    applyMropeHeadInPlaceF32(*wh_s, MropePos3d.textPos(1), inv_buf[0..], ok_cfg) catch |e| {
        got_dim_mismatch = (e == MropeError.DimMismatch);
    };
    expect_true(got_dim_mismatch);

    // 4. SequenceOverflow in MropePosBuilder
    let builder = MropePosBuilder.init();
    builder.appendTextTokens(MROPE_MAX_SEQ_LEN) catch { expect_true(false); };
    expect_eq(builder.len(), MROPE_MAX_SEQ_LEN);
    let got_overflow: bool = false;
    builder.appendTextTokens(1) catch |e| {
        got_overflow = (e == MropeError.SequenceOverflow);
    };
    expect_true(got_overflow);

    // 5. `cap.attention` facade parity
    const f_cfg = cap.attention.MropeConfig.qwen2VlDefaults();
    expect_true(f_cfg.isValid());
    expect_eq(f_cfg.head_dim, @as(usize, 128));
}
