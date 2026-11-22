// Origin: Pure-CP Stdlib (AI Inference Online INT8 KV-Cache Quantization & Fused Attention) | Created: 2026-11-22
// Pure CP zero-allocation per-token symmetric INT8 KV-Cache quantization (4x memory compression,
// vLLM/KIVI style), scale-hoisted query-key dot product, and O(1)-extra-memory Online Softmax
// fused single-query attention directly over INT8 quantized KV-Cache.

const std = @import("std");

/// Per-token quantization metadata for a single KV vector slot.
/// Symmetric INT8 quantization stores `zero_point = 0.0` alongside raw `[min_val, max_val]` telemetry.
pub struct KvQuantParams {
    scale: f32 = 0.0,
    zero_point: f32 = 0.0,
    min_val: f32 = 0.0,
    max_val: f32 = 0.0,
}

// ============================================================================
// Internal Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isValidParams(p: KvQuantParams): bool {
    return isFiniteF32(p.scale) && p.scale >= 0.0 && isFiniteF32(p.zero_point);
}

// ============================================================================
// 1. Online Per-Token Symmetric INT8 Quantization & Dequantization
// ============================================================================

/// Quantizes a single token's K or V vector symmetrically into `[-127, 127]` INT8.
/// Returns `null` with zero side effects if `vec.len == 0`, `out_q.len < vec.len`,
/// or `vec` contains any `NaN`/`Inf`.
pub fn quantizeTokenKvI8(vec: []const f32, out_q: *[]i8): KvQuantParams? {
    if (vec.len == 0 || out_q.len < vec.len) {
        return null;
    }

    // Atomic pre-scan for NaN/Inf and min/max/amax.
    let min_v: f32 = vec[0];
    let max_v: f32 = vec[0];
    let amax: f32 = 0.0;
    let i: usize = 0;
    while (i < vec.len) : (i += 1) {
        const v: f32 = vec[i];
        if (!isFiniteF32(v)) {
            return null;
        }
        if (v < min_v) {
            min_v = v;
        }
        if (v > max_v) {
            max_v = v;
        }
        const av: f32 = @abs(v);
        if (av > amax) {
            amax = av;
        }
    }

    if (amax == 0.0) {
        let z: usize = 0;
        while (z < vec.len) : (z += 1) {
            out_q[z] = 0;
        }
        return KvQuantParams{
            scale: 0.0,
            zero_point: 0.0,
            min_val: 0.0,
            max_val: 0.0,
        };
    }

    const scale: f32 = amax / 127.0;
    const inv_scale: f32 = 127.0 / amax;
    let j: usize = 0;
    while (j < vec.len) : (j += 1) {
        let scaled: f32 = vec[j] * inv_scale;
        if (scaled > 127.0) {
            scaled = 127.0;
        }
        if (scaled < -127.0) {
            scaled = -127.0;
        }
        const rounded_f: f32 = @round(scaled);
        out_q[j] = rounded_f as i8;
    }

    return KvQuantParams{
        scale: scale,
        zero_point: 0.0,
        min_val: min_v,
        max_val: max_v,
    };
}

/// Dequantizes a per-token INT8 KV vector back to `f32`.
/// Returns `false` if lengths are invalid or `params` contains non-finite/negative scale.
pub fn dequantizeTokenKvI8(q: []const i8, params: KvQuantParams, out_vec: *[]f32): bool {
    if (q.len == 0 || out_vec.len < q.len || !isValidParams(params)) {
        return false;
    }

    if (params.scale == 0.0) {
        let i: usize = 0;
        while (i < q.len) : (i += 1) {
            out_vec[i] = params.zero_point;
        }
        return true;
    }

    let i: usize = 0;
    while (i < q.len) : (i += 1) {
        out_vec[i] = ((q[i] as f32) * params.scale) + params.zero_point;
    }
    return true;
}

// ============================================================================
// 2. Scale-Hoisted Query-Key Dot Product (Zero Dequantization Buffer)
// ============================================================================

/// Computes `dot(q_vec, dequant(k_q, k_params))` without allocating a temporary `f32` key buffer.
/// Hoists `k_params.scale` outside the inner loop and accumulates in `f64` for precision.
/// Returns `null` on dimension mismatch, empty inputs, or non-finite values.
pub fn dotQueryQuantKeyI8(q_vec: []const f32, k_q: []const i8, k_params: KvQuantParams): f64? {
    if (q_vec.len == 0 || q_vec.len != k_q.len || !isValidParams(k_params)) {
        return null;
    }

    let dot_unscaled: f64 = 0.0;
    let q_sum: f64 = 0.0;
    const has_zp: bool = k_params.zero_point != 0.0;

    let i: usize = 0;
    while (i < q_vec.len) : (i += 1) {
        const qv: f32 = q_vec[i];
        if (!isFiniteF32(qv)) {
            return null;
        }
        const qd: f64 = qv as f64;
        dot_unscaled += qd * (k_q[i] as f64);
        if (has_zp) {
            q_sum += qd;
        }
    }

    if (k_params.scale == 0.0 && !has_zp) {
        return 0.0;
    }
    return dot_unscaled * (k_params.scale as f64) + q_sum * (k_params.zero_point as f64);
}

// ============================================================================
// 3. Fused Single-Query Online Softmax Attention over INT8 KV-Cache
// ============================================================================

/// Computes scaled dot-product attention for a single decode query directly over INT8-quantized
/// `keys_q` and `vals_q` (`[seq_len, head_dim]` row-major) using O(1)-extra-memory Online Softmax.
///
/// Requires zero heap allocation and zero `O(seq_len)` temporary score buffers.
pub fn quantizedAttentionSingleQuery(
    query: []const f32,
    keys_q: []const i8,
    k_params: []const KvQuantParams,
    vals_q: []const i8,
    v_params: []const KvQuantParams,
    seq_len: usize,
    head_dim: usize,
    out: *[]f32,
): bool {
    if (seq_len == 0 || head_dim == 0) {
        return false;
    }
    if (seq_len > std.math.maxInt(usize) / head_dim) {
        return false;
    }
    const total_elems: usize = seq_len * head_dim;
    if (query.len < head_dim || out.len < head_dim) {
        return false;
    }
    if (keys_q.len < total_elems || vals_q.len < total_elems) {
        return false;
    }
    if (k_params.len < seq_len || v_params.len < seq_len) {
        return false;
    }

    // Atomic validation of query and all per-token quantization parameters.
    let qd_idx: usize = 0;
    while (qd_idx < head_dim) : (qd_idx += 1) {
        if (!isFiniteF32(query[qd_idx])) {
            return false;
        }
    }
    let ti: usize = 0;
    while (ti < seq_len) : (ti += 1) {
        if (!isValidParams(k_params[ti]) || !isValidParams(v_params[ti])) {
            return false;
        }
    }

    const inv_sqrt_d: f64 = 1.0 / @sqrt(head_dim as f64);
    let max_score: f64 = -(std.math.inf(f64));
    let sum_exp: f64 = 0.0;

    let d_init: usize = 0;
    while (d_init < head_dim) : (d_init += 1) {
        out[d_init] = 0.0;
    }

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        const base: usize = t * head_dim;
        const kp = k_params[t];
        const vp = v_params[t];

        // 1. Scale-hoisted Q * K_t dot product.
        let dot_unscaled: f64 = 0.0;
        let q_sum: f64 = 0.0;
        const k_has_zp: bool = kp.zero_point != 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            const qv: f64 = query[d] as f64;
            dot_unscaled += qv * (keys_q[base + d] as f64);
            if (k_has_zp) {
                q_sum += qv;
            }
        }
        const dot_val: f64 = dot_unscaled * (kp.scale as f64) + q_sum * (kp.zero_point as f64);
        const score: f64 = dot_val * inv_sqrt_d;

        // 2. Numerically stable Online Softmax update with hoisted V scale.
        if (t == 0) {
            max_score = score;
            sum_exp = 1.0;
            const v_sc: f64 = vp.scale as f64;
            const v_zp: f64 = vp.zero_point as f64;
            d = 0;
            while (d < head_dim) : (d += 1) {
                out[d] = ((vals_q[base + d] as f64) * v_sc + v_zp) as f32;
            }
        } else if (score > max_score) {
            const rescale: f64 = std.math.exp(max_score - score);
            sum_exp = sum_exp * rescale + 1.0;
            max_score = score;
            const v_sc: f64 = vp.scale as f64;
            const v_zp: f64 = vp.zero_point as f64;
            d = 0;
            while (d < head_dim) : (d += 1) {
                const v_val: f64 = (vals_q[base + d] as f64) * v_sc + v_zp;
                out[d] = ((out[d] as f64) * rescale + v_val) as f32;
            }
        } else {
            const w_t: f64 = std.math.exp(score - max_score);
            sum_exp += w_t;
            const w_v_sc: f64 = w_t * (vp.scale as f64);
            const w_v_zp: f64 = w_t * (vp.zero_point as f64);
            d = 0;
            while (d < head_dim) : (d += 1) {
                const contrib: f64 = (vals_q[base + d] as f64) * w_v_sc + w_v_zp;
                out[d] = ((out[d] as f64) + contrib) as f32;
            }
        }
    }

    // 3. Final normalization by Online Softmax denominator.
    const inv_sum: f64 = 1.0 / sum_exp;
    let df: usize = 0;
    while (df < head_dim) : (df += 1) {
        out[df] = ((out[df] as f64) * inv_sum) as f32;
    }
    return true;
}
