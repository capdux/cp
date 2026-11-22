// Origin: Pure-CP Stdlib (Zero-Alloc Support Vector Machine: Platt SMO Kernel SVM & Pegasos Linear SVM) | Created: 2026-11-22

import std from "std";

const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;

/// Kernel function type for dual SMO SVM.
pub enum KernelKind {
    Linear,
    Poly,
    Rbf,
}

/// Hyperparameters for SVM kernel evaluation.
pub struct KernelParams {
    kind: KernelKind,
    gamma: f64,
    coef0: f64,
    degree: u32,

    /// Linear kernel `K(a, b) = <a, b>`.
    static fn linear(): KernelParams {
        return KernelParams {
            kind: KernelKind.Linear,
            gamma: 1.0,
            coef0: 0.0,
            degree: 1,
        };
    }

    /// Radial Basis Function (Gaussian) kernel `K(a, b) = exp(-gamma * ||a - b||^2)`.
    static fn rbf(gamma: f64): KernelParams {
        return KernelParams {
            kind: KernelKind.Rbf,
            gamma: gamma,
            coef0: 0.0,
            degree: 1,
        };
    }

    /// Polynomial kernel `K(a, b) = (gamma * <a, b> + coef0)^degree`.
    static fn poly(gamma: f64, coef0: f64, degree: u32): KernelParams {
        return KernelParams {
            kind: KernelKind.Poly,
            gamma: gamma,
            coef0: coef0,
            degree: degree,
        };
    }
}

/// Configuration for Platt SMO solver.
pub struct SmoConfig {
    c: f64,
    tol: f64,
    eps: f64,
    max_passes: usize,
    max_iter: usize,

    static fn default(): SmoConfig {
        return SmoConfig {
            c: 1.0,
            tol: 1.0e-3,
            eps: 1.0e-5,
            max_passes: 20,
            max_iter: 2000,
        };
    }
}

/// Training summary returned by `svmFitSmo`.
pub struct SmoResult {
    bias: f64,
    support_vectors: usize,
    iterations: usize,
    converged: bool,
}

/// Training summary returned by `linearSvmFit`.
pub struct LinearSvmResult {
    bias: f64,
    iterations: usize,
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn checkMatrixLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn checkBinaryLabels(y: []const f64, n: usize): bool {
    let has_pos: bool = false;
    let has_neg: bool = false;
    let i: usize = 0;
    while (i < n) {
        const v: f64 = y[i];
        if (v == 1.0) {
            has_pos = true;
        } else if (v == -1.0) {
            has_neg = true;
        } else {
            return false;
        }
        i += 1;
    }
    return has_pos && has_neg;
}

fn checkKernelValid(kp: KernelParams): bool {
    match (kp.kind) {
        .Linear => { return true; },
        .Rbf => { return isFiniteF64(kp.gamma) && kp.gamma > 0.0; },
        .Poly => { return isFiniteF64(kp.gamma) && isFiniteF64(kp.coef0); },
    }
}

fn powU32(base: f64, exp: u32): f64 {
    let res: f64 = 1.0;
    let b: f64 = base;
    let e: u32 = exp;
    while (e > 0) {
        if ((e & 1) != 0) {
            res *= b;
        }
        b *= b;
        e = e >> 1;
    }
    return res;
}

fn splitmixMix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

/// Evaluates the configured kernel function `K(a, b)` over `min(a.len, b.len)` features.
pub fn evalKernel(a: []const f64, b: []const f64, kp: KernelParams): f64 {
    const d: usize = if (a.len < b.len) a.len else b.len;
    match (kp.kind) {
        .Linear => {
            let dot: f64 = 0.0;
            let i: usize = 0;
            while (i < d) {
                dot += a[i] * b[i];
                i += 1;
            }
            return dot;
        },
        .Poly => {
            let dot: f64 = 0.0;
            let i: usize = 0;
            while (i < d) {
                dot += a[i] * b[i];
                i += 1;
            }
            return powU32(kp.gamma * dot + kp.coef0, kp.degree);
        },
        .Rbf => {
            let sq: f64 = 0.0;
            let i: usize = 0;
            while (i < d) {
                const diff: f64 = a[i] - b[i];
                sq += diff * diff;
                i += 1;
            }
            return std.math.exp((0.0 - kp.gamma) * sq);
        },
    }
}

fn sampleRow(x: []const f64, idx: usize, n_features: usize): []const f64 {
    const base: usize = idx * n_features;
    return x[base..(base + n_features)];
}

struct SmoState {
    bias: f64,
    iters: usize,
}

fn smoTakeStep(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    kp: KernelParams,
    cfg: SmoConfig,
    alphas: *[]f64,
    errors: *[]f64,
    st: *SmoState,
    idx1: usize,
    idx2: usize,
): bool {
    if (idx1 == idx2) {
        return false;
    }
    const a1: f64 = alphas.*[idx1];
    const a2: f64 = alphas.*[idx2];
    const y1: f64 = y[idx1];
    const y2: f64 = y[idx2];
    const e1: f64 = errors.*[idx1];
    const e2: f64 = errors.*[idx2];
    const s: f64 = y1 * y2;

    let lo: f64 = 0.0;
    let hi: f64 = cfg.c;
    if (y1 != y2) {
        const diff: f64 = a2 - a1;
        lo = if (diff > 0.0) diff else 0.0;
        const c_diff: f64 = cfg.c + diff;
        hi = if (c_diff < cfg.c) c_diff else cfg.c;
    } else {
        const sum: f64 = a1 + a2;
        const sum_m_c: f64 = sum - cfg.c;
        lo = if (sum_m_c > 0.0) sum_m_c else 0.0;
        hi = if (sum < cfg.c) sum else cfg.c;
    }
    if (lo >= hi) {
        return false;
    }

    const r1: []const f64 = sampleRow(x, idx1, n_features);
    const r2: []const f64 = sampleRow(x, idx2, n_features);
    const k11: f64 = evalKernel(r1, r1, kp);
    const k12: f64 = evalKernel(r1, r2, kp);
    const k22: f64 = evalKernel(r2, r2, kp);
    const eta: f64 = 2.0 * k12 - k11 - k22;
    if (eta >= -1.0e-15) {
        return false;
    }

    let a2_new: f64 = a2 - (y2 * (e1 - e2)) / eta;
    if (a2_new > hi) {
        a2_new = hi;
    } else if (a2_new < lo) {
        a2_new = lo;
    }

    if (absF64(a2_new - a2) < cfg.eps * (a2_new + a2 + cfg.eps)) {
        return false;
    }

    let a1_new: f64 = a1 + s * (a2 - a2_new);
    if (a1_new < 0.0 && a1_new > -1.0e-12) {
        a1_new = 0.0;
    } else if (a1_new > cfg.c && a1_new < cfg.c + 1.0e-12) {
        a1_new = cfg.c;
    }

    const b_old: f64 = st.*.bias;
    const b1: f64 = b_old - e1 - y1 * (a1_new - a1) * k11 - y2 * (a2_new - a2) * k12;
    const b2: f64 = b_old - e2 - y1 * (a1_new - a1) * k12 - y2 * (a2_new - a2) * k22;
    let b_new: f64 = 0.5 * (b1 + b2);
    if (a1_new > 0.0 && a1_new < cfg.c) {
        b_new = b1;
    } else if (a2_new > 0.0 && a2_new < cfg.c) {
        b_new = b2;
    }

    alphas.*[idx1] = a1_new;
    alphas.*[idx2] = a2_new;
    st.*.bias = b_new;
    st.*.iters += 1;

    // Incrementally update error cache E_k = f(x_k) - y_k
    const da1: f64 = y1 * (a1_new - a1);
    const da2: f64 = y2 * (a2_new - a2);
    const db: f64 = b_new - b_old;
    let k: usize = 0;
    while (k < n_samples) {
        const rk: []const f64 = sampleRow(x, k, n_features);
        errors.*[k] += da1 * evalKernel(r1, rk, kp) + da2 * evalKernel(r2, rk, kp) + db;
        k += 1;
    }

    return true;
}

fn smoExamineExample(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    kp: KernelParams,
    cfg: SmoConfig,
    alphas: *[]f64,
    errors: *[]f64,
    st: *SmoState,
    idx2: usize,
): bool {
    const y2: f64 = y[idx2];
    const a2: f64 = alphas.*[idx2];
    const e2: f64 = errors.*[idx2];
    const r2: f64 = e2 * y2;

    if (!((r2 < (0.0 - cfg.tol) && a2 < cfg.c) || (r2 > cfg.tol && a2 > 0.0))) {
        return false;
    }

    // Second-choice heuristic: pick non-bound idx1 maximizing |E_1 - E_2|
    let best_i1: usize = n_samples;
    let max_delta: f64 = -1.0;
    let non_bound_cnt: usize = 0;
    let i: usize = 0;
    while (i < n_samples) {
        if (alphas.*[i] > 0.0 && alphas.*[i] < cfg.c) {
            non_bound_cnt += 1;
            if (i != idx2) {
                const delta: f64 = absF64(errors.*[i] - e2);
                if (delta > max_delta) {
                    max_delta = delta;
                    best_i1 = i;
                }
            }
        }
        i += 1;
    }
    if (best_i1 < n_samples) {
        if (smoTakeStep(x, y, n_samples, n_features, kp, cfg, alphas, errors, st, best_i1, idx2)) {
            return true;
        }
    }

    // Fallback 1: loop over non-bound samples starting from a deterministic offset
    if (non_bound_cnt > 0) {
        const start: usize = (idx2 + st.*.iters + 1) % n_samples;
        let step: usize = 0;
        while (step < n_samples) {
            const cand: usize = (start + step) % n_samples;
            if (alphas.*[cand] > 0.0 && alphas.*[cand] < cfg.c) {
                if (smoTakeStep(x, y, n_samples, n_features, kp, cfg, alphas, errors, st, cand, idx2)) {
                    return true;
                }
            }
            step += 1;
        }
    }

    // Fallback 2: loop over all samples
    const start_all: usize = (idx2 + st.*.iters + 1) % n_samples;
    let step2: usize = 0;
    while (step2 < n_samples) {
        const cand: usize = (start_all + step2) % n_samples;
        if (smoTakeStep(x, y, n_samples, n_features, kp, cfg, alphas, errors, st, cand, idx2)) {
            return true;
        }
        step2 += 1;
    }

    return false;
}

/// Trains a binary kernel SVM using Platt's Sequential Minimal Optimization (SMO).
/// - `x`: row-major `n_samples x n_features` training features
/// - `y`: binary labels strictly in `{-1.0, +1.0}` containing both classes
/// - `alphas_out`: caller buffer of length `>= n_samples` (receives Lagrange multipliers)
/// - `errors_scratch`: caller scratch buffer of length `>= n_samples`
/// Returns `null` on invalid inputs, single-class labels, NaN/Inf, or insufficient buffers.
pub fn svmFitSmo(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    kp: KernelParams,
    cfg: SmoConfig,
    alphas_out: *[]f64,
    errors_scratch: *[]f64,
): SmoResult? {
    if (n_samples < 2 || n_features == 0) {
        return null;
    }
    if (!checkMatrixLen(x.len, n_samples, n_features) || y.len < n_samples) {
        return null;
    }
    if (alphas_out.*.len < n_samples || errors_scratch.*.len < n_samples) {
        return null;
    }
    if (!isFiniteF64(cfg.c) || cfg.c <= 0.0 || !isFiniteF64(cfg.tol) || cfg.tol < 0.0
        || !isFiniteF64(cfg.eps) || cfg.eps <= 0.0 || cfg.max_passes == 0 || cfg.max_iter == 0) {
        return null;
    }
    if (!checkKernelValid(kp) || !checkFinitePrefix(x, n_samples * n_features)) {
        return null;
    }
    if (!checkBinaryLabels(y, n_samples)) {
        return null;
    }

    // Initialize alphas = 0, errors = -y
    let z: usize = 0;
    while (z < n_samples) {
        alphas_out.*[z] = 0.0;
        errors_scratch.*[z] = 0.0 - y[z];
        z += 1;
    }

    let st: SmoState = SmoState { bias: 0.0, iters: 0 };
    let examine_all: bool = true;
    let num_changed: usize = 0;
    let passes: usize = 0;
    let converged: bool = false;

    while ((num_changed > 0 || examine_all) && st.iters < cfg.max_iter) {
        num_changed = 0;
        if (examine_all) {
            let i: usize = 0;
            while (i < n_samples && st.iters < cfg.max_iter) {
                if (smoExamineExample(x, y, n_samples, n_features, kp, cfg, alphas_out, errors_scratch, *st, i)) {
                    num_changed += 1;
                }
                i += 1;
            }
        } else {
            let i: usize = 0;
            while (i < n_samples && st.iters < cfg.max_iter) {
                if (alphas_out.*[i] > 0.0 && alphas_out.*[i] < cfg.c) {
                    if (smoExamineExample(x, y, n_samples, n_features, kp, cfg, alphas_out, errors_scratch, *st, i)) {
                        num_changed += 1;
                    }
                }
                i += 1;
            }
        }

        if (examine_all) {
            examine_all = false;
            if (num_changed == 0) {
                converged = true;
                break;
            }
        } else if (num_changed == 0) {
            examine_all = true;
            passes += 1;
            if (passes >= cfg.max_passes) {
                break;
            }
        }
    }

    let sv_count: usize = 0;
    let sv_i: usize = 0;
    while (sv_i < n_samples) {
        if (alphas_out.*[sv_i] > 1.0e-8) {
            sv_count += 1;
        }
        sv_i += 1;
    }

    return SmoResult {
        bias: st.bias,
        support_vectors: sv_count,
        iterations: st.iters,
        converged: converged,
    };
}

/// Evaluates the dual kernel SVM decision function `f(query) = sum_{alpha_i > 0} alpha_i y_i K(x_i, query) + bias`.
pub fn svmDecision(
    x_train: []const f64,
    y_train: []const f64,
    alphas: []const f64,
    bias: f64,
    n_train: usize,
    n_features: usize,
    kp: KernelParams,
    query: []const f64,
): f64? {
    if (n_train == 0 || n_features == 0 || !isFiniteF64(bias) || !checkKernelValid(kp)) {
        return null;
    }
    if (!checkMatrixLen(x_train.len, n_train, n_features) || y_train.len < n_train || alphas.len < n_train) {
        return null;
    }
    if (query.len < n_features || !checkFinitePrefix(query, n_features)) {
        return null;
    }

    const q_slice: []const f64 = query[0..n_features];
    let sum: f64 = bias;
    let i: usize = 0;
    while (i < n_train) {
        const ai: f64 = alphas[i];
        if (ai > 0.0) {
            sum += ai * y_train[i] * evalKernel(sampleRow(x_train, i, n_features), q_slice, kp);
        }
        i += 1;
    }
    if (!isFiniteF64(sum)) {
        return null;
    }
    return sum;
}

/// Predicts binary class label `+1.0` or `-1.0` using the dual kernel SVM decision function.
pub fn svmPredict(
    x_train: []const f64,
    y_train: []const f64,
    alphas: []const f64,
    bias: f64,
    n_train: usize,
    n_features: usize,
    kp: KernelParams,
    query: []const f64,
): f64? {
    const score: f64 = svmDecision(x_train, y_train, alphas, bias, n_train, n_features, kp, query) ?? return null;
    return if (score >= 0.0) 1.0 else -1.0;
}

/// Trains a primal linear SVM via Pegasos stochastic subgradient descent (zero kernel matrix overhead).
/// Minimizes `0.5 * lambda * ||w||^2 + (1/n) * sum max(0, 1 - y_i * (w^T x_i + b))`.
pub fn linearSvmFit(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    lambda: f64,
    epochs: usize,
    seed: u64,
    w_out: *[]f64,
): LinearSvmResult? {
    if (n_samples < 2 || n_features == 0 || epochs == 0) {
        return null;
    }
    if (!isFiniteF64(lambda) || lambda <= 0.0) {
        return null;
    }
    if (!checkMatrixLen(x.len, n_samples, n_features) || y.len < n_samples || w_out.*.len < n_features) {
        return null;
    }
    if (!checkFinitePrefix(x, n_samples * n_features) || !checkBinaryLabels(y, n_samples)) {
        return null;
    }

    let z: usize = 0;
    while (z < n_features) {
        w_out.*[z] = 0.0;
        z += 1;
    }

    let bias: f64 = 0.0;
    let rng: u64 = seed;
    let step: usize = 0;
    let ep: usize = 0;
    while (ep < epochs) {
        let s: usize = 0;
        while (s < n_samples) {
            rng = rng +% SPLITMIX_INC;
            const idx: usize = (splitmixMix(rng) % (n_samples as u64)) as usize;
            step += 1;
            const eta: f64 = 1.0 / (lambda * (step as f64));

            const row_base: usize = idx * n_features;
            let dot: f64 = bias;
            let f: usize = 0;
            while (f < n_features) {
                dot += w_out.*[f] * x[row_base + f];
                f += 1;
            }
            const margin: f64 = y[idx] * dot;

            let decay: f64 = 1.0 - eta * lambda;
            if (decay < 0.0) {
                decay = 0.0;
            }
            let f_u: usize = 0;
            while (f_u < n_features) {
                w_out.*[f_u] *= decay;
                f_u += 1;
            }
            if (margin < 1.0) {
                const coef: f64 = eta * y[idx];
                let f_a: usize = 0;
                while (f_a < n_features) {
                    w_out.*[f_a] += coef * x[row_base + f_a];
                    f_a += 1;
                }
                bias += coef;
            }
            s += 1;
        }
        ep += 1;
    }

    return LinearSvmResult {
        bias: bias,
        iterations: step,
    };
}

/// Evaluates linear SVM decision score `w^T * query + bias`.
pub fn linearSvmDecision(w: []const f64, bias: f64, query: []const f64): f64? {
    if (w.len == 0 || query.len < w.len || !isFiniteF64(bias)) {
        return null;
    }
    if (!checkFinitePrefix(w, w.len) || !checkFinitePrefix(query, w.len)) {
        return null;
    }
    let sum: f64 = bias;
    let i: usize = 0;
    while (i < w.len) {
        sum += w[i] * query[i];
        i += 1;
    }
    if (!isFiniteF64(sum)) {
        return null;
    }
    return sum;
}
