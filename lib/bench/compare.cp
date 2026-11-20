// Source: cp/lib/bench/compare.cp (Welch t-test & Regression Comparator)
// Welch's Two-Sample t-Test Comparison, Regression Detection & Markdown Report Formatter

const std = @import("std");
import { BenchResult } from "./suite.cp";

pub enum BenchVerdict: u8 {
    Faster,
    Slower,
    NoChange,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .Faster => "Faster",
            .Slower => "Slower",
            .NoChange => "NoChange",
        };
    }

    pub fn toString(): string {
        return _.asStr();
    }
}

/// Two-tailed Student's t critical values at alpha = 0.05 (95% confidence) for df = 1..30.
const T_CRIT_95_TABLE: [30]f64 = [
    12.7062, 4.3027, 3.1824, 2.7764, 2.5706,
    2.4469,  2.3646, 2.3060, 2.2622, 2.2281,
    2.2010,  2.1788, 2.1604, 2.1448, 2.1314,
    2.1199,  2.1098, 2.1009, 2.0930, 2.0860,
    2.0796,  2.0739, 2.0687, 2.0639, 2.0595,
    2.0555,  2.0518, 2.0484, 2.0452, 2.0423,
];

fn criticalT95(df: f64): f64 {
    if (df <= 1.0) {
        return T_CRIT_95_TABLE[0];
    }
    if (df <= 30.0) {
        const lower = df as usize;
        const idx = if (lower >= 1) (lower - 1) else 0;
        if (idx >= 29) {
            return T_CRIT_95_TABLE[29];
        }
        const frac = df - (lower as f64);
        return T_CRIT_95_TABLE[idx] + frac * (T_CRIT_95_TABLE[idx + 1] - T_CRIT_95_TABLE[idx]);
    }
    // Asymptotic Cornish-Fisher expansion around z_0.975 = 1.959964 for df > 30
    const z: f64 = 1.95996398454;
    const z3 = z * z * z;
    const z5 = z3 * z * z;
    return z + (z3 + z) / (4.0 * df) + (5.0 * z5 + 16.0 * z3 + 3.0 * z) / (96.0 * df * df);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

/// Result of comparing a candidate benchmark against a baseline via Welch's t-test.
pub struct BenchComparison {
    pub baseline_name: []const u8 = "";
    pub candidate_name: []const u8 = "";
    pub baseline_mean_ns: f64 = 0.0;
    pub candidate_mean_ns: f64 = 0.0;
    pub delta_ns: f64 = 0.0;
    pub change_pct: f64 = 0.0;
    pub speedup: f64 = 1.0;
    pub t_statistic: f64 = 0.0;
    pub degrees_of_freedom: f64 = 0.0;
    pub is_significant: bool = false;
    pub verdict: BenchVerdict = BenchVerdict.NoChange;

    /// Compares `candidate` against `baseline` using Welch's unequal-variance t-test (alpha = 0.05)
    /// combined with a practical magnitude threshold `threshold_pct` (e.g., 5.0 for 5%).
    pub static fn compare(
        baseline: BenchResult,
        candidate: BenchResult,
        threshold_pct: f64
    ): BenchComparison {
        const n1 = baseline.effectiveSampleCount();
        const n2 = candidate.effectiveSampleCount();
        const m1 = baseline.mean_ns;
        const m2 = candidate.mean_ns;
        const delta = m2 - m1;
        const change_pct = if (m1 > 0.0) ((delta / m1) * 100.0) else 0.0;
        const speedup = if (m1 > 0.0 && m2 > 0.0) (m1 / m2) else 1.0;

        let t_stat: f64 = 0.0;
        let df: f64 = 0.0;
        let significant: bool = false;

        if (n1 >= 2 && n2 >= 2) {
            const v1 = (baseline.stddev_ns * baseline.stddev_ns) / (n1 as f64);
            const v2 = (candidate.stddev_ns * candidate.stddev_ns) / (n2 as f64);
            const se_sq = v1 + v2;

            if (se_sq > 0.0) {
                const se = std.math.sqrt(se_sq);
                t_stat = (m2 - m1) / se;
                const denom = ((v1 * v1) / ((n1 - 1) as f64)) + ((v2 * v2) / ((n2 - 1) as f64));
                df = if (denom > 0.0) ((se_sq * se_sq) / denom) else ((n1 + n2 - 2) as f64);
                const t_crit = criticalT95(df);
                significant = absF64(t_stat) >= t_crit;
            } else if (m1 != m2) {
                // Zero-variance deterministic distributions with distinct means.
                df = (n1 + n2 - 2) as f64;
                t_stat = if (m2 < m1) -1000000.0 else 1000000.0;
                significant = true;
            }
        }

        const eff_threshold = if (threshold_pct < 0.0) 0.0 else threshold_pct;
        let verdict = BenchVerdict.NoChange;
        if (significant && absF64(change_pct) >= eff_threshold) {
            if (change_pct < 0.0) {
                verdict = BenchVerdict.Faster;
            } else if (change_pct > 0.0) {
                verdict = BenchVerdict.Slower;
            }
        }

        return BenchComparison {
            baseline_name: baseline.name,
            candidate_name: candidate.name,
            baseline_mean_ns: m1,
            candidate_mean_ns: m2,
            delta_ns: delta,
            change_pct: change_pct,
            speedup: speedup,
            t_statistic: t_stat,
            degrees_of_freedom: df,
            is_significant: significant,
            verdict: verdict,
        };
    }
}

/// Formatter rendering `BenchResult` slices and `BenchComparison` reports as Markdown tables.
pub struct BenchTableFormatter {
    /// Formats a slice of `BenchResult` records into an owned Markdown summary table.
    pub static fn formatMarkdown(alloc: std.mem.Allocator, results: []const BenchResult): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(
            alloc,
            "| Benchmark | Samples | Min (ns) | Mean (ns) | StdDev (ns) | p50 (ns) | p95 (ns) | p99 (ns) | Max (ns) | Ops/sec |\n"
        )?;
        out.appendSlice(
            alloc,
            "| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n"
        )?;

        let i: usize = 0;
        while (i < results.len) {
            const r = results[i];
            const row = std.fmt.allocPrint(
                alloc,
                "| {s} | {d} | {d} | {d:.2} | {d:.2} | {d} | {d} | {d} | {d} | {d:.2} |\n",
                .{
                    r.name,
                    r.effectiveSampleCount(),
                    r.min_ns,
                    r.mean_ns,
                    r.stddev_ns,
                    r.p50_ns,
                    r.p95_ns,
                    r.p99_ns,
                    r.max_ns,
                    r.ops_per_sec,
                },
            )?;
            defer alloc.free(row);
            out.appendSlice(alloc, row)?;
            i += 1;
        }

        return out.toOwnedSlice(alloc);
    }

    /// Formats a `BenchComparison` result into an owned Markdown comparison table.
    pub static fn formatComparison(alloc: std.mem.Allocator, cmp: BenchComparison): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(
            alloc,
            "| Baseline | Candidate | Baseline Mean (ns) | Candidate Mean (ns) | Change (%) | Speedup | Welch t | df | Significant | Verdict |\n"
        )?;
        out.appendSlice(
            alloc,
            "| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | :---: | :--- |\n"
        )?;

        const sig_str = if (cmp.is_significant) "yes" else "no";
        const sign_str = if (cmp.change_pct > 0.0) "+" else "";
        const row = std.fmt.allocPrint(
            alloc,
            "| {s} | {s} | {d:.2} | {d:.2} | {s}{d:.2}% | {d:.2}x | {d:.3} | {d:.1} | {s} | {s} |\n",
            .{
                cmp.baseline_name,
                cmp.candidate_name,
                cmp.baseline_mean_ns,
                cmp.candidate_mean_ns,
                sign_str,
                cmp.change_pct,
                cmp.speedup,
                cmp.t_statistic,
                cmp.degrees_of_freedom,
                sig_str,
                cmp.verdict.asStr(),
            },
        )?;
        defer alloc.free(row);
        out.appendSlice(alloc, row)?;

        return out.toOwnedSlice(alloc);
    }
}
