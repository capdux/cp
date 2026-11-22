// Origin: Pure-CP Stdlib (Zero-Alloc Diagonal-Covariance Gaussian Mixture Model: EM Fit, Log-Sum-Exp Density, Posterior Prediction & AIC/BIC) | Created: 2026-11-22

const LOG_2PI: f64 = 1.8378770664093453;

/// Configuration for `gmmFit`.
pub struct GmmConfig {
    max_iter: usize,
    tol: f64,
    reg_var: f64,
    seed: u64,

    /// Standard defaults (`max_iter = 200, tol = 1e-6, reg_var = 1e-6, seed = 42`).
    fn default(): GmmConfig {
        return GmmConfig {
            max_iter: 200,
            tol: 1.0e-6,
            reg_var: 1.0e-6,
            seed: 42,
        };
    }
}

/// Summary returned by `gmmFit`.
pub struct GmmFitResult {
    converged: bool,
    iterations: usize,
    log_likelihood: f64,
    aic: f64,
    bic: f64,
}

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn allFinite(s: []const f64, len: usize): bool {
    let i: usize = 0;
    while (i < len) {
        if (!isFiniteF64(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn splitMix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn splitMix64Unit(state: *u64): f64 {
    const bits = splitMix64Next(state) >> 11;
    return (bits as f64) * (1.0 / 9007199254740992.0);
}

fn validateModelParams(
    k: usize,
    n_features: usize,
    weights: []const f64,
    means: []const f64,
    vars: []const f64,
): bool {
    if (k == 0 || n_features == 0) {
        return false;
    }
    if (weights.len < k || means.len < k * n_features || vars.len < k * n_features) {
        return false;
    }
    let c: usize = 0;
    while (c < k) {
        if (!isFiniteF64(weights[c]) || weights[c] <= 0.0) {
            return false;
        }
        let j: usize = 0;
        while (j < n_features) {
            const idx = c * n_features + j;
            if (!isFiniteF64(means[idx]) || !isFiniteF64(vars[idx]) || vars[idx] <= 0.0) {
                return false;
            }
            j += 1;
        }
        c += 1;
    }
    return true;
}

fn logJointComponent(
    sample: []const f64,
    n_features: usize,
    weight: f64,
    mean_row: []const f64,
    var_row: []const f64,
): f64 {
    let quad: f64 = 0.0;
    let log_det: f64 = 0.0;
    let j: usize = 0;
    while (j < n_features) {
        const diff = sample[j] - mean_row[j];
        const v = var_row[j];
        quad += (diff * diff) / v;
        log_det += @log(v);
        j += 1;
    }
    return @log(weight) - 0.5 * ((n_features as f64) * LOG_2PI + log_det + quad);
}

/// Computes the exact log probability density `log p(sample)` under a diagonal-covariance GMM via log-sum-exp.
/// Returns `null` on invalid parameters or non-finite inputs.
pub fn gmmLogProbSample(
    sample: []const f64,
    k: usize,
    n_features: usize,
    weights: []const f64,
    means: []const f64,
    vars: []const f64,
): f64? {
    if (sample.len < n_features || !allFinite(sample, n_features)) {
        return null;
    }
    if (!validateModelParams(k, n_features, weights, means, vars)) {
        return null;
    }

    // Two-pass log-sum-exp (zero scratch memory regardless of k)
    let max_lj = logJointComponent(
        sample,
        n_features,
        weights[0],
        means[0..n_features],
        vars[0..n_features],
    );
    let c: usize = 1;
    while (c < k) {
        const off = c * n_features;
        const lj = logJointComponent(
            sample,
            n_features,
            weights[c],
            means[off .. off + n_features],
            vars[off .. off + n_features],
        );
        if (lj > max_lj) {
            max_lj = lj;
        }
        c += 1;
    }

    let sum_exp: f64 = 0.0;
    c = 0;
    while (c < k) {
        const off = c * n_features;
        const lj = logJointComponent(
            sample,
            n_features,
            weights[c],
            means[off .. off + n_features],
            vars[off .. off + n_features],
        );
        sum_exp += @exp(lj - max_lj);
        c += 1;
    }

    return max_lj + @log(sum_exp);
}

/// Computes posterior responsibilities `P(c | x_i)` into `proba_out` (`n_samples x k` row-major).
/// Each row sums to 1.0. Returns `false` on invalid parameters, undersized buffers, or non-finite inputs.
pub fn gmmPredictProba(
    data: []const f64,
    n_samples: usize,
    n_features: usize,
    k: usize,
    weights: []const f64,
    means: []const f64,
    vars: []const f64,
    proba_out: *[]f64,
): bool {
    if (n_samples == 0 || data.len < n_samples * n_features || proba_out.*.len < n_samples * k) {
        return false;
    }
    if (!validateModelParams(k, n_features, weights, means, vars)) {
        return false;
    }
    if (!allFinite(data, n_samples * n_features)) {
        return false;
    }

    let i: usize = 0;
    while (i < n_samples) {
        const sample = data[i * n_features .. (i + 1) * n_features];
        const row_off = i * k;

        let max_lj = logJointComponent(sample, n_features, weights[0], means[0..n_features], vars[0..n_features]);
        proba_out.*[row_off] = max_lj;
        let c: usize = 1;
        while (c < k) {
            const off = c * n_features;
            const lj = logJointComponent(
                sample,
                n_features,
                weights[c],
                means[off .. off + n_features],
                vars[off .. off + n_features],
            );
            proba_out.*[row_off + c] = lj;
            if (lj > max_lj) {
                max_lj = lj;
            }
            c += 1;
        }

        let sum_exp: f64 = 0.0;
        c = 0;
        while (c < k) {
            const e = @exp(proba_out.*[row_off + c] - max_lj);
            proba_out.*[row_off + c] = e;
            sum_exp += e;
            c += 1;
        }

        const inv_sum = 1.0 / sum_exp;
        c = 0;
        while (c < k) {
            proba_out.*[row_off + c] *= inv_sum;
            c += 1;
        }
        i += 1;
    }

    return true;
}

/// Predicts hard cluster assignments `argmax_c P(c | x_i)` into `labels_out[0..n_samples]`.
/// Returns `false` on invalid parameters, undersized buffers, or non-finite inputs.
pub fn gmmPredict(
    data: []const f64,
    n_samples: usize,
    n_features: usize,
    k: usize,
    weights: []const f64,
    means: []const f64,
    vars: []const f64,
    labels_out: *[]usize,
): bool {
    if (n_samples == 0 || data.len < n_samples * n_features || labels_out.*.len < n_samples) {
        return false;
    }
    if (!validateModelParams(k, n_features, weights, means, vars)) {
        return false;
    }
    if (!allFinite(data, n_samples * n_features)) {
        return false;
    }

    let i: usize = 0;
    while (i < n_samples) {
        const sample = data[i * n_features .. (i + 1) * n_features];
        let best_c: usize = 0;
        let best_lj = logJointComponent(sample, n_features, weights[0], means[0..n_features], vars[0..n_features]);

        let c: usize = 1;
        while (c < k) {
            const off = c * n_features;
            const lj = logJointComponent(
                sample,
                n_features,
                weights[c],
                means[off .. off + n_features],
                vars[off .. off + n_features],
            );
            if (lj > best_lj) {
                best_lj = lj;
                best_c = c;
            }
            c += 1;
        }
        labels_out.*[i] = best_c;
        i += 1;
    }

    return true;
}

/// Evaluates per-sample log density into `log_probs_out[0..n_samples]` and returns the total log-likelihood.
/// Returns `null` on invalid parameters, undersized buffers, or non-finite inputs.
pub fn gmmScoreSamples(
    data: []const f64,
    n_samples: usize,
    n_features: usize,
    k: usize,
    weights: []const f64,
    means: []const f64,
    vars: []const f64,
    log_probs_out: *[]f64,
): f64? {
    if (n_samples == 0 || data.len < n_samples * n_features || log_probs_out.*.len < n_samples) {
        return null;
    }
    if (!validateModelParams(k, n_features, weights, means, vars)) {
        return null;
    }

    let total_ll: f64 = 0.0;
    let i: usize = 0;
    while (i < n_samples) {
        const sample = data[i * n_features .. (i + 1) * n_features];
        const lp = gmmLogProbSample(sample, k, n_features, weights, means, vars) ?? return null;
        log_probs_out.*[i] = lp;
        total_ll += lp;
        i += 1;
    }

    return total_ll;
}

/// Fits a diagonal-covariance Gaussian Mixture Model via Expectation-Maximization (EM)
/// with deterministic SplitMix64 k-means++ mean initialization and `reg_var` variance floor.
/// - Required buffer lengths: `weights_out >= k`, `means_out >= k*d`, `vars_out >= k*d`, `resp_scratch >= n*k`.
/// - Returns `null` on invalid dimensions (`k == 0 || k > n_samples`), invalid config, undersized buffers, or non-finite data.
pub fn gmmFit(
    data: []const f64,
    n_samples: usize,
    n_features: usize,
    k: usize,
    cfg: GmmConfig,
    weights_out: *[]f64,
    means_out: *[]f64,
    vars_out: *[]f64,
    resp_scratch: *[]f64,
): GmmFitResult? {
    if (
        n_samples == 0 || n_features == 0 || k == 0 || k > n_samples ||
        data.len < n_samples * n_features ||
        weights_out.*.len < k ||
        means_out.*.len < k * n_features ||
        vars_out.*.len < k * n_features ||
        resp_scratch.*.len < n_samples * k
    ) {
        return null;
    }
    if (
        !isFiniteF64(cfg.tol) || cfg.tol < 0.0 ||
        !isFiniteF64(cfg.reg_var) || cfg.reg_var <= 0.0
    ) {
        return null;
    }
    if (!allFinite(data, n_samples * n_features)) {
        return null;
    }

    // 1. Deterministic SplitMix64 k-means++ initialization (reusing resp_scratch[0..n_samples] for D^2)
    let rng = cfg.seed;
    const first_idx = (splitMix64Next(&rng) as usize) % n_samples;
    let j: usize = 0;
    while (j < n_features) {
        means_out.*[j] = data[first_idx * n_features + j];
        j += 1;
    }

    let i: usize = 0;
    while (i < n_samples) {
        let d2: f64 = 0.0;
        j = 0;
        while (j < n_features) {
            const diff = data[i * n_features + j] - means_out.*[j];
            d2 += diff * diff;
            j += 1;
        }
        resp_scratch.*[i] = d2;
        i += 1;
    }

    let c_init: usize = 1;
    while (c_init < k) {
        let total_d2: f64 = 0.0;
        i = 0;
        while (i < n_samples) {
            total_d2 += resp_scratch.*[i];
            i += 1;
        }

        let chosen: usize = (first_idx + c_init) % n_samples;
        if (total_d2 > 0.0) {
            const target = splitMix64Unit(&rng) * total_d2;
            let acc: f64 = 0.0;
            chosen = n_samples - 1;
            i = 0;
            while (i < n_samples) {
                acc += resp_scratch.*[i];
                if (acc >= target) {
                    chosen = i;
                    break;
                }
                i += 1;
            }
        }

        const c_off = c_init * n_features;
        j = 0;
        while (j < n_features) {
            means_out.*[c_off + j] = data[chosen * n_features + j];
            j += 1;
        }

        // Update min squared distances to chosen centers
        i = 0;
        while (i < n_samples) {
            let d2: f64 = 0.0;
            j = 0;
            while (j < n_features) {
                const diff = data[i * n_features + j] - means_out.*[c_off + j];
                d2 += diff * diff;
                j += 1;
            }
            if (d2 < resp_scratch.*[i]) {
                resp_scratch.*[i] = d2;
            }
            i += 1;
        }

        c_init += 1;
    }

    // Initialize uniform weights and global per-feature variance floor
    const init_w = 1.0 / (k as f64);
    let c: usize = 0;
    while (c < k) {
        weights_out.*[c] = init_w;
        c += 1;
    }

    j = 0;
    while (j < n_features) {
        let mean_j: f64 = 0.0;
        i = 0;
        while (i < n_samples) {
            mean_j += data[i * n_features + j];
            i += 1;
        }
        mean_j /= n_samples as f64;

        let var_j: f64 = 0.0;
        i = 0;
        while (i < n_samples) {
            const diff = data[i * n_features + j] - mean_j;
            var_j += diff * diff;
            i += 1;
        }
        var_j = var_j / (n_samples as f64) + cfg.reg_var;

        c = 0;
        while (c < k) {
            vars_out.*[c * n_features + j] = var_j;
            c += 1;
        }
        j += 1;
    }

    // 2. EM iterations
    let prev_ll: f64 = -1.0e300;
    let cur_ll: f64 = prev_ll;
    let converged = false;
    let iter: usize = 0;

    while (iter < cfg.max_iter) {
        // E-step: compute log-sum-exp responsibilities & total log-likelihood
        cur_ll = 0.0;
        i = 0;
        while (i < n_samples) {
            const sample = data[i * n_features .. (i + 1) * n_features];
            const row_off = i * k;

            let max_lj = logJointComponent(
                sample,
                n_features,
                weights_out.*[0],
                means_out.*[0..n_features],
                vars_out.*[0..n_features],
            );
            resp_scratch.*[row_off] = max_lj;

            c = 1;
            while (c < k) {
                const off = c * n_features;
                const lj = logJointComponent(
                    sample,
                    n_features,
                    weights_out.*[c],
                    means_out.*[off .. off + n_features],
                    vars_out.*[off .. off + n_features],
                );
                resp_scratch.*[row_off + c] = lj;
                if (lj > max_lj) {
                    max_lj = lj;
                }
                c += 1;
            }

            let sum_exp: f64 = 0.0;
            c = 0;
            while (c < k) {
                const e = @exp(resp_scratch.*[row_off + c] - max_lj);
                resp_scratch.*[row_off + c] = e;
                sum_exp += e;
                c += 1;
            }

            cur_ll += max_lj + @log(sum_exp);
            const inv_sum = 1.0 / sum_exp;
            c = 0;
            while (c < k) {
                resp_scratch.*[row_off + c] *= inv_sum;
                c += 1;
            }
            i += 1;
        }

        // M-step: update weights, means, and regularized diagonal variances
        let w_sum: f64 = 0.0;
        c = 0;
        while (c < k) {
            let nk: f64 = 0.0;
            i = 0;
            while (i < n_samples) {
                nk += resp_scratch.*[i * k + c];
                i += 1;
            }

            const c_off = c * n_features;
            if (nk <= 1.0e-12) {
                weights_out.*[c] = 1.0e-12;
                j = 0;
                while (j < n_features) {
                    vars_out.*[c_off + j] = cfg.reg_var;
                    j += 1;
                }
            } else {
                weights_out.*[c] = nk / (n_samples as f64);
                const inv_nk = 1.0 / nk;

                j = 0;
                while (j < n_features) {
                    let mu: f64 = 0.0;
                    i = 0;
                    while (i < n_samples) {
                        mu += resp_scratch.*[i * k + c] * data[i * n_features + j];
                        i += 1;
                    }
                    mu *= inv_nk;
                    means_out.*[c_off + j] = mu;

                    let v_acc: f64 = 0.0;
                    i = 0;
                    while (i < n_samples) {
                        const diff = data[i * n_features + j] - mu;
                        v_acc += resp_scratch.*[i * k + c] * diff * diff;
                        i += 1;
                    }
                    vars_out.*[c_off + j] = v_acc * inv_nk + cfg.reg_var;
                    j += 1;
                }
            }
            w_sum += weights_out.*[c];
            c += 1;
        }

        // Normalize weights strictly to sum to 1.0
        const inv_w = 1.0 / w_sum;
        c = 0;
        while (c < k) {
            weights_out.*[c] *= inv_w;
            c += 1;
        }

        iter += 1;
        if (iter > 1 && absF64(cur_ll - prev_ll) <= cfg.tol * (1.0 + absF64(cur_ll))) {
            converged = true;
            break;
        }
        prev_ll = cur_ll;
    }

    // Re-evaluate final log-likelihood with final M-step parameters
    let final_ll: f64 = 0.0;
    i = 0;
    while (i < n_samples) {
        const sample = data[i * n_features .. (i + 1) * n_features];
        const lp = gmmLogProbSample(
            sample,
            k,
            n_features,
            weights_out.*[0..k],
            means_out.*[0..k * n_features],
            vars_out.*[0..k * n_features],
        ) ?? return null;
        final_ll += lp;
        i += 1;
    }

    // Free parameter count for diagonal GMM: (k - 1) weights + k*d means + k*d variances
    const n_params = ((k - 1) + 2 * k * n_features) as f64;
    const aic = 2.0 * n_params - 2.0 * final_ll;
    const bic = n_params * @log(n_samples as f64) - 2.0 * final_ll;

    return GmmFitResult {
        converged: converged,
        iterations: iter,
        log_likelihood: final_ll,
        aic: aic,
        bic: bic,
    };
}
