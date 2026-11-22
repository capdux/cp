// Origin: Pure-CP Stdlib (Transformer Kernels: RoPE with NTK Scaling, RMSNorm & SwiGLU/GeLU) | Created: 2026-11-22
// Zero-heap-allocation Transformer inference kernels:
//   - Rotary Position Embedding (RoPE, Su et al. 2021) with NTK-Aware long-context base scaling
//   - RMSNorm & LayerNorm normalization kernels
//   - SiLU, Hendrycks-Gimpel GeLU (tanh approx), and SwiGLU FFN activation kernels

const std = @import("std");

/// Configuration for Rotary Position Embedding (RoPE) with optional NTK-Aware scaling.
pub struct RopeConfig {
    head_dim: usize = 64;
    base_theta: f64 = 10000.0;
    ntk_scale: f64 = 1.0;

    /// Returns standard Llama-style RoPE configuration (`base_theta = 10000.0`, `ntk_scale = 1.0`).
    static fn default(head_dim: usize): RopeConfig {
        return RopeConfig{
            head_dim: head_dim,
            base_theta: 10000.0,
            ntk_scale: 1.0,
        };
    }
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Precomputes compact `[max_seq_len][head_dim / 2]` cosine and sine lookup tables for RoPE.
/// Supports NTK-Aware base scaling (`eff_base = base_theta * ntk_scale^(d / (d - 2))` for `d > 2`).
/// Returns `false` if `head_dim` is 0 or odd, `base_theta <= 1.0`, `ntk_scale <= 0.0`,
/// non-finite parameters, or output slices are too small.
pub fn ropePrecomputeTables(
    cfg: RopeConfig,
    max_seq_len: usize,
    cos_out: *[]f64,
    sin_out: *[]f64,
): bool {
    if (cfg.head_dim == 0 || (cfg.head_dim % 2) != 0) return false;
    if (!isFiniteF64(cfg.base_theta) || cfg.base_theta <= 1.0) return false;
    if (!isFiniteF64(cfg.ntk_scale) || cfg.ntk_scale <= 0.0) return false;

    const half_dim = cfg.head_dim / 2;
    if (max_seq_len > 0 && max_seq_len > std.math.maxInt(usize) / half_dim) return false;
    const needed = max_seq_len * half_dim;
    if (cos_out.len < needed || sin_out.len < needed) return false;
    if (max_seq_len == 0) return true;

    const d_f = cfg.head_dim as f64;
    let eff_base = cfg.base_theta;
    if (cfg.ntk_scale != 1.0) {
        if (cfg.head_dim > 2) {
            eff_base = cfg.base_theta * std.math.pow(f64, cfg.ntk_scale, d_f / (d_f - 2.0));
        } else {
            eff_base = cfg.base_theta * cfg.ntk_scale;
        }
    }
    if (!isFiniteF64(eff_base) || eff_base <= 1.0) return false;

    let j: usize = 0;
    while (j < half_dim) : (j += 1) {
        const exp_val = ((2 * j) as f64) / d_f;
        const inv_freq = 1.0 / std.math.pow(f64, eff_base, exp_val);
        let pos: usize = 0;
        while (pos < max_seq_len) : (pos += 1) {
            const angle = (pos as f64) * inv_freq;
            const idx = pos * half_dim + j;
            cos_out[idx] = std.math.cos(angle);
            sin_out[idx] = std.math.sin(angle);
        }
    }
    return true;
}

/// Applies norm-preserving half-split RoPE rotation in-place to a single head vector `vec_inout[0..head_dim]`
/// at sequence position `pos`, using precomputed `[max_seq_len][head_dim / 2]` tables.
/// Returns `false` if `head_dim` is 0 or odd, `pos` exceeds table length, or inputs contain NaN/Inf.
pub fn ropeApplyInPlace(
    vec_inout: *[]f64,
    pos: usize,
    head_dim: usize,
    cos_table: []const f64,
    sin_table: []const f64,
): bool {
    if (head_dim == 0 || (head_dim % 2) != 0) return false;
    if (vec_inout.len < head_dim) return false;

    const half_dim = head_dim / 2;
    if (pos > std.math.maxInt(usize) / half_dim) return false;
    const off = pos * half_dim;
    if (off > cos_table.len || half_dim > cos_table.len - off) return false;
    if (off > sin_table.len || half_dim > sin_table.len - off) return false;

    let vi: usize = 0;
    while (vi < head_dim) : (vi += 1) {
        if (!isFiniteF64(vec_inout[vi])) return false;
    }
    let ti: usize = 0;
    while (ti < half_dim) : (ti += 1) {
        if (!isFiniteF64(cos_table[off + ti]) || !isFiniteF64(sin_table[off + ti])) return false;
    }

    let j: usize = 0;
    while (j < half_dim) : (j += 1) {
        const x0 = vec_inout[j];
        const x1 = vec_inout[j + half_dim];
        const c = cos_table[off + j];
        const s = sin_table[off + j];
        vec_inout[j] = x0 * c - x1 * s;
        vec_inout[j + half_dim] = x0 * s + x1 * c;
    }
    return true;
}

/// Root Mean Square Layer Normalization (RMSNorm, Llama / Mistral / Qwen standard).
/// `out[i] = x[i] / sqrt(mean(x^2) + eps) * weight[i]` (empty `weight` defaults to all 1.0).
/// Safe for both in-place (`out` aliasing `x`) and out-of-place usage.
pub fn rmsNorm(x: []const f64, weight: []const f64, eps: f64, out: *[]f64): bool {
    if (x.len == 0 || out.len < x.len) return false;
    if (weight.len != 0 && weight.len != x.len) return false;
    if (!isFiniteF64(eps) || eps < 0.0) return false;

    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const v = x[i];
        if (!isFiniteF64(v)) return false;
        if (weight.len != 0 && !isFiniteF64(weight[i])) return false;
        sum_sq += v * v;
    }
    if (!isFiniteF64(sum_sq)) return false;

    const mean_sq = sum_sq / (x.len as f64);
    const denom = std.math.sqrt(mean_sq + eps);
    if (denom == 0.0 || !isFiniteF64(denom)) return false;
    const inv_rms = 1.0 / denom;

    let j: usize = 0;
    while (j < x.len) : (j += 1) {
        const w: f64 = if (weight.len == 0) 1.0 else weight[j];
        out[j] = x[j] * inv_rms * w;
    }
    return true;
}

/// Standard Layer Normalization with optional affine `gamma` (default 1.0) and `beta` (default 0.0).
/// `out[i] = ((x[i] - mean) / sqrt(var + eps)) * gamma[i] + beta[i]`.
pub fn layerNorm(
    x: []const f64,
    gamma: []const f64,
    beta: []const f64,
    eps: f64,
    out: *[]f64,
): bool {
    if (x.len == 0 || out.len < x.len) return false;
    if (gamma.len != 0 && gamma.len != x.len) return false;
    if (beta.len != 0 && beta.len != x.len) return false;
    if (!isFiniteF64(eps) || eps < 0.0) return false;

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const v = x[i];
        if (!isFiniteF64(v)) return false;
        if (gamma.len != 0 && !isFiniteF64(gamma[i])) return false;
        if (beta.len != 0 && !isFiniteF64(beta[i])) return false;
        sum += v;
    }
    const n_f = x.len as f64;
    const mean = sum / n_f;
    if (!isFiniteF64(mean)) return false;

    let var_sum: f64 = 0.0;
    let k: usize = 0;
    while (k < x.len) : (k += 1) {
        const diff = x[k] - mean;
        var_sum += diff * diff;
    }
    if (!isFiniteF64(var_sum)) return false;

    const variance = var_sum / n_f;
    const denom = std.math.sqrt(variance + eps);
    if (denom == 0.0 || !isFiniteF64(denom)) return false;
    const inv_std = 1.0 / denom;

    let j: usize = 0;
    while (j < x.len) : (j += 1) {
        const g: f64 = if (gamma.len == 0) 1.0 else gamma[j];
        const b: f64 = if (beta.len == 0) 0.0 else beta[j];
        out[j] = ((x[j] - mean) * inv_std) * g + b;
    }
    return true;
}

/// Sigmoid Linear Unit (SiLU / Swish-1): `x * sigmoid(x)` with overflow-safe branches.
pub fn silu(x: f64): f64 {
    if (x >= 0.0) {
        return x / (1.0 + std.math.exp(0.0 - x));
    }
    const ex = std.math.exp(x);
    return x * (ex / (1.0 + ex));
}

/// Hendrycks-Gimpel Gaussian Error Linear Unit (tanh approximation):
/// `0.5 * x * (1 + tanh(sqrt(2 / pi) * (x + 0.044715 * x^3)))`.
pub fn geluTanh(x: f64): f64 {
    const sqrt_2_over_pi: f64 = 0.7978845608028654;
    const inner = sqrt_2_over_pi * (x + 0.044715 * x * x * x);
    return 0.5 * x * (1.0 + std.math.tanh(inner));
}

/// SwiGLU elementwise FFN activation kernel: `out[i] = silu(gate[i]) * up[i]`.
/// Returns `false` on length mismatch, insufficient `out`, or NaN/Inf inputs.
pub fn swigluForward(gate: []const f64, up: []const f64, out: *[]f64): bool {
    if (gate.len != up.len || out.len < gate.len) return false;
    let i: usize = 0;
    while (i < gate.len) : (i += 1) {
        if (!isFiniteF64(gate[i]) || !isFiniteF64(up[i])) return false;
    }
    let j: usize = 0;
    while (j < gate.len) : (j += 1) {
        out[j] = silu(gate[j]) * up[j];
    }
    return true;
}
