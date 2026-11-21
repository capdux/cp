// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Naive Bayes Classifiers: Gaussian, Multinomial & Bernoulli)

const LOG_2PI: f64 = 1.8378770664093453;

fn negInfF64(): f64 {
    const z: f64 = 0.0;
    return -1.0 / z;
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn checkMatrixBounds(buf_len: usize, rows: usize, cols: usize): bool {
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

fn checkNonNegativeFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        const v: f64 = data[i];
        if (!isFiniteF64(v) || v < 0.0) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn checkLabelsRange(y: []const usize, n_samples: usize, num_classes: usize): bool {
    let i: usize = 0;
    while (i < n_samples) {
        if (y[i] >= num_classes) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Normalizes unnormalized log-probabilities in-place via log-sum-exp to prevent underflow.
fn normalizeLogProbs(log_scores: []f64, num_classes: usize): bool {
    if (num_classes == 0 || log_scores.len < num_classes) {
        return false;
    }
    let has_finite: bool = false;
    let max_lp: f64 = 0.0;
    let c: usize = 0;
    while (c < num_classes) {
        const v: f64 = log_scores[c];
        if (v != v) {
            return false;
        }
        if (isFiniteF64(v)) {
            if (!has_finite || v > max_lp) {
                max_lp = v;
                has_finite = true;
            }
        }
        c += 1;
    }
    if (!has_finite) {
        return false;
    }

    let sum_exp: f64 = 0.0;
    let i: usize = 0;
    while (i < num_classes) {
        const v: f64 = log_scores[i];
        if (isFiniteF64(v)) {
            const e: f64 = @exp(v - max_lp);
            log_scores[i] = e;
            sum_exp += e;
        } else {
            log_scores[i] = 0.0;
        }
        i += 1;
    }
    if (!isFiniteF64(sum_exp) || sum_exp <= 0.0) {
        return false;
    }

    const inv_sum: f64 = 1.0 / sum_exp;
    let j: usize = 0;
    while (j < num_classes) {
        log_scores[j] = log_scores[j] * inv_sum;
        j += 1;
    }
    return true;
}

fn argmaxSlice(vals: []const f64, count: usize): usize {
    let best_idx: usize = 0;
    let best_val: f64 = vals[0];
    let i: usize = 1;
    while (i < count) {
        if (vals[i] > best_val) {
            best_val = vals[i];
            best_idx = i;
        }
        i += 1;
    }
    return best_idx;
}

/// Fitted Gaussian Naive Bayes model backed by caller-provided parameter buffers.
pub struct GaussianNbModel {
    num_classes: usize,
    num_features: usize,
    log_priors: []f64,
    means: []f64,
    vars: []f64,
    class_counts: []usize,
}

/// Fits a Gaussian Naive Bayes classifier for continuous features using caller-provided buffers.
/// Adds `var_smoothing` (scaled by maximum feature variance when non-zero) to prevent zero-variance division.
pub fn fitGaussianNb(
    x: []const f64,
    y: []const usize,
    n_samples: usize,
    n_features: usize,
    num_classes: usize,
    var_smoothing: f64,
    log_priors_out: *[]f64,
    means_out: *[]f64,
    vars_out: *[]f64,
    counts_out: *[]usize,
): GaussianNbModel? {
    if (n_samples == 0 || n_features == 0 || num_classes == 0) {
        return null;
    }
    if (!checkMatrixBounds(x.len, n_samples, n_features) || y.len < n_samples) {
        return null;
    }
    if (!isFiniteF64(var_smoothing) || var_smoothing < 0.0) {
        return null;
    }
    if (log_priors_out.*.len < num_classes || counts_out.*.len < num_classes) {
        return null;
    }
    if (
        !checkMatrixBounds(means_out.*.len, num_classes, n_features) ||
        !checkMatrixBounds(vars_out.*.len, num_classes, n_features)
    ) {
        return null;
    }
    if (!checkFinitePrefix(x, n_samples * n_features)) {
        return null;
    }
    if (!checkLabelsRange(y, n_samples, num_classes)) {
        return null;
    }

    let log_priors: []f64 = log_priors_out.*;
    let means: []f64 = means_out.*;
    let vars: []f64 = vars_out.*;
    let counts: []usize = counts_out.*;

    const total_params: usize = num_classes * n_features;
    let c: usize = 0;
    while (c < num_classes) {
        counts[c] = 0;
        log_priors[c] = negInfF64();
        c += 1;
    }
    let p: usize = 0;
    while (p < total_params) {
        means[p] = 0.0;
        vars[p] = 0.0;
        p += 1;
    }

    // Pass 1: accumulate counts and feature sums per class.
    let i: usize = 0;
    while (i < n_samples) {
        const cls: usize = y[i];
        counts[cls] += 1;
        const x_row: usize = i * n_features;
        const m_row: usize = cls * n_features;
        let j: usize = 0;
        while (j < n_features) {
            means[m_row + j] += x[x_row + j];
            j += 1;
        }
        i += 1;
    }

    const n_f: f64 = n_samples as f64;
    let cls_i: usize = 0;
    while (cls_i < num_classes) {
        const cnt: usize = counts[cls_i];
        if (cnt > 0) {
            const cnt_f: f64 = cnt as f64;
            log_priors[cls_i] = @log(cnt_f / n_f);
            const m_row: usize = cls_i * n_features;
            let j: usize = 0;
            while (j < n_features) {
                means[m_row + j] = means[m_row + j] / cnt_f;
                j += 1;
            }
        }
        cls_i += 1;
    }

    // Pass 2: accumulate squared deviations per class.
    let s: usize = 0;
    while (s < n_samples) {
        const cls: usize = y[s];
        const x_row: usize = s * n_features;
        const m_row: usize = cls * n_features;
        let j: usize = 0;
        while (j < n_features) {
            const diff: f64 = x[x_row + j] - means[m_row + j];
            vars[m_row + j] += diff * diff;
            j += 1;
        }
        s += 1;
    }

    let max_var: f64 = 0.0;
    let c2: usize = 0;
    while (c2 < num_classes) {
        const cnt: usize = counts[c2];
        if (cnt > 0) {
            const inv_cnt: f64 = 1.0 / (cnt as f64);
            const m_row: usize = c2 * n_features;
            let j: usize = 0;
            while (j < n_features) {
                const v: f64 = vars[m_row + j] * inv_cnt;
                vars[m_row + j] = v;
                if (v > max_var) {
                    max_var = v;
                }
                j += 1;
            }
        }
        c2 += 1;
    }

    // Apply variance smoothing; falls back to raw `var_smoothing` when all features are constant.
    const eps: f64 = if (max_var > 0.0) (var_smoothing * max_var) else var_smoothing;
    let c3: usize = 0;
    while (c3 < num_classes) {
        const m_row: usize = c3 * n_features;
        let j: usize = 0;
        while (j < n_features) {
            const smoothed: f64 = vars[m_row + j] + eps;
            if (counts[c3] > 0 && (!isFiniteF64(smoothed) || smoothed <= 0.0)) {
                return null;
            }
            vars[m_row + j] = smoothed;
            j += 1;
        }
        c3 += 1;
    }

    return GaussianNbModel {
        num_classes: num_classes,
        num_features: n_features,
        log_priors: log_priors,
        means: means,
        vars: vars,
        class_counts: counts,
    };
}

/// Computes normalized posterior class probabilities for a continuous feature vector.
pub fn gaussianNbPredictProba(
    model: GaussianNbModel,
    sample: []const f64,
    proba_out: *[]f64,
): bool {
    const k: usize = model.num_classes;
    const d: usize = model.num_features;
    if (k == 0 || d == 0 || sample.len < d || proba_out.*.len < k) {
        return false;
    }
    if (
        model.log_priors.len < k ||
        model.class_counts.len < k ||
        !checkMatrixBounds(model.means.len, k, d) ||
        !checkMatrixBounds(model.vars.len, k, d)
    ) {
        return false;
    }
    if (!checkFinitePrefix(sample, d)) {
        return false;
    }

    let proba: []f64 = proba_out.*;
    let c: usize = 0;
    while (c < k) {
        const prior: f64 = model.log_priors[c];
        if (model.class_counts[c] == 0 || !isFiniteF64(prior)) {
            proba[c] = negInfF64();
        } else {
            let lp: f64 = prior;
            const row: usize = c * d;
            let j: usize = 0;
            while (j < d) {
                const v: f64 = model.vars[row + j];
                if (!isFiniteF64(v) || v <= 0.0) {
                    return false;
                }
                const diff: f64 = sample[j] - model.means[row + j];
                lp -= 0.5 * (LOG_2PI + @log(v) + ((diff * diff) / v));
                j += 1;
            }
            proba[c] = lp;
        }
        c += 1;
    }

    return normalizeLogProbs(proba, k);
}

/// Predicts the most probable class label under a Gaussian Naive Bayes model.
pub fn gaussianNbPredict(
    model: GaussianNbModel,
    sample: []const f64,
    proba_scratch: *[]f64,
): usize? {
    if (!gaussianNbPredictProba(model, sample, proba_scratch)) {
        return null;
    }
    return argmaxSlice(proba_scratch.*, model.num_classes);
}

/// Fitted Multinomial Naive Bayes model backed by caller-provided parameter buffers.
pub struct MultinomialNbModel {
    num_classes: usize,
    num_features: usize,
    log_priors: []f64,
    feature_log_probs: []f64,
}

/// Fits a Multinomial Naive Bayes classifier for non-negative count/frequency features
/// with Laplace/Lidstone smoothing parameter `alpha >= 0`.
pub fn fitMultinomialNb(
    counts: []const f64,
    y: []const usize,
    n_samples: usize,
    n_features: usize,
    num_classes: usize,
    alpha: f64,
    log_priors_out: *[]f64,
    feat_log_probs_out: *[]f64,
    class_counts_s: *[]usize,
): MultinomialNbModel? {
    if (n_samples == 0 || n_features == 0 || num_classes == 0) {
        return null;
    }
    if (!checkMatrixBounds(counts.len, n_samples, n_features) || y.len < n_samples) {
        return null;
    }
    if (!isFiniteF64(alpha) || alpha < 0.0) {
        return null;
    }
    if (log_priors_out.*.len < num_classes || class_counts_s.*.len < num_classes) {
        return null;
    }
    if (!checkMatrixBounds(feat_log_probs_out.*.len, num_classes, n_features)) {
        return null;
    }
    if (!checkNonNegativeFinitePrefix(counts, n_samples * n_features)) {
        return null;
    }
    if (!checkLabelsRange(y, n_samples, num_classes)) {
        return null;
    }

    let log_priors: []f64 = log_priors_out.*;
    let feat_log_probs: []f64 = feat_log_probs_out.*;
    let class_counts: []usize = class_counts_s.*;

    const total_params: usize = num_classes * n_features;
    let c: usize = 0;
    while (c < num_classes) {
        class_counts[c] = 0;
        log_priors[c] = negInfF64();
        c += 1;
    }
    let p: usize = 0;
    while (p < total_params) {
        feat_log_probs[p] = 0.0;
        p += 1;
    }

    // Accumulate class frequencies and per-class feature counts.
    let i: usize = 0;
    while (i < n_samples) {
        const cls: usize = y[i];
        class_counts[cls] += 1;
        const x_row: usize = i * n_features;
        const f_row: usize = cls * n_features;
        let j: usize = 0;
        while (j < n_features) {
            feat_log_probs[f_row + j] += counts[x_row + j];
            j += 1;
        }
        i += 1;
    }

    const n_f: f64 = n_samples as f64;
    const d_f: f64 = n_features as f64;
    let cls_i: usize = 0;
    while (cls_i < num_classes) {
        const cnt: usize = class_counts[cls_i];
        const f_row: usize = cls_i * n_features;
        if (cnt > 0) {
            log_priors[cls_i] = @log((cnt as f64) / n_f);
            let total_word_count: f64 = 0.0;
            let j: usize = 0;
            while (j < n_features) {
                total_word_count += feat_log_probs[f_row + j];
                j += 1;
            }
            const denom: f64 = total_word_count + (alpha * d_f);
            if (!isFiniteF64(denom) || denom <= 0.0) {
                return null;
            }
            const log_denom: f64 = @log(denom);
            let j2: usize = 0;
            while (j2 < n_features) {
                const num: f64 = feat_log_probs[f_row + j2] + alpha;
                if (num > 0.0) {
                    feat_log_probs[f_row + j2] = @log(num) - log_denom;
                } else {
                    feat_log_probs[f_row + j2] = negInfF64();
                }
                j2 += 1;
            }
        } else {
            let j: usize = 0;
            while (j < n_features) {
                feat_log_probs[f_row + j] = negInfF64();
                j += 1;
            }
        }
        cls_i += 1;
    }

    return MultinomialNbModel {
        num_classes: num_classes,
        num_features: n_features,
        log_priors: log_priors,
        feature_log_probs: feat_log_probs,
    };
}

/// Computes normalized posterior class probabilities for a non-negative count vector.
pub fn multinomialNbPredictProba(
    model: MultinomialNbModel,
    sample: []const f64,
    proba_out: *[]f64,
): bool {
    const k: usize = model.num_classes;
    const d: usize = model.num_features;
    if (k == 0 || d == 0 || sample.len < d || proba_out.*.len < k) {
        return false;
    }
    if (model.log_priors.len < k || !checkMatrixBounds(model.feature_log_probs.len, k, d)) {
        return false;
    }
    if (!checkNonNegativeFinitePrefix(sample, d)) {
        return false;
    }

    let proba: []f64 = proba_out.*;
    let c: usize = 0;
    while (c < k) {
        const prior: f64 = model.log_priors[c];
        if (!isFiniteF64(prior)) {
            proba[c] = negInfF64();
        } else {
            let lp: f64 = prior;
            const row: usize = c * d;
            let j: usize = 0;
            while (j < d) {
                const cnt: f64 = sample[j];
                if (cnt > 0.0) {
                    const flp: f64 = model.feature_log_probs[row + j];
                    if (isFiniteF64(flp)) {
                        lp += cnt * flp;
                    } else {
                        lp = negInfF64();
                        break;
                    }
                }
                j += 1;
            }
            proba[c] = lp;
        }
        c += 1;
    }

    return normalizeLogProbs(proba, k);
}

/// Predicts the most probable class label under a Multinomial Naive Bayes model.
pub fn multinomialNbPredict(
    model: MultinomialNbModel,
    sample: []const f64,
    proba_scratch: *[]f64,
): usize? {
    if (!multinomialNbPredictProba(model, sample, proba_scratch)) {
        return null;
    }
    return argmaxSlice(proba_scratch.*, model.num_classes);
}

/// Fitted Bernoulli Naive Bayes model backed by caller-provided parameter buffers.
pub struct BernoulliNbModel {
    num_classes: usize,
    num_features: usize,
    binarize_threshold: f64,
    log_priors: []f64,
    feat_log_prob_1: []f64,
    feat_log_prob_0: []f64,
}

/// Fits a Bernoulli Naive Bayes classifier with threshold binarization (`x > binarize_threshold`)
/// and additive smoothing parameter `alpha >= 0`.
pub fn fitBernoulliNb(
    x: []const f64,
    y: []const usize,
    n_samples: usize,
    n_features: usize,
    num_classes: usize,
    alpha: f64,
    binarize_threshold: f64,
    log_priors_out: *[]f64,
    log_p1_out: *[]f64,
    log_p0_out: *[]f64,
    class_counts_s: *[]usize,
): BernoulliNbModel? {
    if (n_samples == 0 || n_features == 0 || num_classes == 0) {
        return null;
    }
    if (!checkMatrixBounds(x.len, n_samples, n_features) || y.len < n_samples) {
        return null;
    }
    if (!isFiniteF64(alpha) || alpha < 0.0 || !isFiniteF64(binarize_threshold)) {
        return null;
    }
    if (log_priors_out.*.len < num_classes || class_counts_s.*.len < num_classes) {
        return null;
    }
    if (
        !checkMatrixBounds(log_p1_out.*.len, num_classes, n_features) ||
        !checkMatrixBounds(log_p0_out.*.len, num_classes, n_features)
    ) {
        return null;
    }
    if (!checkFinitePrefix(x, n_samples * n_features)) {
        return null;
    }
    if (!checkLabelsRange(y, n_samples, num_classes)) {
        return null;
    }

    let log_priors: []f64 = log_priors_out.*;
    let log_p1: []f64 = log_p1_out.*;
    let log_p0: []f64 = log_p0_out.*;
    let class_counts: []usize = class_counts_s.*;

    const total_params: usize = num_classes * n_features;
    let c: usize = 0;
    while (c < num_classes) {
        class_counts[c] = 0;
        log_priors[c] = negInfF64();
        c += 1;
    }
    let p: usize = 0;
    while (p < total_params) {
        log_p1[p] = 0.0;
        log_p0[p] = 0.0;
        p += 1;
    }

    // Count class totals and per-class active (x > threshold) feature occurrences.
    let i: usize = 0;
    while (i < n_samples) {
        const cls: usize = y[i];
        class_counts[cls] += 1;
        const x_row: usize = i * n_features;
        const f_row: usize = cls * n_features;
        let j: usize = 0;
        while (j < n_features) {
            if (x[x_row + j] > binarize_threshold) {
                log_p1[f_row + j] += 1.0;
            }
            j += 1;
        }
        i += 1;
    }

    const n_f: f64 = n_samples as f64;
    let cls_i: usize = 0;
    while (cls_i < num_classes) {
        const cnt: usize = class_counts[cls_i];
        const f_row: usize = cls_i * n_features;
        if (cnt > 0) {
            const cnt_f: f64 = cnt as f64;
            log_priors[cls_i] = @log(cnt_f / n_f);
            const denom: f64 = cnt_f + (2.0 * alpha);
            if (!isFiniteF64(denom) || denom <= 0.0) {
                return null;
            }
            const log_denom: f64 = @log(denom);
            let j: usize = 0;
            while (j < n_features) {
                const active_cnt: f64 = log_p1[f_row + j];
                const absent_cnt: f64 = cnt_f - active_cnt;
                const num1: f64 = active_cnt + alpha;
                const num0: f64 = absent_cnt + alpha;
                log_p1[f_row + j] = if (num1 > 0.0) (@log(num1) - log_denom) else negInfF64();
                log_p0[f_row + j] = if (num0 > 0.0) (@log(num0) - log_denom) else negInfF64();
                j += 1;
            }
        } else {
            let j: usize = 0;
            while (j < n_features) {
                log_p1[f_row + j] = negInfF64();
                log_p0[f_row + j] = negInfF64();
                j += 1;
            }
        }
        cls_i += 1;
    }

    return BernoulliNbModel {
        num_classes: num_classes,
        num_features: n_features,
        binarize_threshold: binarize_threshold,
        log_priors: log_priors,
        feat_log_prob_1: log_p1,
        feat_log_prob_0: log_p0,
    };
}

/// Computes normalized posterior class probabilities for a feature vector (binarized via `> binarize_threshold`),
/// explicitly penalizing absent features using `feat_log_prob_0`.
pub fn bernoulliNbPredictProba(
    model: BernoulliNbModel,
    sample: []const f64,
    proba_out: *[]f64,
): bool {
    const k: usize = model.num_classes;
    const d: usize = model.num_features;
    if (k == 0 || d == 0 || sample.len < d || proba_out.*.len < k) {
        return false;
    }
    if (
        model.log_priors.len < k ||
        !checkMatrixBounds(model.feat_log_prob_1.len, k, d) ||
        !checkMatrixBounds(model.feat_log_prob_0.len, k, d)
    ) {
        return false;
    }
    if (!checkFinitePrefix(sample, d)) {
        return false;
    }

    let proba: []f64 = proba_out.*;
    let c: usize = 0;
    while (c < k) {
        const prior: f64 = model.log_priors[c];
        if (!isFiniteF64(prior)) {
            proba[c] = negInfF64();
        } else {
            let lp: f64 = prior;
            const row: usize = c * d;
            let j: usize = 0;
            while (j < d) {
                const flp: f64 = if (sample[j] > model.binarize_threshold) {
                    model.feat_log_prob_1[row + j]
                } else {
                    model.feat_log_prob_0[row + j]
                };
                if (isFiniteF64(flp)) {
                    lp += flp;
                } else {
                    lp = negInfF64();
                    break;
                }
                j += 1;
            }
            proba[c] = lp;
        }
        c += 1;
    }

    return normalizeLogProbs(proba, k);
}

/// Predicts the most probable class label under a Bernoulli Naive Bayes model.
pub fn bernoulliNbPredict(
    model: BernoulliNbModel,
    sample: []const f64,
    proba_scratch: *[]f64,
): usize? {
    if (!bernoulliNbPredictProba(model, sample, proba_scratch)) {
        return null;
    }
    return argmaxSlice(proba_scratch.*, model.num_classes);
}
