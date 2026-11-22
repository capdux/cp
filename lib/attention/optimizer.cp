// Origin: Pure-CP Stdlib (AI Training Optimizers: AdamW, Lion, Cosine Warmup & GradClip) | Created: 2026-11-22
// Zero-heap-allocation LLM training optimization & learning-rate scheduling primitives:
//   - Learning-rate schedules: linear warmup + cosine annealing (`cosineWarmupLr`) & linear decay (`linearWarmupDecayLr`)
//   - Global L2 gradient norm (`gradGlobalL2Norm`) & in-place norm clipping (`clipGradNormInPlace`)
//   - Decoupled weight-decay AdamW (`adamWStepInPlace`) with exact bias correction `1 - beta^t`
//   - Google Brain Lion sign-momentum optimizer (`lionStepInPlace`, 50% optimizer-state memory)

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Learning-rate schedule configuration shared by `cosineWarmupLr` and `linearWarmupDecayLr`.
pub struct LrScheduleConfig {
    peak_lr: f64 = 0.001;
    min_lr: f64 = 0.0;
    warmup_steps: usize = 0;
    total_steps: usize = 1000;
}

/// Configuration for `adamWStepInPlace` (Loshchilov & Hutter, 2019).
pub struct AdamWConfig {
    beta1: f64 = 0.9;
    beta2: f64 = 0.999;
    eps: f64 = 0.00000001;
    weight_decay: f64 = 0.01;

    /// Returns standard LLM pretraining AdamW defaults (`beta1=0.9`, `beta2=0.999`, `eps=1e-8`, `wd=0.01`).
    static fn default(): AdamWConfig {
        return AdamWConfig{
            beta1: 0.9,
            beta2: 0.999,
            eps: 0.00000001,
            weight_decay: 0.01,
        };
    }
}

/// Configuration for `lionStepInPlace` (Chen et al., Google Brain 2023).
pub struct LionConfig {
    beta1: f64 = 0.9;
    beta2: f64 = 0.99;
    weight_decay: f64 = 0.01;

    /// Returns standard Lion defaults (`beta1=0.9`, `beta2=0.99`, `wd=0.01`).
    static fn default(): LionConfig {
        return LionConfig{
            beta1: 0.9,
            beta2: 0.99,
            weight_decay: 0.01,
        };
    }
}

fn isValidSchedule(cfg: LrScheduleConfig): bool {
    if (!isFiniteF64(cfg.peak_lr) || !isFiniteF64(cfg.min_lr)) return false;
    if (cfg.peak_lr < 0.0 || cfg.min_lr < 0.0 || cfg.min_lr > cfg.peak_lr) return false;
    if (cfg.total_steps == 0 || cfg.warmup_steps > cfg.total_steps) return false;
    return true;
}

/// Computes learning rate at `step` under linear warmup followed by cosine annealing to `min_lr`.
/// - `step < warmup_steps`: `peak_lr * (step / warmup_steps)`
/// - `warmup_steps <= step < total_steps`: `min_lr + 0.5 * (peak_lr - min_lr) * (1 + cos(pi * progress))`
/// - `step >= total_steps`: `min_lr`
/// Returns `null` on invalid schedule configuration.
pub fn cosineWarmupLr(step: usize, cfg: LrScheduleConfig): f64? {
    if (!isValidSchedule(cfg)) return null;

    if (cfg.warmup_steps > 0 && step < cfg.warmup_steps) {
        return cfg.peak_lr * ((step as f64) / (cfg.warmup_steps as f64));
    }
    if (step >= cfg.total_steps) {
        return cfg.min_lr;
    }

    const decay_steps = cfg.total_steps - cfg.warmup_steps;
    if (decay_steps == 0) return cfg.min_lr;

    const progress = ((step - cfg.warmup_steps) as f64) / (decay_steps as f64);
    const cosine = 0.5 * (1.0 + std.math.cos(std.math.pi * progress));
    return cfg.min_lr + (cfg.peak_lr - cfg.min_lr) * cosine;
}

/// Computes learning rate at `step` under linear warmup followed by linear decay to `min_lr`.
/// Returns `null` on invalid schedule configuration.
pub fn linearWarmupDecayLr(step: usize, cfg: LrScheduleConfig): f64? {
    if (!isValidSchedule(cfg)) return null;

    if (cfg.warmup_steps > 0 && step < cfg.warmup_steps) {
        return cfg.peak_lr * ((step as f64) / (cfg.warmup_steps as f64));
    }
    if (step >= cfg.total_steps) {
        return cfg.min_lr;
    }

    const decay_steps = cfg.total_steps - cfg.warmup_steps;
    if (decay_steps == 0) return cfg.min_lr;

    const progress = ((step - cfg.warmup_steps) as f64) / (decay_steps as f64);
    return cfg.min_lr + (cfg.peak_lr - cfg.min_lr) * (1.0 - progress);
}

/// Computes the global L2 norm `sqrt(sum_i grads[i]^2)` of a gradient slice.
/// Returns `0.0` for an empty slice, or `null` if any gradient element is NaN/Inf.
pub fn gradGlobalL2Norm(grads: []const f64): f64? {
    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < grads.len) : (i += 1) {
        const g = grads[i];
        if (!isFiniteF64(g)) return null;
        sum_sq += g * g;
    }
    const norm = std.math.sqrt(sum_sq);
    if (!isFiniteF64(norm)) return null;
    return norm;
}

/// Clips `grads` in-place so its global L2 norm does not exceed `max_norm`.
/// Returns the pre-clipping global L2 norm, or `null` if `max_norm <= 0` or any gradient is NaN/Inf.
pub fn clipGradNormInPlace(grads: *[]f64, max_norm: f64): f64? {
    if (!isFiniteF64(max_norm) || !(max_norm > 0.0)) return null;
    const norm = gradGlobalL2Norm(grads) ?? return null;

    if (norm > max_norm) {
        const scale = max_norm / norm;
        let i: usize = 0;
        while (i < grads.len) : (i += 1) {
            grads[i] *= scale;
        }
    }
    return norm;
}

/// Performs one in-place decoupled AdamW parameter update (1-based `step >= 1`):
/// - `params[i] -= lr * weight_decay * params[i]`
/// - `m[i] = beta1 * m[i] + (1 - beta1) * grads[i]`
/// - `v[i] = beta2 * v[i] + (1 - beta2) * grads[i]^2`
/// - `params[i] -= lr * (m[i] / (1 - beta1^step)) / (sqrt(v[i] / (1 - beta2^step)) + eps)`
///
/// Validation is atomic: returns `false` without mutating any buffer on invalid hyperparameters,
/// `step == 0`, length mismatch, negative `v[i]`, or NaN/Inf inputs.
pub fn adamWStepInPlace(
    params: *[]f64,
    grads: []const f64,
    m: *[]f64,
    v: *[]f64,
    step: usize,
    lr: f64,
    cfg: AdamWConfig,
): bool {
    const n = params.len;
    if (n == 0 || grads.len != n || m.len != n || v.len != n) return false;
    if (step == 0) return false;
    if (!isFiniteF64(lr) || lr < 0.0) return false;
    if (!isFiniteF64(cfg.beta1) || cfg.beta1 < 0.0 || cfg.beta1 >= 1.0) return false;
    if (!isFiniteF64(cfg.beta2) || cfg.beta2 < 0.0 || cfg.beta2 >= 1.0) return false;
    if (!isFiniteF64(cfg.eps) || !(cfg.eps > 0.0)) return false;
    if (!isFiniteF64(cfg.weight_decay) || cfg.weight_decay < 0.0) return false;

    const bc1 = 1.0 - std.math.pow(f64, cfg.beta1, step as f64);
    const bc2 = 1.0 - std.math.pow(f64, cfg.beta2, step as f64);
    if (!(bc1 > 0.0) || !(bc2 > 0.0) || !isFiniteF64(bc1) || !isFiniteF64(bc2)) return false;

    let chk: usize = 0;
    while (chk < n) : (chk += 1) {
        if (!isFiniteF64(params[chk]) || !isFiniteF64(grads[chk]) || !isFiniteF64(m[chk]) || !isFiniteF64(v[chk])) {
            return false;
        }
        if (v[chk] < 0.0) return false;
    }

    const one_m_b1 = 1.0 - cfg.beta1;
    const one_m_b2 = 1.0 - cfg.beta2;
    const wd_factor = lr * cfg.weight_decay;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const g = grads[i];
        const m_new = cfg.beta1 * m[i] + one_m_b1 * g;
        const v_new = cfg.beta2 * v[i] + one_m_b2 * g * g;
        m[i] = m_new;
        v[i] = v_new;

        const m_hat = m_new / bc1;
        const v_hat = v_new / bc2;
        const p_decayed = params[i] - wd_factor * params[i];
        params[i] = p_decayed - lr * (m_hat / (std.math.sqrt(v_hat) + cfg.eps));
    }

    return true;
}

/// Performs one in-place Lion (EvoLved Sign Momentum) parameter update:
/// - `c = beta1 * m[i] + (1 - beta1) * grads[i]`
/// - `params[i] -= lr * (sign(c) + weight_decay * params[i])`
/// - `m[i] = beta2 * m[i] + (1 - beta2) * grads[i]`
///
/// Requires only a single momentum buffer `m` (half the optimizer state memory of AdamW).
/// Validation is atomic: returns `false` without mutating state on invalid inputs.
pub fn lionStepInPlace(
    params: *[]f64,
    grads: []const f64,
    m: *[]f64,
    lr: f64,
    cfg: LionConfig,
): bool {
    const n = params.len;
    if (n == 0 || grads.len != n || m.len != n) return false;
    if (!isFiniteF64(lr) || lr < 0.0) return false;
    if (!isFiniteF64(cfg.beta1) || cfg.beta1 < 0.0 || cfg.beta1 >= 1.0) return false;
    if (!isFiniteF64(cfg.beta2) || cfg.beta2 < 0.0 || cfg.beta2 >= 1.0) return false;
    if (!isFiniteF64(cfg.weight_decay) || cfg.weight_decay < 0.0) return false;

    let chk: usize = 0;
    while (chk < n) : (chk += 1) {
        if (!isFiniteF64(params[chk]) || !isFiniteF64(grads[chk]) || !isFiniteF64(m[chk])) {
            return false;
        }
    }

    const one_m_b1 = 1.0 - cfg.beta1;
    const one_m_b2 = 1.0 - cfg.beta2;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const g = grads[i];
        const c = cfg.beta1 * m[i] + one_m_b1 * g;
        const sign_c: f64 = if (c > 0.0) 1.0 else if (c < 0.0) -1.0 else 0.0;
        params[i] -= lr * (sign_c + cfg.weight_decay * params[i]);
        m[i] = cfg.beta2 * m[i] + one_m_b2 * g;
    }

    return true;
}
