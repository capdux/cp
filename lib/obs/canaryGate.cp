// Origin: Pure-CP Stdlib (Zero-Alloc Kayenta/Flagger-Style Canary Statistical Gate & Auto-Rollback Judge) | Created: 2026-11-23
// Pure CP zero-heap-allocation automated canary release statistical judge & rollback gate:
//   - Fixed 16-KPI metric table with direction (`lower_is_better | higher_is_better`), weight,
//     `max_regression_bps`, Welch t-test significance threshold (`z_threshold`), and `critical` veto flag
//   - Two-sample Welch t-test + relative regression joint criterion (`evaluateMetricSample`): filters
//     NaN/Inf, handles zero-variance edge cases, and requires BOTH statistical significance AND
//     regression budget exceedance to flag regression (eliminates small-sample noise false alarms)
//   - Weighted multi-KPI scoring (`evaluateCanary`) outputting `pass_promote | marginal_hold |
//     fail_rollback | insufficient_data`, `score_bps` (0..10000), and per-metric breakdown
//   - Zero-alloc JSON summary formatter (`formatCanaryReportJson`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of canary KPI metrics evaluated inline.
pub const CANARY_MAX_METRICS: usize = 16;

/// Maximum byte length of a metric name.
pub const CANARY_NAME_MAX_LEN: usize = 48;

/// Basis-point scale (10000 bps = 100.00%).
pub const CANARY_BPS_SCALE: u32 = 10_000;

/// Optimization direction for a canary KPI metric.
pub enum CanaryDirection {
    lower_is_better,
    higher_is_better,
}

/// Evaluation status for a single KPI metric.
pub enum CanaryMetricStatus {
    passed,
    regressed,
    insufficient_data,
}

/// Final release verdict returned by `CanaryGateJudge.evaluateCanary`.
pub enum CanaryVerdict {
    pass_promote,
    marginal_hold,
    fail_rollback,
    insufficient_data,
}

/// Configuration specification for one canary KPI metric.
pub struct CanaryMetricSpec {
    active: bool = false;
    name_buf: [CANARY_NAME_MAX_LEN]u8 = [0; CANARY_NAME_MAX_LEN];
    name_len: usize = 0;
    direction: CanaryDirection = CanaryDirection.lower_is_better;
    weight: u32 = 1;
    max_regression_bps: u32 = 500;
    z_threshold: f64 = 1.96;
    critical: bool = false;
    min_samples: usize = 5;

    static fn make(
        name: []const u8,
        direction: CanaryDirection,
        weight: u32,
        max_regression_bps: u32,
        z_threshold: f64,
        critical: bool,
        min_samples: usize,
    ): CanaryMetricSpec {
        let s = CanaryMetricSpec{};
        s.active = true;
        const copy_len = if (name.len > CANARY_NAME_MAX_LEN) CANARY_NAME_MAX_LEN else name.len;
        let i: usize = 0;
        while (i < copy_len) : (i += 1) {
            s.name_buf[i] = name[i];
        }
        s.name_len = copy_len;
        s.direction = direction;
        s.weight = if (weight == 0) 1 else weight;
        s.max_regression_bps = max_regression_bps;
        s.z_threshold = if (!isFiniteF64(z_threshold) or z_threshold < 0.0) 1.96 else z_threshold;
        s.critical = critical;
        s.min_samples = if (min_samples < 2) 2 else min_samples;
        return s;
    }

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }
}

/// Statistical evaluation outcome for a single KPI metric.
pub struct CanaryMetricResult {
    evaluated: bool = false;
    status: CanaryMetricStatus = CanaryMetricStatus.insufficient_data;
    baseline_count: usize = 0;
    canary_count: usize = 0;
    baseline_mean: f64 = 0.0;
    canary_mean: f64 = 0.0;
    baseline_var: f64 = 0.0;
    canary_var: f64 = 0.0;
    welch_t: f64 = 0.0;
    regression_bps: i64 = 0;
    statistically_significant: bool = false;
    exceeds_regression_budget: bool = false;
}

/// Threshold configuration for `CanaryGateJudge.evaluateCanary`.
pub struct CanaryGateConfig {
    pass_score_bps: u32 = 8500;
    marginal_score_bps: u32 = 6500;
    fail_on_insufficient_critical: bool = true;
    min_valid_metrics_bps: u32 = 5000;

    static fn defaults(): CanaryGateConfig {
        return CanaryGateConfig{};
    }
}

/// Final multi-KPI canary judge report.
pub struct CanaryJudgeReport {
    verdict: CanaryVerdict = CanaryVerdict.insufficient_data;
    score_bps: u32 = 0;
    total_metrics: usize = 0;
    passed_metrics: usize = 0;
    regressed_metrics: usize = 0;
    insufficient_metrics: usize = 0;
    critical_failures: usize = 0;
    has_first_failed: bool = false;
    first_failed_index: usize = 0;
    first_failed_name_buf: [CANARY_NAME_MAX_LEN]u8 = [0; CANARY_NAME_MAX_LEN];
    first_failed_name_len: usize = 0;

    pub fn firstFailedNameSlice(): []const u8 {
        return first_failed_name_buf[0..first_failed_name_len];
    }
}

// ============================================================================
// 2. Numerical Hygiene, Welch t-Test & Formatting Helpers
// ============================================================================

fn isFiniteF64(v: f64): bool {
    if (v != v) return false;
    if (v > 1.7e308 or v < -1.7e308) return false;
    return true;
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

struct SampleMomentStats {
    count: usize = 0;
    mean: f64 = 0.0;
    unbiased_var: f64 = 0.0;
}

/// Welford one-pass mean & unbiased sample variance filtering non-finite values.
fn computeFiniteMoments(samples: []const f64): SampleMomentStats {
    let n: usize = 0;
    let mean: f64 = 0.0;
    let m2: f64 = 0.0;

    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        const x = samples[i];
        if (!isFiniteF64(x)) continue;
        n += 1;
        const delta = x - mean;
        mean += delta / @as(f64, @floatFromInt(n));
        const delta2 = x - mean;
        m2 += delta * delta2;
    }

    let uvar: f64 = 0.0;
    if (n >= 2) {
        uvar = m2 / @as(f64, @floatFromInt(n - 1));
        if (uvar < 0.0) uvar = 0.0;
    }

    return SampleMomentStats{
        count: n,
        mean: mean,
        unbiased_var: uvar,
    };
}

/// Evaluate a single KPI metric using two-sample Welch t-test + relative regression joint criterion.
/// Flags `regressed` ONLY when BOTH `welch_t >= spec.z_threshold` AND `regression_bps > max_regression_bps`.
pub fn evaluateMetricSample(
    spec: CanaryMetricSpec,
    baseline_samples: []const f64,
    canary_samples: []const f64,
): CanaryMetricResult {
    const min_n = if (spec.min_samples < 2) @as(usize, 2) else spec.min_samples;
    const b_stats = computeFiniteMoments(baseline_samples);
    const c_stats = computeFiniteMoments(canary_samples);

    let out = CanaryMetricResult{
        evaluated: true,
        status: CanaryMetricStatus.insufficient_data,
        baseline_count: b_stats.count,
        canary_count: c_stats.count,
        baseline_mean: b_stats.mean,
        canary_mean: c_stats.mean,
        baseline_var: b_stats.unbiased_var,
        canary_var: c_stats.unbiased_var,
    };

    if (b_stats.count < min_n or c_stats.count < min_n) {
        return out;
    }

    // Positive `degradation` always means canary is worse than baseline.
    const degradation: f64 = if (spec.direction == CanaryDirection.lower_is_better)
        c_stats.mean - b_stats.mean
    else
        b_stats.mean - c_stats.mean;

    // Relative degradation in basis points (10000 bps = 100%).
    let reg_bps: i64 = 0;
    const base_mag = absF64(b_stats.mean);
    if (base_mag > 0.000000001) {
        let ratio_bps = (degradation / base_mag) * @as(f64, @floatFromInt(CANARY_BPS_SCALE));
        if (ratio_bps > 1_000_000.0) ratio_bps = 1_000_000.0;
        if (ratio_bps < -1_000_000.0) ratio_bps = -1_000_000.0;
        reg_bps = @as(i64, @intFromFloat(ratio_bps));
    } else if (degradation > 0.000000001) {
        reg_bps = 100_000; // Baseline was 0 and canary degraded above 0
    } else if (degradation < -0.000000001) {
        reg_bps = -100_000;
    }

    // Welch standard error: sqrt(s1^2 / n1 + s2^2 / n2)
    const se_sq = b_stats.unbiased_var / @as(f64, @floatFromInt(b_stats.count)) +
        c_stats.unbiased_var / @as(f64, @floatFromInt(c_stats.count));

    let t_stat: f64 = 0.0;
    if (se_sq > 0.000000000000000001) {
        t_stat = degradation / @sqrt(se_sq);
    } else if (degradation > 0.000000001) {
        t_stat = 100.0; // Deterministic zero-variance regression
    } else if (degradation < -0.000000001) {
        t_stat = -100.0;
    }

    const sig = t_stat >= spec.z_threshold;
    const over_budget = reg_bps > @as(i64, @intCast(spec.max_regression_bps));

    out.welch_t = t_stat;
    out.regression_bps = reg_bps;
    out.statistically_significant = sig;
    out.exceeds_regression_budget = over_budget;
    out.status = if (sig and over_budget) CanaryMetricStatus.regressed else CanaryMetricStatus.passed;
    return out;
}

/// Format a `CanaryVerdict` variant as a canonical ASCII string.
pub fn formatCanaryVerdict(verdict: CanaryVerdict): []const u8 {
    if (verdict == CanaryVerdict.pass_promote) return "pass_promote";
    if (verdict == CanaryVerdict.marginal_hold) return "marginal_hold";
    if (verdict == CanaryVerdict.fail_rollback) return "fail_rollback";
    return "insufficient_data";
}

fn appendBytes(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    pos += s.len;
    return true;
}

fn appendJsonEscaped(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"') {
            if (!appendBytes(out_buf, pos, "\\\"")) return false;
        } else if (c == '\\') {
            if (!appendBytes(out_buf, pos, "\\\\")) return false;
        } else if (c < 0x20) {
            if (!appendBytes(out_buf, pos, " ")) return false;
        } else {
            if (pos + 1 > out_buf.len) return false;
            out_buf[pos] = c;
            pos += 1;
        }
    }
    return true;
}

fn appendU64(out_buf: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (n += 1) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v = v / 10;
        }
    }
    if (pos + n > out_buf.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos + i] = tmp[n - 1 - i];
    }
    pos += n;
    return true;
}

/// Zero-alloc JSON summary formatter for `CanaryJudgeReport`.
/// Returns byte count written, or `null` if `out_buf` is too small.
pub fn formatCanaryReportJson(report: CanaryJudgeReport, out_buf: *[]u8): usize? {
    let pos: usize = 0;
    if (!appendBytes(out_buf, *pos, "{\"verdict\":\"")) return null;
    if (!appendBytes(out_buf, *pos, formatCanaryVerdict(report.verdict))) return null;
    if (!appendBytes(out_buf, *pos, "\",\"score_bps\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, report.score_bps))) return null;
    if (!appendBytes(out_buf, *pos, ",\"total_metrics\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, @intCast(report.total_metrics)))) return null;
    if (!appendBytes(out_buf, *pos, ",\"passed_metrics\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, @intCast(report.passed_metrics)))) return null;
    if (!appendBytes(out_buf, *pos, ",\"regressed_metrics\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, @intCast(report.regressed_metrics)))) return null;
    if (!appendBytes(out_buf, *pos, ",\"insufficient_metrics\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, @intCast(report.insufficient_metrics)))) return null;
    if (!appendBytes(out_buf, *pos, ",\"critical_failures\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, @intCast(report.critical_failures)))) return null;
    if (report.has_first_failed) {
        if (!appendBytes(out_buf, *pos, ",\"first_failed\":\"")) return null;
        if (!appendJsonEscaped(out_buf, *pos, report.firstFailedNameSlice())) return null;
        if (!appendBytes(out_buf, *pos, "\"}")) return null;
    } else {
        if (!appendBytes(out_buf, *pos, "}")) return null;
    }
    return pos;
}

// ============================================================================
// 3. CanaryGateJudge — Zero-Heap Multi-KPI Automated Release Gate
// ============================================================================

/// Zero-heap Kayenta/Flagger-style multi-KPI canary statistical gate judge.
pub struct CanaryGateJudge {
    specs: [CANARY_MAX_METRICS]CanaryMetricSpec = [CanaryMetricSpec{}; CANARY_MAX_METRICS];
    results: [CANARY_MAX_METRICS]CanaryMetricResult = [CanaryMetricResult{}; CANARY_MAX_METRICS];
    metric_count: usize = 0;

    static fn init(): CanaryGateJudge {
        return CanaryGateJudge{};
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < CANARY_MAX_METRICS) : (i += 1) {
            specs[i] = CanaryMetricSpec{};
            results[i] = CanaryMetricResult{};
        }
        metric_count = 0;
    }

    /// Register a KPI metric specification and return its slot index (`0..15`), or `null` if full/empty name.
    pub fn addMetric(*_, spec: CanaryMetricSpec): usize? {
        if (spec.name_len == 0 or metric_count >= CANARY_MAX_METRICS) return null;
        const idx = metric_count;
        specs[idx] = spec;
        specs[idx].active = true;
        results[idx] = CanaryMetricResult{};
        metric_count += 1;
        return idx;
    }

    /// Evaluate and record baseline vs canary samples for metric `metric_index`.
    pub fn recordMetricSamples(
        *_,
        metric_index: usize,
        baseline_samples: []const f64,
        canary_samples: []const f64,
    ): CanaryMetricResult? {
        if (metric_index >= metric_count or !specs[metric_index].active) return null;
        const res = evaluateMetricSample(specs[metric_index], baseline_samples, canary_samples);
        results[metric_index] = res;
        return res;
    }

    /// Read the last recorded evaluation result for `metric_index`.
    pub fn getMetricResult(metric_index: usize): CanaryMetricResult? {
        if (metric_index >= metric_count or !specs[metric_index].active) return null;
        return results[metric_index];
    }

    /// Aggregate all registered KPI metrics into a weighted release verdict.
    pub fn evaluateCanary(cfg: CanaryGateConfig): CanaryJudgeReport {
        let rep = CanaryJudgeReport{};
        rep.total_metrics = metric_count;
        if (metric_count == 0) {
            rep.verdict = CanaryVerdict.insufficient_data;
            return rep;
        }

        let evaluated_weight: u64 = 0;
        let passed_weight: u64 = 0;
        let critical_insufficient = false;

        let i: usize = 0;
        while (i < metric_count) : (i += 1) {
            const sp = specs[i];
            const r = results[i];
            const w = @as(u64, sp.weight);

            if (!r.evaluated or r.status == CanaryMetricStatus.insufficient_data) {
                rep.insufficient_metrics += 1;
                if (sp.critical) critical_insufficient = true;
            } else if (r.status == CanaryMetricStatus.passed) {
                rep.passed_metrics += 1;
                evaluated_weight += w;
                passed_weight += w;
            } else {
                rep.regressed_metrics += 1;
                evaluated_weight += w;
                if (sp.critical) {
                    rep.critical_failures += 1;
                }
                if (!rep.has_first_failed or (sp.critical and !specs[rep.first_failed_index].critical)) {
                    rep.has_first_failed = true;
                    rep.first_failed_index = i;
                    rep.first_failed_name_len = sp.name_len;
                    let k: usize = 0;
                    while (k < sp.name_len) : (k += 1) {
                        rep.first_failed_name_buf[k] = sp.name_buf[k];
                    }
                }
            }
        }

        if (evaluated_weight > 0) {
            rep.score_bps = @as(u32, @intCast((passed_weight * @as(u64, CANARY_BPS_SCALE)) / evaluated_weight));
        } else {
            rep.score_bps = 0;
        }

        const valid_metrics = rep.passed_metrics + rep.regressed_metrics;
        const valid_bps = @as(u32, @intCast((valid_metrics * @as(usize, CANARY_BPS_SCALE)) / metric_count));

        // 1. Critical veto takes immediate rollback precedence.
        if (rep.critical_failures > 0) {
            rep.verdict = CanaryVerdict.fail_rollback;
            return rep;
        }

        // 2. Insufficient data guard prevents blind promotion.
        if (valid_metrics == 0 or valid_bps < cfg.min_valid_metrics_bps or (cfg.fail_on_insufficient_critical and critical_insufficient)) {
            rep.verdict = CanaryVerdict.insufficient_data;
            return rep;
        }

        // 3. Weighted score thresholds.
        if (rep.score_bps >= cfg.pass_score_bps) {
            rep.verdict = CanaryVerdict.pass_promote;
        } else if (rep.score_bps >= cfg.marginal_score_bps) {
            rep.verdict = CanaryVerdict.marginal_hold;
        } else {
            rep.verdict = CanaryVerdict.fail_rollback;
        }
        return rep;
    }
}
