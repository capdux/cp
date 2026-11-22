// Origin: Pure-CP Stdlib (Zero-Alloc Binary & Multinomial L2 Logistic Regression) | Created: 2026-11-22

const PROB_EPS: f64 = 1.0e-15;
const ARMIJO_C1: f64 = 1.0e-4;
const MAX_BACKTRACK: usize = 32;
const EXP_SAFE_BOUND: f64 = 709.0;
const SOFTPLUS_BOUND: f64 = 37.0;

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn softplus(z: f64): f64 {
    if (z > SOFTPLUS_BOUND) {
        return z;
    }
    if (z < 0.0 - SOFTPLUS_BOUND) {
        return @exp(z);
    }
    if (z >= 0.0) {
        return z + std.math.log1p(@exp(0.0 - z));
    }
    return std.math.log1p(@exp(z));
}

fn validateConfig(cfg: LogRegConfig): bool {
    if (!isFinite(cfg.learning_rate) || cfg.learning_rate <= 0.0) {
        return false;
    }
    if (!isFinite(cfg.l2_reg) || cfg.l2_reg < 0.0) {
        return false;
    }
    if (!isFinite(cfg.tol) || cfg.tol < 0.0) {
        return false;
    }
    if (cfg.max_iter == 0) {
        return false;
    }
    return true;
}

/// Hyperparameters for deterministic L2-regularized logistic regression.
pub struct LogRegConfig {
    learning_rate: f64,
    l2_reg: f64,
    max_iter: usize,
    tol: f64,

    fn default(): LogRegConfig {
        return {
            learning_rate: 0.5,
            l2_reg: 1.0e-4,
            max_iter: 1000,
            tol: 1.0e-6,
        };
    }
}

/// Convergence summary returned by `fitBinary`.
pub struct LogRegResult {
    bias: f64,
    loss: f64,
    grad_norm: f64,
    iterations: usize,
    converged: bool,
}

/// Convergence summary returned by `fitMultinomial`.
pub struct MultinomialResult {
    loss: f64,
    grad_norm: f64,
    iterations: usize,
    converged: bool,
}

/// Binary classification confusion matrix and derived metrics.
pub struct ClassificationMetrics {
    tp: usize,
    fp: usize,
    tn: usize,
    fn_count: usize,
    accuracy: f64,
    precision: f64,
    recall: f64,
    f1: f64,
}

/// Numerically stable logistic sigmoid `1 / (1 + exp(-z))` without overflow for extreme logits.
pub fn sigmoid(z: f64): f64 {
    if (std.math.isNan(z)) {
        return z;
    }
    if (z >= 0.0) {
        if (z >= EXP_SAFE_BOUND) {
            return 1.0;
        }
        const e = @exp(0.0 - z);
        return 1.0 / (1.0 + e);
    }
    if (z <= 0.0 - EXP_SAFE_BOUND) {
        return 0.0;
    }
    const e = @exp(z);
    return e / (1.0 + e);
}

/// Mean binary cross-entropy loss with probability clipping `[1e-15, 1 - 1e-15]`.
/// Returns `null` on empty/mismatched slices, out-of-range probabilities, invalid labels (`y ∉ {0, 1}`), or non-finite inputs.
pub fn logLossBinary(probs: []const f64, y: []const f64): f64? {
    const n = probs.len;
    if (n == 0 || n != y.len) {
        return null;
    }
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const p_raw = probs[i];
        const yi = y[i];
        if (!isFinite(p_raw) || p_raw < 0.0 || p_raw > 1.0) {
            return null;
        }
        if (yi != 0.0 && yi != 1.0) {
            return null;
        }
        let p = p_raw;
        if (p < PROB_EPS) {
            p = PROB_EPS;
        } else if (p > 1.0 - PROB_EPS) {
            p = 1.0 - PROB_EPS;
        }
        if (yi == 1.0) {
            total -= @log(p);
        } else {
            total -= @log(1.0 - p);
        }
        i += 1;
    }
    return total / (n as f64);
}

/// Numerically stable max-shifted softmax writing normalized probabilities into `probs_out[0..logits.len]`.
/// Returns `false` on empty input, insufficient output buffer, or non-finite logits.
pub fn softmax(logits: []const f64, probs_out: *[]f64): bool {
    const n = logits.len;
    if (n == 0 || probs_out.*.len < n) {
        return false;
    }
    let max_v = logits[0];
    if (!isFinite(max_v)) {
        return false;
    }
    let i: usize = 1;
    while (i < n) {
        const v = logits[i];
        if (!isFinite(v)) {
            return false;
        }
        if (v > max_v) {
            max_v = v;
        }
        i += 1;
    }

    let sum_e: f64 = 0.0;
    let j: usize = 0;
    while (j < n) {
        const e = @exp(logits[j] - max_v);
        probs_out.*[j] = e;
        sum_e += e;
        j += 1;
    }
    if (!isFinite(sum_e) || sum_e <= 0.0) {
        return false;
    }

    let k: usize = 0;
    while (k < n) {
        probs_out.*[k] = probs_out.*[k] / sum_e;
        k += 1;
    }
    return true;
}

fn evalBinaryLossTrial(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    w: []const f64,
    bias: f64,
    grad_w: []const f64,
    grad_b: f64,
    step: f64,
    l2_reg: f64,
): f64 {
    const b_cand = bias - step * grad_b;
    let loss_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n_samples) {
        const row_off = i * n_features;
        let z = b_cand;
        let j: usize = 0;
        while (j < n_features) {
            const wj = w[j] - step * grad_w[j];
            z += wj * x[row_off + j];
            j += 1;
        }
        loss_sum += softplus(z) - y[i] * z;
        i += 1;
    }
    let reg_sum: f64 = 0.0;
    let f: usize = 0;
    while (f < n_features) {
        const wj = w[f] - step * grad_w[f];
        reg_sum += wj * wj;
        f += 1;
    }
    return (loss_sum / (n_samples as f64)) + 0.5 * l2_reg * reg_sum;
}

/// Zero-allocation deterministic L2-regularized binary logistic regression with Armijo backtracking line search.
/// Feature matrix `x` is row-major (`n_samples * n_features`), labels `y[i] ∈ {0.0, 1.0}`.
/// Learned weights are written into `w_out[0..n_features]`; `grad_scratch` requires length `>= n_features`.
pub fn fitBinary(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    cfg: LogRegConfig,
    w_out: *[]f64,
    grad_scratch: *[]f64,
): LogRegResult? {
    if (n_samples == 0 || n_features == 0) {
        return null;
    }
    if (!validateConfig(cfg)) {
        return null;
    }
    if (x.len / n_samples < n_features || y.len < n_samples) {
        return null;
    }
    if (w_out.*.len < n_features || grad_scratch.*.len < n_features) {
        return null;
    }

    const total_x = n_samples * n_features;
    let xi: usize = 0;
    while (xi < total_x) {
        if (!isFinite(x[xi])) {
            return null;
        }
        xi += 1;
    }
    let yi_idx: usize = 0;
    while (yi_idx < n_samples) {
        const yv = y[yi_idx];
        if (yv != 0.0 && yv != 1.0) {
            return null;
        }
        yi_idx += 1;
    }

    // Initialize weights and bias to zero
    let init_j: usize = 0;
    while (init_j < n_features) {
        w_out.*[init_j] = 0.0;
        grad_scratch.*[init_j] = 0.0;
        init_j += 1;
    }

    const inv_n = 1.0 / (n_samples as f64);
    const max_step = if (cfg.learning_rate * 16.0 > 10.0) cfg.learning_rate * 16.0 else 10.0;
    let bias: f64 = 0.0;
    let step: f64 = cfg.learning_rate;
    let cur_loss: f64 = 0.0;
    let cur_grad_norm: f64 = 0.0;
    let iters: usize = 0;
    let converged = false;

    while (iters < cfg.max_iter) {
        let j_zero: usize = 0;
        while (j_zero < n_features) {
            grad_scratch.*[j_zero] = 0.0;
            j_zero += 1;
        }

        let loss_sum: f64 = 0.0;
        let grad_b: f64 = 0.0;
        let i: usize = 0;
        while (i < n_samples) {
            const row_off = i * n_features;
            let z = bias;
            let j: usize = 0;
            while (j < n_features) {
                z += w_out.*[j] * x[row_off + j];
                j += 1;
            }
            const yi = y[i];
            loss_sum += softplus(z) - yi * z;
            const diff = sigmoid(z) - yi;
            grad_b += diff;
            let k: usize = 0;
            while (k < n_features) {
                grad_scratch.*[k] += diff * x[row_off + k];
                k += 1;
            }
            i += 1;
        }

        grad_b = grad_b * inv_n;
        let reg_sum: f64 = 0.0;
        let g2: f64 = grad_b * grad_b;
        let f: usize = 0;
        while (f < n_features) {
            const wf = w_out.*[f];
            reg_sum += wf * wf;
            const gf = grad_scratch.*[f] * inv_n + cfg.l2_reg * wf;
            grad_scratch.*[f] = gf;
            g2 += gf * gf;
            f += 1;
        }

        cur_loss = loss_sum * inv_n + 0.5 * cfg.l2_reg * reg_sum;
        cur_grad_norm = @sqrt(g2);

        if (cur_grad_norm <= cfg.tol) {
            converged = true;
            break;
        }

        // Armijo backtracking line search (zero extra buffer)
        let trial = step;
        let accepted = false;
        let bt: usize = 0;
        let next_loss = cur_loss;
        while (bt < MAX_BACKTRACK) {
            const cand_loss = evalBinaryLossTrial(
                x,
                y,
                n_samples,
                n_features,
                w_out.*,
                bias,
                grad_scratch.*,
                grad_b,
                trial,
                cfg.l2_reg,
            );
            if (isFinite(cand_loss) && cand_loss <= cur_loss - ARMIJO_C1 * trial * g2) {
                accepted = true;
                next_loss = cand_loss;
                break;
            }
            trial *= 0.5;
            bt += 1;
        }

        if (!accepted) {
            converged = true;
            break;
        }

        let u: usize = 0;
        while (u < n_features) {
            w_out.*[u] -= trial * grad_scratch.*[u];
            u += 1;
        }
        bias -= trial * grad_b;
        cur_loss = next_loss;
        iters += 1;

        const grown = trial * 1.5;
        step = if (grown < max_step) grown else max_step;
    }

    // Recompute exact final gradient norm if loop ended on max_iter
    if (!converged) {
        let j_zero: usize = 0;
        while (j_zero < n_features) {
            grad_scratch.*[j_zero] = 0.0;
            j_zero += 1;
        }
        let loss_sum: f64 = 0.0;
        let grad_b: f64 = 0.0;
        let i: usize = 0;
        while (i < n_samples) {
            const row_off = i * n_features;
            let z = bias;
            let j: usize = 0;
            while (j < n_features) {
                z += w_out.*[j] * x[row_off + j];
                j += 1;
            }
            const yi = y[i];
            loss_sum += softplus(z) - yi * z;
            const diff = sigmoid(z) - yi;
            grad_b += diff;
            let k: usize = 0;
            while (k < n_features) {
                grad_scratch.*[k] += diff * x[row_off + k];
                k += 1;
            }
            i += 1;
        }
        grad_b = grad_b * inv_n;
        let reg_sum: f64 = 0.0;
        let g2: f64 = grad_b * grad_b;
        let f: usize = 0;
        while (f < n_features) {
            const wf = w_out.*[f];
            reg_sum += wf * wf;
            const gf = grad_scratch.*[f] * inv_n + cfg.l2_reg * wf;
            grad_scratch.*[f] = gf;
            g2 += gf * gf;
            f += 1;
        }
        cur_loss = loss_sum * inv_n + 0.5 * cfg.l2_reg * reg_sum;
        cur_grad_norm = @sqrt(g2);
        if (cur_grad_norm <= cfg.tol) {
            converged = true;
        }
    }

    return {
        bias: bias,
        loss: cur_loss,
        grad_norm: cur_grad_norm,
        iterations: iters,
        converged: converged,
    };
}

/// Predicts binary positive-class probability `sigmoid(w · sample + bias)`.
/// Returns `null` on empty/mismatched dimensions or non-finite inputs.
pub fn predictProbaBinary(w: []const f64, bias: f64, sample: []const f64): f64? {
    const n = w.len;
    if (n == 0 || n != sample.len || !isFinite(bias)) {
        return null;
    }
    let z = bias;
    let i: usize = 0;
    while (i < n) {
        const wi = w[i];
        const si = sample[i];
        if (!isFinite(wi) || !isFinite(si)) {
            return null;
        }
        z += wi * si;
        i += 1;
    }
    if (!isFinite(z)) {
        return null;
    }
    return sigmoid(z);
}

/// Predicts binary class label (`1` if `proba >= threshold` else `0`).
/// Returns `null` on invalid threshold (`∉ [0, 1]`), dimension mismatch, or non-finite inputs.
pub fn predictBinary(w: []const f64, bias: f64, sample: []const f64, threshold: f64): u8? {
    if (!isFinite(threshold) || threshold < 0.0 || threshold > 1.0) {
        return null;
    }
    if (p := predictProbaBinary(w, bias, sample)) {
        return if (p >= threshold) 1 else 0;
    }
    return null;
}

fn evalMultinomialLossTrial(
    x: []const f64,
    y_class: []const usize,
    n_samples: usize,
    n_features: usize,
    n_classes: usize,
    w: []const f64,
    b: []const f64,
    grad_w: []const f64,
    grad_b: []const f64,
    prob_s: *[]f64,
    step: f64,
    l2_reg: f64,
): f64 {
    let loss_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n_samples) {
        const row_off = i * n_features;
        let max_z: f64 = 0.0;
        let c: usize = 0;
        while (c < n_classes) {
            const w_off = c * n_features;
            let z = b[c] - step * grad_b[c];
            let j: usize = 0;
            while (j < n_features) {
                const wcj = w[w_off + j] - step * grad_w[w_off + j];
                z += wcj * x[row_off + j];
                j += 1;
            }
            prob_s.*[c] = z;
            if (c == 0 || z > max_z) {
                max_z = z;
            }
            c += 1;
        }
        let sum_e: f64 = 0.0;
        let c2: usize = 0;
        while (c2 < n_classes) {
            sum_e += @exp(prob_s.*[c2] - max_z);
            c2 += 1;
        }
        const lse = max_z + @log(sum_e);
        loss_sum += lse - prob_s.*[y_class[i]];
        i += 1;
    }

    let reg_sum: f64 = 0.0;
    const w_total = n_classes * n_features;
    let wi: usize = 0;
    while (wi < w_total) {
        const wv = w[wi] - step * grad_w[wi];
        reg_sum += wv * wv;
        wi += 1;
    }
    return (loss_sum / (n_samples as f64)) + 0.5 * l2_reg * reg_sum;
}

/// Zero-allocation deterministic L2-regularized multinomial (Softmax) logistic regression.
/// `w_out` & `grad_w_s` require length `>= n_classes * n_features` (row-major by class);
/// `b_out`, `grad_b_s`, and `prob_s` require length `>= n_classes`.
pub fn fitMultinomial(
    x: []const f64,
    y_class: []const usize,
    n_samples: usize,
    n_features: usize,
    n_classes: usize,
    cfg: LogRegConfig,
    w_out: *[]f64,
    b_out: *[]f64,
    grad_w_s: *[]f64,
    grad_b_s: *[]f64,
    prob_s: *[]f64,
): MultinomialResult? {
    if (n_samples == 0 || n_features == 0 || n_classes < 2) {
        return null;
    }
    if (!validateConfig(cfg)) {
        return null;
    }
    if (x.len / n_samples < n_features || y_class.len < n_samples) {
        return null;
    }
    if (w_out.*.len / n_classes < n_features || grad_w_s.*.len / n_classes < n_features) {
        return null;
    }
    if (b_out.*.len < n_classes || grad_b_s.*.len < n_classes || prob_s.*.len < n_classes) {
        return null;
    }

    const total_x = n_samples * n_features;
    let xi: usize = 0;
    while (xi < total_x) {
        if (!isFinite(x[xi])) {
            return null;
        }
        xi += 1;
    }
    let yi_idx: usize = 0;
    while (yi_idx < n_samples) {
        if (y_class[yi_idx] >= n_classes) {
            return null;
        }
        yi_idx += 1;
    }

    const w_total = n_classes * n_features;
    let wi: usize = 0;
    while (wi < w_total) {
        w_out.*[wi] = 0.0;
        grad_w_s.*[wi] = 0.0;
        wi += 1;
    }
    let ci: usize = 0;
    while (ci < n_classes) {
        b_out.*[ci] = 0.0;
        grad_b_s.*[ci] = 0.0;
        prob_s.*[ci] = 0.0;
        ci += 1;
    }

    const inv_n = 1.0 / (n_samples as f64);
    const max_step = if (cfg.learning_rate * 16.0 > 10.0) cfg.learning_rate * 16.0 else 10.0;
    let step: f64 = cfg.learning_rate;
    let cur_loss: f64 = 0.0;
    let cur_grad_norm: f64 = 0.0;
    let iters: usize = 0;
    let converged = false;

    while (iters < cfg.max_iter) {
        let wz: usize = 0;
        while (wz < w_total) {
            grad_w_s.*[wz] = 0.0;
            wz += 1;
        }
        let bz: usize = 0;
        while (bz < n_classes) {
            grad_b_s.*[bz] = 0.0;
            bz += 1;
        }

        let loss_sum: f64 = 0.0;
        let i: usize = 0;
        while (i < n_samples) {
            const row_off = i * n_features;
            let max_z: f64 = 0.0;
            let c: usize = 0;
            while (c < n_classes) {
                const w_off = c * n_features;
                let z = b_out.*[c];
                let j: usize = 0;
                while (j < n_features) {
                    z += w_out.*[w_off + j] * x[row_off + j];
                    j += 1;
                }
                prob_s.*[c] = z;
                if (c == 0 || z > max_z) {
                    max_z = z;
                }
                c += 1;
            }

            const y_c = y_class[i];
            const z_true = prob_s.*[y_c];
            let sum_e: f64 = 0.0;
            let c2: usize = 0;
            while (c2 < n_classes) {
                const e = @exp(prob_s.*[c2] - max_z);
                prob_s.*[c2] = e;
                sum_e += e;
                c2 += 1;
            }
            loss_sum += (max_z + @log(sum_e)) - z_true;

            let c3: usize = 0;
            while (c3 < n_classes) {
                const p = prob_s.*[c3] / sum_e;
                const diff = if (c3 == y_c) p - 1.0 else p;
                grad_b_s.*[c3] += diff;
                const w_off = c3 * n_features;
                let j2: usize = 0;
                while (j2 < n_features) {
                    grad_w_s.*[w_off + j2] += diff * x[row_off + j2];
                    j2 += 1;
                }
                c3 += 1;
            }
            i += 1;
        }

        let reg_sum: f64 = 0.0;
        let g2: f64 = 0.0;
        let c4: usize = 0;
        while (c4 < n_classes) {
            const gb = grad_b_s.*[c4] * inv_n;
            grad_b_s.*[c4] = gb;
            g2 += gb * gb;
            c4 += 1;
        }
        let f: usize = 0;
        while (f < w_total) {
            const wf = w_out.*[f];
            reg_sum += wf * wf;
            const gf = grad_w_s.*[f] * inv_n + cfg.l2_reg * wf;
            grad_w_s.*[f] = gf;
            g2 += gf * gf;
            f += 1;
        }

        cur_loss = loss_sum * inv_n + 0.5 * cfg.l2_reg * reg_sum;
        cur_grad_norm = @sqrt(g2);

        if (cur_grad_norm <= cfg.tol) {
            converged = true;
            break;
        }

        // Armijo backtracking line search
        let trial = step;
        let accepted = false;
        let bt: usize = 0;
        let next_loss = cur_loss;
        while (bt < MAX_BACKTRACK) {
            const cand_loss = evalMultinomialLossTrial(
                x,
                y_class,
                n_samples,
                n_features,
                n_classes,
                w_out.*,
                b_out.*,
                grad_w_s.*,
                grad_b_s.*,
                prob_s,
                trial,
                cfg.l2_reg,
            );
            if (isFinite(cand_loss) && cand_loss <= cur_loss - ARMIJO_C1 * trial * g2) {
                accepted = true;
                next_loss = cand_loss;
                break;
            }
            trial *= 0.5;
            bt += 1;
        }

        if (!accepted) {
            converged = true;
            break;
        }

        let u: usize = 0;
        while (u < w_total) {
            w_out.*[u] -= trial * grad_w_s.*[u];
            u += 1;
        }
        let uc: usize = 0;
        while (uc < n_classes) {
            b_out.*[uc] -= trial * grad_b_s.*[uc];
            uc += 1;
        }
        cur_loss = next_loss;
        iters += 1;

        const grown = trial * 1.5;
        step = if (grown < max_step) grown else max_step;
    }

    if (!converged) {
        let wz: usize = 0;
        while (wz < w_total) {
            grad_w_s.*[wz] = 0.0;
            wz += 1;
        }
        let bz: usize = 0;
        while (bz < n_classes) {
            grad_b_s.*[bz] = 0.0;
            bz += 1;
        }
        let loss_sum: f64 = 0.0;
        let i: usize = 0;
        while (i < n_samples) {
            const row_off = i * n_features;
            let max_z: f64 = 0.0;
            let c: usize = 0;
            while (c < n_classes) {
                const w_off = c * n_features;
                let z = b_out.*[c];
                let j: usize = 0;
                while (j < n_features) {
                    z += w_out.*[w_off + j] * x[row_off + j];
                    j += 1;
                }
                prob_s.*[c] = z;
                if (c == 0 || z > max_z) {
                    max_z = z;
                }
                c += 1;
            }
            const y_c = y_class[i];
            const z_true = prob_s.*[y_c];
            let sum_e: f64 = 0.0;
            let c2: usize = 0;
            while (c2 < n_classes) {
                const e = @exp(prob_s.*[c2] - max_z);
                prob_s.*[c2] = e;
                sum_e += e;
                c2 += 1;
            }
            loss_sum += (max_z + @log(sum_e)) - z_true;
            let c3: usize = 0;
            while (c3 < n_classes) {
                const p = prob_s.*[c3] / sum_e;
                const diff = if (c3 == y_c) p - 1.0 else p;
                grad_b_s.*[c3] += diff;
                const w_off = c3 * n_features;
                let j2: usize = 0;
                while (j2 < n_features) {
                    grad_w_s.*[w_off + j2] += diff * x[row_off + j2];
                    j2 += 1;
                }
                c3 += 1;
            }
            i += 1;
        }
        let reg_sum: f64 = 0.0;
        let g2: f64 = 0.0;
        let c4: usize = 0;
        while (c4 < n_classes) {
            const gb = grad_b_s.*[c4] * inv_n;
            grad_b_s.*[c4] = gb;
            g2 += gb * gb;
            c4 += 1;
        }
        let f: usize = 0;
        while (f < w_total) {
            const wf = w_out.*[f];
            reg_sum += wf * wf;
            const gf = grad_w_s.*[f] * inv_n + cfg.l2_reg * wf;
            grad_w_s.*[f] = gf;
            g2 += gf * gf;
            f += 1;
        }
        cur_loss = loss_sum * inv_n + 0.5 * cfg.l2_reg * reg_sum;
        cur_grad_norm = @sqrt(g2);
        if (cur_grad_norm <= cfg.tol) {
            converged = true;
        }
    }

    return {
        loss: cur_loss,
        grad_norm: cur_grad_norm,
        iterations: iters,
        converged: converged,
    };
}

/// Computes normalized class probabilities for a single sample using multinomial weights `w` & biases `b`.
/// Writes probabilities to `proba_out[0..n_classes]`; returns `false` on dimension mismatch or non-finite inputs.
pub fn predictProbaMultinomial(
    w: []const f64,
    b: []const f64,
    n_classes: usize,
    n_features: usize,
    sample: []const f64,
    proba_out: *[]f64,
): bool {
    if (n_classes < 2 || n_features == 0) {
        return false;
    }
    if (sample.len != n_features || b.len < n_classes || w.len / n_classes < n_features || proba_out.*.len < n_classes) {
        return false;
    }
    let j_chk: usize = 0;
    while (j_chk < n_features) {
        if (!isFinite(sample[j_chk])) {
            return false;
        }
        j_chk += 1;
    }

    let max_z: f64 = 0.0;
    let c: usize = 0;
    while (c < n_classes) {
        const bc = b[c];
        if (!isFinite(bc)) {
            return false;
        }
        const w_off = c * n_features;
        let z = bc;
        let j: usize = 0;
        while (j < n_features) {
            const wcj = w[w_off + j];
            if (!isFinite(wcj)) {
                return false;
            }
            z += wcj * sample[j];
            j += 1;
        }
        if (!isFinite(z)) {
            return false;
        }
        proba_out.*[c] = z;
        if (c == 0 || z > max_z) {
            max_z = z;
        }
        c += 1;
    }

    let sum_e: f64 = 0.0;
    let c2: usize = 0;
    while (c2 < n_classes) {
        const e = @exp(proba_out.*[c2] - max_z);
        proba_out.*[c2] = e;
        sum_e += e;
        c2 += 1;
    }
    if (!isFinite(sum_e) || sum_e <= 0.0) {
        return false;
    }
    let c3: usize = 0;
    while (c3 < n_classes) {
        proba_out.*[c3] = proba_out.*[c3] / sum_e;
        c3 += 1;
    }
    return true;
}

/// Predicts the argmax class index `0..n_classes - 1` for a single sample using caller `proba_scratch`.
/// Returns `null` on invalid dimensions or non-finite inputs.
pub fn predictClassMultinomial(
    w: []const f64,
    b: []const f64,
    n_classes: usize,
    n_features: usize,
    sample: []const f64,
    proba_scratch: *[]f64,
): usize? {
    if (!predictProbaMultinomial(w, b, n_classes, n_features, sample, proba_scratch)) {
        return null;
    }
    let best_cls: usize = 0;
    let best_p = proba_scratch.*[0];
    let c: usize = 1;
    while (c < n_classes) {
        if (proba_scratch.*[c] > best_p) {
            best_p = proba_scratch.*[c];
            best_cls = c;
        }
        c += 1;
    }
    return best_cls;
}

/// Computes binary confusion matrix (`tp`, `fp`, `tn`, `fn_count`) and `accuracy`, `precision`, `recall`, `f1`.
/// Returns `null` on empty/mismatched slices, invalid threshold/probabilities/labels, or non-finite values.
pub fn binaryMetrics(probs: []const f64, y: []const f64, threshold: f64): ClassificationMetrics? {
    const n = probs.len;
    if (n == 0 || n != y.len || !isFinite(threshold) || threshold < 0.0 || threshold > 1.0) {
        return null;
    }
    let tp: usize = 0;
    let fp: usize = 0;
    let tn: usize = 0;
    let fn_count: usize = 0;

    let i: usize = 0;
    while (i < n) {
        const p = probs[i];
        const yi = y[i];
        if (!isFinite(p) || p < 0.0 || p > 1.0) {
            return null;
        }
        if (yi != 0.0 && yi != 1.0) {
            return null;
        }
        const pred_pos = p >= threshold;
        const true_pos = yi == 1.0;
        if (pred_pos && true_pos) {
            tp += 1;
        } else if (pred_pos && !true_pos) {
            fp += 1;
        } else if (!pred_pos && true_pos) {
            fn_count += 1;
        } else {
            tn += 1;
        }
        i += 1;
    }

    const accuracy = ((tp + tn) as f64) / (n as f64);
    const precision = if (tp + fp > 0) (tp as f64) / ((tp + fp) as f64) else 0.0;
    const recall = if (tp + fn_count > 0) (tp as f64) / ((tp + fn_count) as f64) else 0.0;
    const f1 = if (precision + recall > 0.0) (2.0 * precision * recall) / (precision + recall) else 0.0;

    return {
        tp: tp,
        fp: fp,
        tn: tn,
        fn_count: fn_count,
        accuracy: accuracy,
        precision: precision,
        recall: recall,
        f1: f1,
    };
}
