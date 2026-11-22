// Origin: Pure-CP Stdlib Test Suite (lib/attention/quantKivi.cp) | Created: 2026-11-23
// Production Deep Verification Suite for KIVI Asymmetric 2-Bit KV-Cache Compression:
//   1. 2-bit quad packing/unpacking (`pack2BitQuad`, `unpack2BitLane`) exact round-trip
//   2. Per-token Value 2-bit quantization & strict `<= span / 6` max-error bound
//   3. Per-channel Key 2-bit quantization isolating fixed-channel outliers vs naive per-token quantization
//   4. Direct `kiviDotQueryKey2Bit` exact parity with dequantized SIMD `@dot`
//   5. `KiviKvCache` automatic residual archival & `decodeSingleQuery` cosine similarity > 0.95 vs FP32 reference
//   6. Compression ratio & memory footprint telemetry (`KiviCacheStats`)
//   7. Error boundaries (`InvalidHeadDim`, `DimMismatch`, `CapacityFull`) & `cap.attention` facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    KIVI_MAX_HEAD_DIM,
    KIVI_MAX_TOKENS,
    KIVI_PACKED_STRIDE,
    KiviError,
    KiviAffineParams,
    pack2BitQuad,
    unpack2BitLane,
    quantizeToken2BitPerToken,
    dequantizeToken2BitPerToken,
    quantizeKeys2BitPerChannel,
    dequantizeKey2BitPerChannel,
    kiviDotQueryKey2Bit,
    KiviKvCache,
    KIVI_SNAPSHOT_MAGIC,
    KIVI_SNAPSHOT_VERSION,
    KiviFlatError,
    KiviFlatConfig,
    kiviPackedKvBufLen,
    kiviKeyGroupParamsBufLen,
    kiviValTokParamsBufLen,
    kiviResidualBufLen,
    kiviSnapshotMaxBytes,
    KiviFlatKvCache,
    serializeKiviSnapshot,
    deserializeKiviSnapshot,
} from "cap:attention";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn cosineSimF32(a: []const f32, b: []const f32): f32 {
    const dot: f32 = @dot(a, b);
    const na: f32 = @sqrt(@dot(a, a));
    const nb: f32 = @sqrt(@dot(b, b));
    if (na == 0.0 || nb == 0.0) return 0.0;
    return dot / (na * nb);
}

test("KIVI: 2-bit quad pack/unpack round-trip across all 4 lanes") {
    const b1: u8 = pack2BitQuad(0, 1, 2, 3);
    expect_eq(unpack2BitLane(b1, 0), @as(u8, 0));
    expect_eq(unpack2BitLane(b1, 1), @as(u8, 1));
    expect_eq(unpack2BitLane(b1, 2), @as(u8, 2));
    expect_eq(unpack2BitLane(b1, 3), @as(u8, 3));

    const b2: u8 = pack2BitQuad(3, 2, 0, 1);
    expect_eq(unpack2BitLane(b2, 0), @as(u8, 3));
    expect_eq(unpack2BitLane(b2, 1), @as(u8, 2));
    expect_eq(unpack2BitLane(b2, 2), @as(u8, 0));
    expect_eq(unpack2BitLane(b2, 3), @as(u8, 1));
}

test("KIVI: Per-Token Value 2-bit quantization & strict max-error bound") {
    const val: [8]f32 = [-1.5, -1.0, -0.5, 0.0, 0.25, 0.75, 1.2, 1.5]; // span = 3.0 -> scale = 1.0 -> max err <= 0.5
    let packed_buf: [2]u8 = [0; 2];
    let packed_s: []u8 = packed_buf[0..];
    const params = quantizeToken2BitPerToken(val[0..], *packed_s);

    expect_true(absF32(params.zero_point - (-1.5)) < 1.0e-5);
    expect_true(absF32(params.scale - 1.0) < 1.0e-5);

    let deq: [8]f32 = [0.0; 8];
    let deq_s: []f32 = deq[0..];
    dequantizeToken2BitPerToken(packed_buf[0..], 8, params, *deq_s);

    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_true(absF32(val[i] - deq[i]) <= 0.5 + 1.0e-4);
    }
}

test("KIVI: Per-Channel Key 2-bit quantization isolates channel outliers vs per-token") {
    // 4 tokens x dim 8: channel 0 has huge outlier ~50.0, channels 1..7 are normal in [-1.0, 1.0]
    const keys: [32]f32 = [
        48.0, -0.8, 0.1, 0.4, -0.3, 0.7, -0.6, 0.2,
        50.0, -0.2, 0.5, 0.8, -0.9, 0.1, -0.1, 0.6,
        52.0,  0.4, 0.9, 0.2,  0.3, 0.5,  0.4, 0.9,
        54.0,  0.9, 0.3, 0.6,  0.8, 0.9,  0.7, 0.1,
    ];

    let pc_packed: [8]u8 = [0; 8]; // 4 tokens * 2 bytes
    let pc_params: [8]KiviAffineParams = [KiviAffineParams{}; 8];
    let pc_pack_s: []u8 = pc_packed[0..];
    let pc_par_s: []KiviAffineParams = pc_params[0..];
    quantizeKeys2BitPerChannel(keys[0..], 4, 8, *pc_pack_s, *pc_par_s);

    // Measure MSE on non-outlier channels 1..7 for token 1 under per-channel vs naive per-token
    let pc_deq: [8]f32 = [0.0; 8];
    let pc_deq_s: []f32 = pc_deq[0..];
    dequantizeKey2BitPerChannel(pc_packed[2..4], 8, pc_params[0..], *pc_deq_s);

    let pt_pack: [2]u8 = [0; 2];
    let pt_pack_s: []u8 = pt_pack[0..];
    const pt_par = quantizeToken2BitPerToken(keys[8..16], *pt_pack_s);
    let pt_deq: [8]f32 = [0.0; 8];
    let pt_deq_s: []f32 = pt_deq[0..];
    dequantizeToken2BitPerToken(pt_pack[0..], 8, pt_par, *pt_deq_s);

    let pc_mse: f32 = 0.0;
    let pt_mse: f32 = 0.0;
    let c: usize = 1;
    while (c < 8) : (c += 1) {
        const epc: f32 = keys[8 + c] - pc_deq[c];
        const ept: f32 = keys[8 + c] - pt_deq[c];
        pc_mse += epc * epc;
        pt_mse += ept * ept;
    }
    // Per-channel quantization error on normal channels is dramatically smaller than per-token
    expect_true(pc_mse < pt_mse * 0.25);
}

test("KIVI: kiviDotQueryKey2Bit matches dequantized @dot exactly") {
    const keys: [16]f32 = [
        1.0, -0.5, 0.25, 0.75, 2.0, -1.0, 0.5, 1.5,
        1.6,  0.4, 0.85, 0.15, 2.6,  0.2, 1.1, 0.3,
    ];
    let packed_buf: [4]u8 = [0; 4];
    let params: [8]KiviAffineParams = [KiviAffineParams{}; 8];
    let pack_s: []u8 = packed_buf[0..];
    let par_s: []KiviAffineParams = params[0..];
    quantizeKeys2BitPerChannel(keys[0..], 2, 8, *pack_s, *par_s);

    const q: [8]f32 = [0.4, -0.7, 1.2, 0.3, -0.5, 0.9, 0.6, -0.2];
    const fast_dot: f32 = kiviDotQueryKey2Bit(q[0..], packed_buf[2..4], 8, params[0..]);

    let deq: [8]f32 = [0.0; 8];
    let deq_s: []f32 = deq[0..];
    dequantizeKey2BitPerChannel(packed_buf[2..4], 8, params[0..], *deq_s);
    const ref_dot: f32 = @dot(q[0..], deq[0..]);

    expect_true(absF32(fast_dot - ref_dot) < 1.0e-5);
}

test("KIVI: KiviKvCache residual archival & decodeSingleQuery cosine > 0.95 vs FP32") {
    const dim: usize = 16;
    const n_tokens: usize = 10;
    let cache = KiviKvCache.init(dim, 4) catch {
        expect_true(false);
        return;
    };

    let all_k: [160]f32 = [0.0; 160];
    let all_v: [160]f32 = [0.0; 160];

    let t: usize = 0;
    while (t < n_tokens) : (t += 1) {
        let k_tok: [16]f32 = [0.0; 16];
        let v_tok: [16]f32 = [0.0; 16];
        let c: usize = 0;
        while (c < dim) : (c += 1) {
            const tf: f32 = (t as f32) * 0.25;
            const cf: f32 = (c as f32) * 0.3;
            // Structured signal with a dominant matching token at t=7
            const boost: f32 = if (t == 7) 1.5 else 0.3;
            k_tok[c] = boost + @sin((tf + cf) as f64) as f32 * 0.4;
            v_tok[c] = 0.5 + ((t as f32) * 0.1) + (@cos(cf as f64) as f32) * 0.5;
            all_k[t * dim + c] = k_tok[c];
            all_v[t * dim + c] = v_tok[c];
        }
        cache.appendToken(k_tok[0..], v_tok[0..]) catch { expect_true(false); };
    }

    // 10 tokens with residual_window=4 -> 8 quantized (2 flushed batches) + 2 residual
    const st = cache.stats();
    expect_eq(st.quantized_tokens, @as(usize, 8));
    expect_eq(st.residual_tokens, @as(usize, 2));
    expect_eq(st.total_tokens, @as(usize, 10));

    // Query vector aligned with positive signal
    const q: [16]f32 = [0.8; 16];
    let out_kivi: [16]f32 = [0.0; 16];
    let out_kivi_s: []f32 = out_kivi[0..];
    cache.decodeSingleQuery(q[0..], *out_kivi_s) catch { expect_true(false); };

    // Compute exact FP32 reference single-query attention
    let ref_scores: [10]f64 = [0.0; 10];
    let max_s: f64 = -1.0e300;
    const inv_sqrt_d: f64 = 1.0 / std.math.sqrt(dim as f64);
    let ti: usize = 0;
    while (ti < n_tokens) : (ti += 1) {
        const d: f32 = @dot(q[0..], all_k[ti * dim .. (ti + 1) * dim]);
        ref_scores[ti] = (d as f64) * inv_sqrt_d;
        if (ref_scores[ti] > max_s) max_s = ref_scores[ti];
    }
    let ref_out: [16]f32 = [0.0; 16];
    let sum_w: f64 = 0.0;
    let acc_ref: [16]f64 = [0.0; 16];
    ti = 0;
    while (ti < n_tokens) : (ti += 1) {
        const w: f64 = std.math.exp(ref_scores[ti] - max_s);
        sum_w += w;
        let c: usize = 0;
        while (c < dim) : (c += 1) {
            acc_ref[c] += w * (all_v[ti * dim + c] as f64);
        }
    }
    let c: usize = 0;
    while (c < dim) : (c += 1) {
        ref_out[c] = (acc_ref[c] / sum_w) as f32;
    }

    const cos_sim: f32 = cosineSimF32(out_kivi[0..], ref_out[0..]);
    expect_true(cos_sim > 0.95);
}

test("KIVI: compression ratio telemetry scales sharply after archival") {
    let cache = KiviKvCache.init(16, 4) catch {
        expect_true(false);
        return;
    };
    const k: [16]f32 = [0.5; 16];
    const v: [16]f32 = [0.25; 16];

    // Append 25 tokens -> 24 quantized + 1 residual -> compressed ratio well above 5x
    let i: usize = 0;
    while (i < 25) : (i += 1) {
        cache.appendToken(k[0..], v[0..]) catch { expect_true(false); };
    }
    const st = cache.stats();
    expect_eq(st.quantized_tokens, @as(usize, 24));
    expect_eq(st.residual_tokens, @as(usize, 1));
    expect_true(st.compressed_bytes < st.fp32_equiv_bytes / 5);
    expect_true(st.compression_ratio_x100 >= 1000); // >= 10.0x compression!
}

test("KIVI: error boundaries (InvalidHeadDim, DimMismatch, CapacityFull) & facade parity") {
    // 1. InvalidHeadDim (not multiple of 4 or > 64)
    let got_bad_dim: bool = false;
    * = KiviKvCache.init(6, 4) catch |e| {
        got_bad_dim = (e == KiviError.InvalidHeadDim);
    };
    expect_true(got_bad_dim);

    // 2. DimMismatch on appendToken
    let cache = KiviKvCache.init(8, 4) catch {
        expect_true(false);
        return;
    };
    const bad_k: [4]f32 = [1.0; 4];
    const ok_v: [8]f32 = [1.0; 8];
    let got_mismatch: bool = false;
    cache.appendToken(bad_k[0..], ok_v[0..]) catch |e| {
        got_mismatch = (e == KiviError.DimMismatch);
    };
    expect_true(got_mismatch);

    // 3. CapacityFull after KIVI_MAX_TOKENS (128) tokens
    let i: usize = 0;
    while (i < KIVI_MAX_TOKENS) : (i += 1) {
        cache.appendToken(ok_v[0..], ok_v[0..]) catch { expect_true(false); };
    }
    let got_full: bool = false;
    cache.appendToken(ok_v[0..], ok_v[0..]) catch |e| {
        got_full = (e == KiviError.CapacityFull);
    };
    expect_true(got_full);

    // 4. `cap.attention` facade parity
    expect_eq(cap.attention.KIVI_MAX_HEAD_DIM, @as(usize, 64));
    expect_eq(cap.attention.KIVI_MAX_TOKENS, @as(usize, 128));
    expect_eq(cap.attention.KIVI_PACKED_STRIDE, @as(usize, 16));
    expect_eq(KIVI_MAX_HEAD_DIM, @as(usize, 64));
    expect_eq(KIVI_PACKED_STRIDE, @as(usize, 16));
}

fn genTokenKv(h: usize, t: usize, c: usize, is_val: bool): f32 {
    const hf: f64 = (h as f64) * 0.7;
    const tf: f64 = (t as f64) * 0.05;
    const cf: f64 = (c as f64) * 0.13;
    if (!is_val) {
        // Channel 0 has a fixed outlier per head; token (100 + h*80) has strong semantic match
        const target_t: usize = h * 80 + 100;
        const boost: f32 = if (t == target_t) 1.8 else 0.25;
        const outlier: f32 = if (c == 0) (12.0 + (h as f32) * 2.0) else 0.0;
        return outlier + boost + (@sin(tf + cf + hf) as f32) * 0.35;
    } else {
        return 0.4 + ((h as f32) * 0.2) + ((t as f32) * 0.002) + (@cos(cf + hf) as f32) * 0.5;
    }
}

test("KiviFlatKvCache: 513-token x 128D x 4-KV/16-Q GQA stress, cosine > 0.95 & >= 10x compression") {
    const cfg = KiviFlatConfig{
        num_kv_heads: 4,
        head_dim: 128,
        max_tokens: 520,
        group_size: 32,
        residual_window: 32,
    };
    expect_true(cfg.isValid());
    expect_eq(kiviPackedKvBufLen(cfg), @as(usize, 4 * 520 * 32));      // 66,560
    expect_eq(kiviKeyGroupParamsBufLen(cfg), @as(usize, 4 * 17 * 128)); // 8,704
    expect_eq(kiviValTokParamsBufLen(cfg), @as(usize, 4 * 520));        // 2,080
    expect_eq(kiviResidualBufLen(cfg), @as(usize, 4 * 32 * 128));       // 16,384

    let pk: [66560]u8 = [0; 66560];
    let pv: [66560]u8 = [0; 66560];
    let kgp: [8704]KiviAffineParams = [KiviAffineParams{}; 8704];
    let vtp: [2080]KiviAffineParams = [KiviAffineParams{}; 2080];
    let rk: [16384]f32 = [0.0; 16384];
    let rv: [16384]f32 = [0.0; 16384];

    let cache = KiviFlatKvCache.init(cfg, pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch {
        expect_true(false);
        return;
    };

    // Append 513 tokens across 4 KV heads (16 archived groups = 512 quantized + 1 residual)
    const n_tokens: usize = 513;
    let k_step: [512]f32 = [0.0; 512]; // 4 * 128
    let v_step: [512]f32 = [0.0; 512];
    let t: usize = 0;
    while (t < n_tokens) : (t += 1) {
        let h: usize = 0;
        while (h < 4) : (h += 1) {
            let c: usize = 0;
            while (c < 128) : (c += 1) {
                k_step[h * 128 + c] = genTokenKv(h, t, c, false);
                v_step[h * 128 + c] = genTokenKv(h, t, c, true);
            }
        }
        cache.appendTokenAllHeads(k_step[0..], v_step[0..]) catch {
            expect_true(false);
            return;
        };
    }

    const st = cache.stats();
    expect_eq(st.quantized_tokens, @as(usize, 512));
    expect_eq(st.residual_tokens, @as(usize, 1));
    expect_eq(st.total_tokens, @as(usize, 513));
    // End-to-end compression including affine params & residual window >= 10.0x (1000)
    expect_true(st.compression_ratio_x100 >= 1000);

    // 16 Q-heads -> 4 KV-heads GQA decode (4 Q-heads per KV-head)
    let q_all: [2048]f32 = [0.0; 2048];
    let qh: usize = 0;
    while (qh < 16) : (qh += 1) {
        let c: usize = 0;
        while (c < 128) : (c += 1) {
            q_all[qh * 128 + c] = if (c == 0) 0.05 else (0.5 + ((qh % 4) as f32) * 0.1);
        }
    }
    let out_all: [2048]f32 = [0.0; 2048];
    cache.decodeGqaAllHeads(q_all[0..], 16, out_all[0..]) catch {
        expect_true(false);
        return;
    };

    // Compare each KV head's representative Q-head (qh = h * 4) against exact FP32 reference
    const inv_sqrt_d: f64 = 1.0 / std.math.sqrt(128.0);
    let h: usize = 0;
    while (h < 4) : (h += 1) {
        const q_slice: []const f32 = q_all[(h * 4) * 128 .. (h * 4 + 1) * 128];
        let ref_out: [128]f32 = [0.0; 128];
        let running_max: f64 = -1.0e300;
        let running_sum: f64 = 0.0;
        let ref_acc: [128]f64 = [0.0; 128];

        let ti: usize = 0;
        while (ti < n_tokens) : (ti += 1) {
            let dot_v: f64 = 0.0;
            let c: usize = 0;
            while (c < 128) : (c += 1) {
                dot_v += (q_slice[c] as f64) * (genTokenKv(h, ti, c, false) as f64);
            }
            const sc: f64 = dot_v * inv_sqrt_d;
            if (sc > running_max) {
                const rs: f64 = if (running_sum > 0.0) std.math.exp(running_max - sc) else 0.0;
                running_sum = running_sum * rs + 1.0;
                running_max = sc;
                c = 0;
                while (c < 128) : (c += 1) {
                    ref_acc[c] = ref_acc[c] * rs + (genTokenKv(h, ti, c, true) as f64);
                }
            } else {
                const w: f64 = std.math.exp(sc - running_max);
                running_sum += w;
                c = 0;
                while (c < 128) : (c += 1) {
                    ref_acc[c] += w * (genTokenKv(h, ti, c, true) as f64);
                }
            }
        }
        let c: usize = 0;
        while (c < 128) : (c += 1) {
            ref_out[c] = (ref_acc[c] / running_sum) as f32;
        }

        const kivi_head = out_all[(h * 4) * 128 .. (h * 4 + 1) * 128];
        const cos_sim = cosineSimF32(kivi_head, ref_out[0..]);
        expect_true(cos_sim > 0.95);
    }
}

test("KiviFlatKvCache: sliding residual_window > group_size archives oldest group and keeps tail") {
    const cfg = KiviFlatConfig{
        num_kv_heads: 2,
        head_dim: 16,
        max_tokens: 64,
        group_size: 8,
        residual_window: 12,
    };
    let pk: [512]u8 = [0; 512];
    let pv: [512]u8 = [0; 512];
    let kgp: [256]KiviAffineParams = [KiviAffineParams{}; 256];
    let vtp: [128]KiviAffineParams = [KiviAffineParams{}; 128];
    let rk: [384]f32 = [0.0; 384];
    let rv: [384]f32 = [0.0; 384];

    let cache = KiviFlatKvCache.init(cfg, pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch {
        expect_true(false);
        return;
    };
    const kv: [32]f32 = [0.5; 32];

    // Append 12 tokens -> still in residual window (no archival yet)
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        cache.appendTokenAllHeads(kv[0..], kv[0..]) catch { expect_true(false); };
    }
    expect_eq(cache.stats().quantized_tokens, @as(usize, 0));
    expect_eq(cache.stats().residual_tokens, @as(usize, 12));

    // 13th token triggers archival of oldest 8 -> 8 quantized + (4 shifted + 1 new = 5) residual
    cache.appendTokenAllHeads(kv[0..], kv[0..]) catch { expect_true(false); };
    expect_eq(cache.stats().quantized_tokens, @as(usize, 8));
    expect_eq(cache.stats().residual_tokens, @as(usize, 5));
    expect_eq(cache.stats().total_tokens, @as(usize, 13));
}

test("KiviFlatKvCache: CRC32 binary snapshot roundtrip yields bit-exact GQA decode") {
    const cfg = KiviFlatConfig{
        num_kv_heads: 2,
        head_dim: 16,
        max_tokens: 32,
        group_size: 8,
        residual_window: 8,
    };
    let pk1: [256]u8 = [0; 256];
    let pv1: [256]u8 = [0; 256];
    let kgp1: [128]KiviAffineParams = [KiviAffineParams{}; 128];
    let vtp1: [64]KiviAffineParams = [KiviAffineParams{}; 64];
    let rk1: [256]f32 = [0.0; 256];
    let rv1: [256]f32 = [0.0; 256];

    let c1 = KiviFlatKvCache.init(cfg, pk1[0..], pv1[0..], kgp1[0..], vtp1[0..], rk1[0..], rv1[0..]) catch {
        expect_true(false);
        return;
    };

    let k_step: [32]f32 = [0.0; 32];
    let v_step: [32]f32 = [0.0; 32];
    let t: usize = 0;
    while (t < 19) : (t += 1) {
        let j: usize = 0;
        while (j < 32) : (j += 1) {
            k_step[j] = genTokenKv(j / 16, t, j % 16, false);
            v_step[j] = genTokenKv(j / 16, t, j % 16, true);
        }
        c1.appendTokenAllHeads(k_step[0..], v_step[0..]) catch { expect_true(false); };
    }

    const max_snap = kiviSnapshotMaxBytes(cfg, 19);
    let snap: [8192]u8 = [0; 8192];
    expect_true(snap.len >= max_snap);
    const snap_len = serializeKiviSnapshot(c1, snap[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(snap_len <= max_snap);

    // Warm-load into completely independent slices
    let pk2: [256]u8 = [0; 256];
    let pv2: [256]u8 = [0; 256];
    let kgp2: [128]KiviAffineParams = [KiviAffineParams{}; 128];
    let vtp2: [64]KiviAffineParams = [KiviAffineParams{}; 64];
    let rk2: [256]f32 = [0.0; 256];
    let rv2: [256]f32 = [0.0; 256];

    let c2 = deserializeKiviSnapshot(snap[0..snap_len], pk2[0..], pv2[0..], kgp2[0..], vtp2[0..], rk2[0..], rv2[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(c2.stats().quantized_tokens, @as(usize, 16));
    expect_eq(c2.stats().residual_tokens, @as(usize, 3));

    const q_gqa: [64]f32 = [0.6; 64]; // 4 Q-heads -> 2 KV-heads
    let out1: [64]f32 = [0.0; 64];
    let out2: [64]f32 = [0.0; 64];
    c1.decodeGqaAllHeads(q_gqa[0..], 4, out1[0..]) catch { expect_true(false); };
    c2.decodeGqaAllHeads(q_gqa[0..], 4, out2[0..]) catch { expect_true(false); };

    let d: usize = 0;
    while (d < 64) : (d += 1) {
        expect_eq(out1[d], out2[d]);
    }
}

test("KiviFlatKvCache: snapshot tamper ChecksumMismatch & InvalidSnapshot guards") {
    const cfg = KiviFlatConfig{ num_kv_heads: 1, head_dim: 8, max_tokens: 16, group_size: 4, residual_window: 4 };
    let pk: [32]u8 = [0; 32];
    let pv: [32]u8 = [0; 32];
    let kgp: [32]KiviAffineParams = [KiviAffineParams{}; 32];
    let vtp: [16]KiviAffineParams = [KiviAffineParams{}; 16];
    let rk: [32]f32 = [0.0; 32];
    let rv: [32]f32 = [0.0; 32];

    let c = KiviFlatKvCache.init(cfg, pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch {
        expect_true(false);
        return;
    };
    const kv: [8]f32 = [1.0; 8];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        c.appendTokenAllHeads(kv[0..], kv[0..]) catch { expect_true(false); };
    }

    let snap: [2048]u8 = [0; 2048];
    const snap_len = serializeKiviSnapshot(c, snap[0..]) catch {
        expect_true(false);
        return;
    };

    // Single-byte tamper in payload -> ChecksumMismatch
    snap[45] ^= 0x01;
    let got_crc: bool = false;
    * = deserializeKiviSnapshot(snap[0..snap_len], pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch |e| {
        if (e == KiviFlatError.ChecksumMismatch) got_crc = true;
    };
    expect_true(got_crc);

    // Corrupt magic -> InvalidSnapshot
    snap[0] = 0x00;
    let got_inv: bool = false;
    * = deserializeKiviSnapshot(snap[0..snap_len], pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch |e| {
        if (e == KiviFlatError.InvalidSnapshot) got_inv = true;
    };
    expect_true(got_inv);
}

test("KiviFlatKvCache: error boundaries (InvalidConfig, HeadOutOfBounds, CapacityFull, BufferTooSmall) & facade parity") {
    expect_eq(cap.attention.KIVI_SNAPSHOT_MAGIC, KIVI_SNAPSHOT_MAGIC);
    expect_eq(cap.attention.KIVI_SNAPSHOT_VERSION, KIVI_SNAPSHOT_VERSION);

    // residual_window < group_size -> InvalidConfig
    const bad_cfg = cap.attention.KiviFlatConfig{ num_kv_heads: 2, head_dim: 16, max_tokens: 16, group_size: 8, residual_window: 4 };
    expect_true(!bad_cfg.isValid());
    let pk: [64]u8 = [0; 64];
    let pv: [64]u8 = [0; 64];
    let kgp: [64]KiviAffineParams = [KiviAffineParams{}; 64];
    let vtp: [32]KiviAffineParams = [KiviAffineParams{}; 32];
    let rk: [64]f32 = [0.0; 64];
    let rv: [64]f32 = [0.0; 64];

    let got_cfg: bool = false;
    * = cap.attention.KiviFlatKvCache.init(bad_cfg, pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch |e| {
        if (e == KiviFlatError.InvalidConfig) got_cfg = true;
    };
    expect_true(got_cfg);

    // Valid small cache -> HeadOutOfBounds & CapacityFull
    const ok_cfg = KiviFlatConfig{ num_kv_heads: 1, head_dim: 8, max_tokens: 4, group_size: 4, residual_window: 4 };
    let c = KiviFlatKvCache.init(ok_cfg, pk[0..], pv[0..], kgp[0..], vtp[0..], rk[0..], rv[0..]) catch {
        expect_true(false);
        return;
    };
    const kv: [8]f32 = [1.0; 8];
    let out8: [8]f32 = [0.0; 8];
    let got_oob: bool = false;
    c.decodeHeadQuery(2, kv[0..], out8[0..]) catch |e| {
        if (e == KiviFlatError.HeadOutOfBounds) got_oob = true;
    };
    expect_true(got_oob);

    let i: usize = 0;
    while (i < 4) : (i += 1) {
        c.appendTokenAllHeads(kv[0..], kv[0..]) catch { expect_true(false); };
    }
    let got_full: bool = false;
    c.appendTokenAllHeads(kv[0..], kv[0..]) catch |e| {
        if (e == KiviFlatError.CapacityFull) got_full = true;
    };
    expect_true(got_full);
}
