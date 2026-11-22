// Origin: Pure-CP Stdlib (AI Inference Multi-Head Latent Attention MLA & ALiBi Slopes) | Created: 2026-11-22
// Zero-heap-allocation Multi-Head Latent Attention (DeepSeek-V2/V3 MLA with KV latent compression,
// decoupled RoPE, and weight-absorption decode) & ALiBi linear distance bias attention kernels.

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Configuration descriptor for DeepSeek-V2/V3 Multi-Head Latent Attention (MLA).
pub struct MlaConfig {
    d_model: usize = 0;
    d_latent: usize = 0;
    n_heads: usize = 0;
    d_nope: usize = 0;
    d_rope: usize = 0;
    d_v: usize = 0;
}

/// Computes ALiBi per-head linear distance slopes `m_h` (Press et al., 2022).
/// - When `n_heads` is a power of two: `m_h = 2^(-8 * (h + 1) / n_heads)`.
/// - When `n_heads` is not a power of two: first `p` slopes use the largest power-of-two `p < n_heads`,
///   and the remaining `n_heads - p` slopes interleave odd powers from `2 * p`.
///
/// Returns `false` if `n_heads == 0` or `slopes_out.len < n_heads`.
pub fn alibiSlopes(n_heads: usize, slopes_out: *[]f64): bool {
    if (n_heads == 0 || slopes_out.len < n_heads) return false;

    let closest_pow2: usize = 1;
    while (closest_pow2 <= n_heads / 2) {
        closest_pow2 <<= 1;
    }

    const base_p = std.math.pow(f64, 2.0, -8.0 / (closest_pow2 as f64));
    let cur = base_p;
    let h: usize = 0;
    while (h < closest_pow2) : (h += 1) {
        slopes_out[h] = cur;
        cur *= base_p;
    }

    if (closest_pow2 < n_heads) {
        const extra = n_heads - closest_pow2;
        const base_2p = std.math.pow(f64, 2.0, -8.0 / ((closest_pow2 * 2) as f64));
        const step_2p = base_2p * base_2p;
        let cur_extra = base_2p;
        let k: usize = 0;
        while (k < extra) : (k += 1) {
            slopes_out[closest_pow2 + k] = cur_extra;
            cur_extra *= step_2p;
        }
    }

    return true;
}

/// Zero-heap-allocation online-Softmax single-query multi-head ALiBi attention.
/// Never materializes an `N x N` or `seq_kv` score buffer.
///
/// For each head `h` and key position `j in 0..seq_kv`:
/// `score[h, j] = (q[h] · k[h, j]) / sqrt(head_dim) - slopes[h] * ((seq_kv - 1) - j)`.
///
/// Layouts:
/// - `q_heads`, `out_heads`: `[n_heads, head_dim]` row-major
/// - `k_heads`, `v_heads`: `[n_heads, seq_kv, head_dim]` row-major
/// - `slopes`: `[n_heads]`
///
/// Returns `false` on zero dimensions, undersized slices, or NaN/Inf inputs.
pub fn alibiAttentionSingleQuery(
    q_heads: []const f64,
    k_heads: []const f64,
    v_heads: []const f64,
    seq_kv: usize,
    n_heads: usize,
    head_dim: usize,
    slopes: []const f64,
    out_heads: *[]f64,
): bool {
    if (seq_kv == 0 || n_heads == 0 || head_dim == 0) return false;
    if (n_heads > std.math.maxInt(usize) / head_dim) return false;
    const q_elems = n_heads * head_dim;
    if (seq_kv > std.math.maxInt(usize) / q_elems) return false;
    const kv_elems = q_elems * seq_kv;

    if (q_heads.len < q_elems || out_heads.len < q_elems) return false;
    if (k_heads.len < kv_elems || v_heads.len < kv_elems) return false;
    if (slopes.len < n_heads) return false;

    let ih: usize = 0;
    while (ih < n_heads) : (ih += 1) {
        if (!isFiniteF64(slopes[ih])) return false;
    }
    let iq: usize = 0;
    while (iq < q_elems) : (iq += 1) {
        if (!isFiniteF64(q_heads[iq])) return false;
    }
    let ikv: usize = 0;
    while (ikv < kv_elems) : (ikv += 1) {
        if (!isFiniteF64(k_heads[ikv]) || !isFiniteF64(v_heads[ikv])) return false;
    }

    const scale = 1.0 / std.math.sqrt(head_dim as f64);
    const kv_head_stride = seq_kv * head_dim;

    let h: usize = 0;
    while (h < n_heads) : (h += 1) {
        const q_base = h * head_dim;
        const kv_base = h * kv_head_stride;
        const slope = slopes[h];

        let d0: usize = 0;
        while (d0 < head_dim) : (d0 += 1) {
            out_heads[q_base + d0] = 0.0;
        }

        let max_s: f64 = 0.0 - std.math.inf(f64);
        let sum_e: f64 = 0.0;

        let j: usize = 0;
        while (j < seq_kv) : (j += 1) {
            const k_row = kv_base + j * head_dim;
            let dot: f64 = 0.0;
            let d: usize = 0;
            while (d < head_dim) : (d += 1) {
                dot += q_heads[q_base + d] * k_heads[k_row + d];
            }
            const dist = ((seq_kv - 1) - j) as f64;
            const score = dot * scale - slope * dist;
            if (!isFiniteF64(score)) return false;

            if (sum_e == 0.0) {
                max_s = score;
                sum_e = 1.0;
                let d1: usize = 0;
                while (d1 < head_dim) : (d1 += 1) {
                    out_heads[q_base + d1] = v_heads[k_row + d1];
                }
            } else if (score > max_s) {
                const alpha = std.math.exp(max_s - score);
                sum_e = sum_e * alpha + 1.0;
                max_s = score;
                let d1: usize = 0;
                while (d1 < head_dim) : (d1 += 1) {
                    out_heads[q_base + d1] = out_heads[q_base + d1] * alpha + v_heads[k_row + d1];
                }
            } else {
                const w = std.math.exp(score - max_s);
                sum_e += w;
                let d1: usize = 0;
                while (d1 < head_dim) : (d1 += 1) {
                    out_heads[q_base + d1] += w * v_heads[k_row + d1];
                }
            }
        }

        if (!(sum_e > 0.0) || !isFiniteF64(sum_e)) return false;
        const inv_sum = 1.0 / sum_e;
        let d2: usize = 0;
        while (d2 < head_dim) : (d2 += 1) {
            out_heads[q_base + d2] *= inv_sum;
        }
    }

    return true;
}

/// Compresses a hidden token vector `x` (`[d_model]`) into a low-rank KV latent vector
/// `latent_out` (`[d_latent]`) via `latent_out = w_dkv * x`, where `w_dkv` has row-major shape
/// `[d_latent, d_model]`.
///
/// Returns `false` on zero dimensions, undersized slices, or NaN/Inf inputs.
pub fn mlaCompressKv(
    x: []const f64,
    w_dkv: []const f64,
    d_model: usize,
    d_latent: usize,
    latent_out: *[]f64,
): bool {
    if (d_model == 0 || d_latent == 0) return false;
    if (d_latent > std.math.maxInt(usize) / d_model) return false;
    const w_elems = d_latent * d_model;
    if (x.len < d_model || w_dkv.len < w_elems || latent_out.len < d_latent) return false;

    let ix: usize = 0;
    while (ix < d_model) : (ix += 1) {
        if (!isFiniteF64(x[ix])) return false;
    }
    let iw: usize = 0;
    while (iw < w_elems) : (iw += 1) {
        if (!isFiniteF64(w_dkv[iw])) return false;
    }

    let c: usize = 0;
    while (c < d_latent) : (c += 1) {
        const row = c * d_model;
        let acc: f64 = 0.0;
        let m: usize = 0;
        while (m < d_model) : (m += 1) {
            acc += w_dkv[row + m] * x[m];
        }
        latent_out[c] = acc;
    }

    return true;
}

/// DeepSeek-V2/V3 Multi-Head Latent Attention (MLA) single-head decode kernel with
/// **Weight Absorption** optimization (zero K/V cache reconstruction!):
///
/// 1. Absorbs `w_uk` (`[d_nope, d_latent]`) into `q_nope`: `q_absorbed = q_nope^T * w_uk` (`[d_latent]`).
/// 2. Streams over cached low-rank latents `c_kv_cache` (`[seq_kv, d_latent]`) and decoupled RoPE keys
///    `k_rope_cache` (`[seq_kv, d_rope]`):
///    `score_j = scale * (q_absorbed · c_kv[j] + q_rope · k_rope[j])`,
///    aggregating `out_latent = sum_j softmax(score)_j * c_kv[j]` via O(1)-memory online Softmax.
/// 3. Projects the aggregated latent once through `w_uv` (`[d_v, d_latent]`): `out_v = w_uv * out_latent`.
///
/// If `scale <= 0.0`, defaults to `1.0 / sqrt(d_nope + d_rope)`. `d_rope == 0` disables the RoPE term.
/// Returns `false` on zero dimensions, undersized slices, or NaN/Inf inputs.
pub fn mlaDecodeSingleHead(
    q_nope: []const f64,
    q_rope: []const f64,
    c_kv_cache: []const f64,
    k_rope_cache: []const f64,
    w_uk: []const f64,
    w_uv: []const f64,
    seq_kv: usize,
    d_latent: usize,
    d_nope: usize,
    d_rope: usize,
    d_v: usize,
    scale: f64,
    q_absorbed_s: *[]f64,
    out_latent_s: *[]f64,
    out_v: *[]f64,
): bool {
    if (seq_kv == 0 || d_latent == 0 || d_nope == 0 || d_v == 0) return false;
    if (!isFiniteF64(scale)) return false;

    if (seq_kv > std.math.maxInt(usize) / d_latent) return false;
    if (d_nope > std.math.maxInt(usize) / d_latent) return false;
    if (d_v > std.math.maxInt(usize) / d_latent) return false;
    if (d_rope > 0 && seq_kv > std.math.maxInt(usize) / d_rope) return false;

    const c_elems = seq_kv * d_latent;
    const kr_elems = seq_kv * d_rope;
    const wuk_elems = d_nope * d_latent;
    const wuv_elems = d_v * d_latent;

    if (q_nope.len < d_nope || q_rope.len < d_rope) return false;
    if (c_kv_cache.len < c_elems || k_rope_cache.len < kr_elems) return false;
    if (w_uk.len < wuk_elems || w_uv.len < wuv_elems) return false;
    if (q_absorbed_s.len < d_latent || out_latent_s.len < d_latent || out_v.len < d_v) return false;

    let i_qn: usize = 0;
    while (i_qn < d_nope) : (i_qn += 1) {
        if (!isFiniteF64(q_nope[i_qn])) return false;
    }
    let i_qr: usize = 0;
    while (i_qr < d_rope) : (i_qr += 1) {
        if (!isFiniteF64(q_rope[i_qr])) return false;
    }
    let i_c: usize = 0;
    while (i_c < c_elems) : (i_c += 1) {
        if (!isFiniteF64(c_kv_cache[i_c])) return false;
    }
    let i_kr: usize = 0;
    while (i_kr < kr_elems) : (i_kr += 1) {
        if (!isFiniteF64(k_rope_cache[i_kr])) return false;
    }
    let i_uk: usize = 0;
    while (i_uk < wuk_elems) : (i_uk += 1) {
        if (!isFiniteF64(w_uk[i_uk])) return false;
    }
    let i_uv: usize = 0;
    while (i_uv < wuv_elems) : (i_uv += 1) {
        if (!isFiniteF64(w_uv[i_uv])) return false;
    }

    const eff_scale = if (scale > 0.0)
        scale
    else
        1.0 / std.math.sqrt((d_nope + d_rope) as f64);

    // Step 1: Absorb W_UK into q_nope -> q_absorbed_s[c] = sum_i q_nope[i] * w_uk[i * d_latent + c]
    let c0: usize = 0;
    while (c0 < d_latent) : (c0 += 1) {
        let acc: f64 = 0.0;
        let i_n: usize = 0;
        while (i_n < d_nope) : (i_n += 1) {
            acc += q_nope[i_n] * w_uk[i_n * d_latent + c0];
        }
        q_absorbed_s[c0] = acc;
        out_latent_s[c0] = 0.0;
    }

    // Step 2: Online Softmax over cached latents + decoupled RoPE keys
    let max_s: f64 = 0.0 - std.math.inf(f64);
    let sum_e: f64 = 0.0;

    let j: usize = 0;
    while (j < seq_kv) : (j += 1) {
        const c_row = j * d_latent;
        let dot: f64 = 0.0;
        let c1: usize = 0;
        while (c1 < d_latent) : (c1 += 1) {
            dot += q_absorbed_s[c1] * c_kv_cache[c_row + c1];
        }
        if (d_rope > 0) {
            const kr_row = j * d_rope;
            let r: usize = 0;
            while (r < d_rope) : (r += 1) {
                dot += q_rope[r] * k_rope_cache[kr_row + r];
            }
        }

        const score = dot * eff_scale;
        if (!isFiniteF64(score)) return false;

        if (sum_e == 0.0) {
            max_s = score;
            sum_e = 1.0;
            let c2: usize = 0;
            while (c2 < d_latent) : (c2 += 1) {
                out_latent_s[c2] = c_kv_cache[c_row + c2];
            }
        } else if (score > max_s) {
            const alpha = std.math.exp(max_s - score);
            sum_e = sum_e * alpha + 1.0;
            max_s = score;
            let c2: usize = 0;
            while (c2 < d_latent) : (c2 += 1) {
                out_latent_s[c2] = out_latent_s[c2] * alpha + c_kv_cache[c_row + c2];
            }
        } else {
            const w = std.math.exp(score - max_s);
            sum_e += w;
            let c2: usize = 0;
            while (c2 < d_latent) : (c2 += 1) {
                out_latent_s[c2] += w * c_kv_cache[c_row + c2];
            }
        }
    }

    if (!(sum_e > 0.0) || !isFiniteF64(sum_e)) return false;
    const inv_sum = 1.0 / sum_e;
    let c3: usize = 0;
    while (c3 < d_latent) : (c3 += 1) {
        out_latent_s[c3] *= inv_sum;
    }

    // Step 3: Single up-projection W_UV * out_latent -> out_v
    let o: usize = 0;
    while (o < d_v) : (o += 1) {
        const uv_row = o * d_latent;
        let acc: f64 = 0.0;
        let c4: usize = 0;
        while (c4 < d_latent) : (c4 += 1) {
            acc += w_uv[uv_row + c4] * out_latent_s[c4];
        }
        out_v[o] = acc;
    }

    return true;
}
