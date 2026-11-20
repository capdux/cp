// Source: cp/lib/eval/suite.cp (Evaluation Suite Aggregator, P95 Latency, Regression Gate & Reports)
// Pure CP evaluation report builder with weighted scoring, exact P95 latency, CI regression gating, and Markdown/JSON export.

const std = @import("std");
import { EvalError } from "./metrics.cp";

/// Single evaluated test case record inside an `EvalReport`.
pub struct EvalCase {
    id: string = "",
    category: string = "",
    score: f64 = 0.0,
    passed: bool = false,
    latency_ms: f64 = 0.0,
    tokens_in: u64 = 0,
    tokens_out: u64 = 0,
    cost_usd: f64 = 0.0,
    weight: f64 = 1.0,
}

/// Aggregated summary metrics across all recorded `EvalCase` entries.
pub struct EvalSummary {
    total_cases: usize = 0,
    passed_cases: usize = 0,
    pass_rate: f64 = 0.0,
    weighted_score: f64 = 0.0,
    mean_latency_ms: f64 = 0.0,
    p95_latency_ms: f64 = 0.0,
    total_tokens: u64 = 0,
    total_cost_usd: f64 = 0.0,
}

/// Comparison delta between the current `EvalSummary` and a baseline `EvalSummary`.
pub struct RegressionDiff {
    pass_rate_delta: f64 = 0.0,
    score_delta: f64 = 0.0,
    latency_p95_delta_ms: f64 = 0.0,
    cost_delta_usd: f64 = 0.0,
    regressed: bool = false,
    reason: string = "",
}

fn dupSlice(alloc: std.mem.Allocator, s: string): string ! EvalError {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return EvalError.OutOfMemory;
}

fn freeSlice(alloc: std.mem.Allocator, s: string) {
    if (s.len > 0) {
        alloc.free(s);
    }
}

fn isNanF64(x: f64): bool {
    return x != x;
}

fn sortF64Asc(arr: []f64) {
    if (arr.len < 2) return;
    let i: usize = 1;
    while (i < arr.len) : (i += 1) {
        const key = arr[i];
        let j: usize = i;
        while (j > 0 && arr[j - 1] > key) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

fn appendJsonStr(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: string): void ! EvalError {
    out.append(alloc, '"') catch return EvalError.OutOfMemory;
    let i: usize = 0;
    let flush_start: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\' || c < 0x20) {
            if (i > flush_start) {
                out.appendSlice(alloc, s[flush_start..i]) catch return EvalError.OutOfMemory;
            }
            switch (c) {
                '"' => out.appendSlice(alloc, "\\\"") catch return EvalError.OutOfMemory,
                '\\' => out.appendSlice(alloc, "\\\\") catch return EvalError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return EvalError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return EvalError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return EvalError.OutOfMemory,
                else => {
                    const hex_buf = std.fmt.allocPrint(alloc, "\\u{x:0>4}", .{c}) catch return EvalError.OutOfMemory;
                    defer alloc.free(hex_buf);
                    out.appendSlice(alloc, hex_buf) catch return EvalError.OutOfMemory;
                },
            }
            flush_start = i + 1;
        }
    }
    if (flush_start < s.len) {
        out.appendSlice(alloc, s[flush_start..s.len]) catch return EvalError.OutOfMemory;
    }
    out.append(alloc, '"') catch return EvalError.OutOfMemory;
}

fn appendFmt(out: *std.ArrayList(u8), alloc: std.mem.Allocator, comptime fmt: string, args: auto): void ! EvalError {
    const s = std.fmt.allocPrint(alloc, fmt, args) catch return EvalError.OutOfMemory;
    defer alloc.free(s);
    out.appendSlice(alloc, s) catch return EvalError.OutOfMemory;
}

/// Evaluation suite report collector supporting weighted aggregation, P95 latency, CI gating, and Markdown/JSON export.
pub struct EvalReport {
    alloc: std.mem.Allocator = undefined,
    suite_name: string = "",
    cases: std.ArrayList(EvalCase) = .empty,

    /// Initializes an `EvalReport` owning its `suite_name` and recorded cases.
    static fn init(alloc: std.mem.Allocator, suite_name: string): EvalReport ! EvalError {
        const owned_name = dupSlice(alloc, suite_name)?;
        return EvalReport{
            alloc: alloc,
            suite_name: owned_name,
            cases: .empty,
        };
    }

    /// Releases all owned strings and internal case storage.
    fn deinit(*_) {
        let i: usize = 0;
        while (i < _.cases.items.len) : (i += 1) {
            freeSlice(_.alloc, _.cases.items[i].id);
            freeSlice(_.alloc, _.cases.items[i].category);
        }
        _.cases.deinit(_.alloc);
        freeSlice(_.alloc, _.suite_name);
        _.suite_name = "";
    }

    /// Records a new `EvalCase` into the suite report (cloning `id` and `category`).
    fn recordCase(*_, c: EvalCase): void ! EvalError {
        if (isNanF64(c.score) || isNanF64(c.latency_ms) || isNanF64(c.cost_usd) || isNanF64(c.weight)) {
            return EvalError.InvalidInput;
        }
        if (c.latency_ms < 0.0 || c.weight < 0.0) {
            return EvalError.InvalidInput;
        }
        const owned_id = dupSlice(_.alloc, c.id)?;
        errdefer freeSlice(_.alloc, owned_id);
        const owned_cat = dupSlice(_.alloc, c.category)?;
        errdefer freeSlice(_.alloc, owned_cat);

        const stored = EvalCase{
            id: owned_id,
            category: owned_cat,
            score: c.score,
            passed: c.passed,
            latency_ms: c.latency_ms,
            tokens_in: c.tokens_in,
            tokens_out: c.tokens_out,
            cost_usd: c.cost_usd,
            weight: c.weight,
        };
        _.cases.append(_.alloc, stored) catch return EvalError.OutOfMemory;
    }

    /// Computes aggregate statistics including weighted score, mean latency, exact P95 latency, total tokens, and cost.
    fn summarize(): EvalSummary ! EvalError {
        const n = _.cases.items.len;
        if (n == 0) {
            return EvalError.EmptyDataset;
        }

        let latencies = _.alloc.alloc(f64, n) catch return EvalError.OutOfMemory;
        defer _.alloc.free(latencies);

        let passed_cases: usize = 0;
        let weighted_sum: f64 = 0.0;
        let weight_total: f64 = 0.0;
        let latency_sum: f64 = 0.0;
        let total_tokens: u64 = 0;
        let total_cost: f64 = 0.0;

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const c = _.cases.items[i];
            if (c.passed) passed_cases += 1;
            weighted_sum += c.score * c.weight;
            weight_total += c.weight;
            latency_sum += c.latency_ms;
            latencies[i] = c.latency_ms;
            total_tokens += c.tokens_in + c.tokens_out;
            total_cost += c.cost_usd;
        }

        if (weight_total <= 0.0) {
            return EvalError.DivisionByZero;
        }

        sortF64Asc(latencies);
        let p95_idx = ((95 * n + 99) / 100);
        if (p95_idx > 0) p95_idx -= 1;
        if (p95_idx >= n) p95_idx = n - 1;

        return EvalSummary{
            total_cases: n,
            passed_cases: passed_cases,
            pass_rate: (passed_cases as f64) / (n as f64),
            weighted_score: weighted_sum / weight_total,
            mean_latency_ms: latency_sum / (n as f64),
            p95_latency_ms: latencies[p95_idx],
            total_tokens: total_tokens,
            total_cost_usd: total_cost,
        };
    }

    /// Compares current suite summary against `baseline_summary`, flagging regressions exceeding `max_score_drop` or `max_p95_increase_ms`.
    fn compareBaseline(
        baseline_summary: EvalSummary,
        max_score_drop: f64,
        max_p95_increase_ms: f64,
    ): RegressionDiff ! EvalError {
        if (isNanF64(max_score_drop) || isNanF64(max_p95_increase_ms) || max_score_drop < 0.0 || max_p95_increase_ms < 0.0) {
            return EvalError.InvalidThreshold;
        }
        const cur = _.summarize()?;
        const score_drop = baseline_summary.weighted_score - cur.weighted_score;
        const p95_increase = cur.p95_latency_ms - baseline_summary.p95_latency_ms;

        const score_bad = score_drop > max_score_drop;
        const lat_bad = p95_increase > max_p95_increase_ms;

        let reason: string = "ok";
        if (score_bad && lat_bad) {
            reason = "weighted_score_drop+p95_latency_increase";
        } else if (score_bad) {
            reason = "weighted_score_drop";
        } else if (lat_bad) {
            reason = "p95_latency_increase";
        }

        return RegressionDiff{
            pass_rate_delta: cur.pass_rate - baseline_summary.pass_rate,
            score_delta: cur.weighted_score - baseline_summary.weighted_score,
            latency_p95_delta_ms: p95_increase,
            cost_delta_usd: cur.total_cost_usd - baseline_summary.total_cost_usd,
            regressed: score_bad || lat_bad,
            reason: reason,
        };
    }

    /// Exports a Markdown summary table and per-case breakdown. Caller owns the returned slice and must free it with `alloc.free`.
    fn toMarkdown(): string ! EvalError {
        const summary = _.summarize()?;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.alloc);

        appendFmt(*out, _.alloc, "# Eval Report: {s}\n\n", .{_.suite_name})?;
        appendFmt(*out, _.alloc, "| Metric | Value |\n| :--- | :--- |\n", .{})?;
        appendFmt(*out, _.alloc, "| Total Cases | {d} |\n", .{summary.total_cases})?;
        appendFmt(*out, _.alloc, "| Passed Cases | {d} |\n", .{summary.passed_cases})?;
        appendFmt(*out, _.alloc, "| Pass Rate | {d:.2}% |\n", .{summary.pass_rate * 100.0})?;
        appendFmt(*out, _.alloc, "| Weighted Score | {d:.4} |\n", .{summary.weighted_score})?;
        appendFmt(*out, _.alloc, "| Mean Latency (ms) | {d:.2} |\n", .{summary.mean_latency_ms})?;
        appendFmt(*out, _.alloc, "| P95 Latency (ms) | {d:.2} |\n", .{summary.p95_latency_ms})?;
        appendFmt(*out, _.alloc, "| Total Tokens | {d} |\n", .{summary.total_tokens})?;
        appendFmt(*out, _.alloc, "| Total Cost (USD) | {d:.6} |\n\n", .{summary.total_cost_usd})?;

        appendFmt(*out, _.alloc, "| ID | Category | Passed | Score | Latency (ms) | Tokens | Cost (USD) |\n", .{})?;
        appendFmt(*out, _.alloc, "| :--- | :--- | :---: | :---: | :---: | :---: | :---: |\n", .{})?;
        let i: usize = 0;
        while (i < _.cases.items.len) : (i += 1) {
            const c = _.cases.items[i];
            const pass_label: string = if (c.passed) "PASS" else "FAIL";
            const toks = c.tokens_in + c.tokens_out;
            appendFmt(
                *out,
                _.alloc,
                "| {s} | {s} | {s} | {d:.4} | {d:.2} | {d} | {d:.6} |\n",
                .{ c.id, c.category, pass_label, c.score, c.latency_ms, toks, c.cost_usd },
            )?;
        }

        return out.toOwnedSlice(_.alloc) catch return EvalError.OutOfMemory;
    }

    /// Exports an RFC 8259 compliant JSON evaluation report. Caller owns the returned slice and must free it with `alloc.free`.
    fn toJson(): string ! EvalError {
        const summary = _.summarize()?;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.alloc);

        out.appendSlice(_.alloc, "{\"suite_name\":") catch return EvalError.OutOfMemory;
        appendJsonStr(*out, _.alloc, _.suite_name)?;
        appendFmt(
            *out,
            _.alloc,
            ",\"summary\":{{\"total_cases\":{d},\"passed_cases\":{d},\"pass_rate\":{d:.6},\"weighted_score\":{d:.6},\"mean_latency_ms\":{d:.4},\"p95_latency_ms\":{d:.4},\"total_tokens\":{d},\"total_cost_usd\":{d:.6}}},\"cases\":[",
            .{
                summary.total_cases,
                summary.passed_cases,
                summary.pass_rate,
                summary.weighted_score,
                summary.mean_latency_ms,
                summary.p95_latency_ms,
                summary.total_tokens,
                summary.total_cost_usd,
            },
        )?;

        let i: usize = 0;
        while (i < _.cases.items.len) : (i += 1) {
            if (i > 0) {
                out.append(_.alloc, ',') catch return EvalError.OutOfMemory;
            }
            const c = _.cases.items[i];
            out.appendSlice(_.alloc, "{\"id\":") catch return EvalError.OutOfMemory;
            appendJsonStr(*out, _.alloc, c.id)?;
            out.appendSlice(_.alloc, ",\"category\":") catch return EvalError.OutOfMemory;
            appendJsonStr(*out, _.alloc, c.category)?;
            const passed_str: string = if (c.passed) "true" else "false";
            appendFmt(
                *out,
                _.alloc,
                ",\"score\":{d:.6},\"passed\":{s},\"latency_ms\":{d:.4},\"tokens_in\":{d},\"tokens_out\":{d},\"cost_usd\":{d:.6},\"weight\":{d:.4}}}",
                .{ c.score, passed_str, c.latency_ms, c.tokens_in, c.tokens_out, c.cost_usd, c.weight },
            )?;
        }

        out.appendSlice(_.alloc, "]}") catch return EvalError.OutOfMemory;
        return out.toOwnedSlice(_.alloc) catch return EvalError.OutOfMemory;
    }
}
