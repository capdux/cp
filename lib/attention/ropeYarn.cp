// Origin: Pure-CP Stdlib (AI Long-Context RoPE Extension: Linear PI, NTK-Aware & YaRN) | Created: 2026-11-22
// Pure CP zero-heap-allocation long-context RoPE frequency scaling engine supporting standard
// RoPE, Linear Position Interpolation (PI), NTK-Aware scaling, and YaRN (Peng et al. 2023)
// wavelength-band ramp interpolation with attention temperature scaling (`mscale`).

const std = @import("std");

const TWO_PI: f64 = 6.283185307179586476925286766559;

/// Long-context RoPE frequency scaling strategy.
pub enum RopeScalingKind {
    None,
    Linear,
    NtkAware,
    Yarn,
}

/// Configuration parameters for Linear PI / NTK-Aware / YaRN RoPE scaling.
pub struct YarnConfig {
    head_dim: usize = 64;
    rope_theta: f64 = 10000.0;
    orig_max_pos: usize = 4096;
    scale_factor: f64 = 1.0;
    beta_fast: f64 = 32.0;
    beta_slow: f64 = 1.0;
    mscale_all_dim: f64 = 0.0;

    /// Constructs a default YaRN configuration with standard `rope_theta = 10000.0`,
    /// `beta_fast = 32.0`, `beta_slow = 1.0`, and `mscale_all_dim = 0.0`.
    static fn default(head_dim: usize, orig_max_pos: usize, scale_factor: f64): YarnConfig {
        return YarnConfig{
            head_dim: head_dim,
            rope_theta: 10000.0,
            orig_max_pos: orig_max_pos,
            scale_factor: scale_factor,
            beta_fast: 32.0,
            beta_slow: 1.0,
            mscale_all_dim: 0.0,
        };
    }
}

// ============================================================================
// Internal Numeric Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// Public YaRN / Long-Context RoPE API
// ============================================================================

/// Computes the YaRN attention temperature scaling factor `mscale = 0.1 * mscale_coeff * ln(scale_factor) + 1.0`.
/// Returns `1.0` when `scale_factor <= 1.0`, `mscale_coeff <= 0.0`, or either input is non-finite.
pub fn yarnAttentionMscale(scale_factor: f64, mscale_coeff: f64): f64 {
    if (!isFiniteF64(scale_factor) || !isFiniteF64(mscale_coeff)) return 1.0;
    if (scale_factor <= 1.0 || mscale_coeff <= 0.0) return 1.0;
    return 0.1 * mscale_coeff * std.math.log(f64, std.math.e, scale_factor) + 1.0;
}

/// Computes `head_dim / 2` inverse RoPE frequencies into `out_inv_freqs` according to `kind`:
///
/// - `RopeScalingKind.None`: standard RoPE `theta^(-2i / d)`
/// - `RopeScalingKind.Linear`: Linear Position Interpolation `theta^(-2i / d) / scale_factor`
/// - `RopeScalingKind.NtkAware`: NTK-Aware base scaling `base' = theta * s^(d / (d - 2))`
/// - `RopeScalingKind.Yarn`: NTK-by-parts wavelength ramp (high-freq untouched, low-freq linearly
///   interpolated, mid-band linearly blended via `[beta_slow, beta_fast]`)
///
/// Returns `false` on invalid `head_dim` (0 or odd), undersized `out_inv_freqs`, invalid `rope_theta <= 1.0`,
/// `scale_factor < 1.0`, or invalid YaRN band parameters.
pub fn computeRopeInvFreqs(
    kind: RopeScalingKind,
    cfg: YarnConfig,
    out_inv_freqs: *[]f64,
): bool {
    if (cfg.head_dim == 0 || (cfg.head_dim & 1) != 0) return false;
    const half_dim: usize = cfg.head_dim >> 1;
    if (out_inv_freqs.len < half_dim) return false;
    if (!isFiniteF64(cfg.rope_theta) || cfg.rope_theta <= 1.0) return false;
    if (!isFiniteF64(cfg.scale_factor) || cfg.scale_factor < 1.0) return false;

    if (kind == RopeScalingKind.Yarn) {
        if (cfg.orig_max_pos == 0) return false;
        if (!isFiniteF64(cfg.beta_fast) || !isFiniteF64(cfg.beta_slow)) return false;
        if (cfg.beta_slow <= 0.0 || cfg.beta_fast <= cfg.beta_slow) return false;
    }

    const d_f64: f64 = cfg.head_dim as f64;
    let ntk_base: f64 = cfg.rope_theta;
    if (kind == RopeScalingKind.NtkAware && cfg.scale_factor > 1.0 && cfg.head_dim > 2) {
        const exp_ntk: f64 = d_f64 / ((cfg.head_dim - 2) as f64);
        ntk_base = cfg.rope_theta * std.math.pow(f64, cfg.scale_factor, exp_ntk);
    }

    const orig_max_f64: f64 = cfg.orig_max_pos as f64;
    const beta_span: f64 = cfg.beta_fast - cfg.beta_slow;

    let i: usize = 0;
    while (i < half_dim) : (i += 1) {
        const exp_i: f64 = ((i << 1) as f64) / d_f64;
        const base_freq: f64 = 1.0 / std.math.pow(f64, cfg.rope_theta, exp_i);

        if (kind == RopeScalingKind.None || cfg.scale_factor == 1.0) {
            out_inv_freqs[i] = base_freq;
        } else if (kind == RopeScalingKind.Linear) {
            out_inv_freqs[i] = base_freq / cfg.scale_factor;
        } else if (kind == RopeScalingKind.NtkAware) {
            if (cfg.head_dim == 2) {
                out_inv_freqs[i] = base_freq / cfg.scale_factor;
            } else {
                out_inv_freqs[i] = 1.0 / std.math.pow(f64, ntk_base, exp_i);
            }
        } else {
            // YaRN: NTK-by-parts wavelength-band ramp
            const freq_interp: f64 = base_freq / cfg.scale_factor;
            const n_rotations: f64 = (orig_max_f64 * base_freq) / TWO_PI;
            let gamma: f64 = 0.0;
            if (n_rotations >= cfg.beta_fast) {
                gamma = 1.0;
            } else if (n_rotations > cfg.beta_slow) {
                gamma = (n_rotations - cfg.beta_slow) / beta_span;
            }
            out_inv_freqs[i] = (1.0 - gamma) * freq_interp + gamma * base_freq;
        }
    }

    return true;
}

/// Applies half-split rotary positional embedding with `mscale` magnitude scaling to `vec` in-place
/// at sequence position `pos` using precomputed `inv_freqs[0 .. vec.len / 2]`.
///
/// Strictly preserves scaled L2 norm: `||vec_out||_2 == mscale * ||vec_in||_2`.
/// Returns `false` on empty/odd `vec`, undersized `inv_freqs`, invalid `mscale <= 0.0`, or non-finite values.
pub fn applyScaledRopeInPlaceF32(
    vec: *[]f32,
    pos: usize,
    inv_freqs: []const f64,
    mscale: f32,
): bool {
    if (vec.len == 0 || (vec.len & 1) != 0) return false;
    const half_dim: usize = vec.len >> 1;
    if (inv_freqs.len < half_dim) return false;
    if (!isFiniteF32(mscale) || mscale <= 0.0) return false;

    let check: usize = 0;
    while (check < vec.len) : (check += 1) {
        if (!isFiniteF32(vec[check])) return false;
    }
    let fcheck: usize = 0;
    while (fcheck < half_dim) : (fcheck += 1) {
        if (!isFiniteF64(inv_freqs[fcheck])) return false;
    }

    const pos_f64: f64 = pos as f64;
    let i: usize = 0;
    while (i < half_dim) : (i += 1) {
        const angle: f64 = pos_f64 * inv_freqs[i];
        const c: f32 = (@cos(angle) as f32) * mscale;
        const s: f32 = (@sin(angle) as f32) * mscale;
        const x0: f32 = vec[i];
        const x1: f32 = vec[i + half_dim];
        vec[i] = x0 * c - x1 * s;
        vec[i + half_dim] = x0 * s + x1 * c;
    }

    return true;
}
