// Source: /data/dev/bun/src/perf/perf.cp
// lib/bench/suite.cp
// Multi-Case Benchmark Runner with Warmup, Min-Duration Loops, Markdown & JSON Export

const std = @import("std");
import { HwTimer, BenchStats } from "./timer.cp";

pub struct BenchCaseResult {
    pub name: []const u8 = "";
    pub stats: BenchStats = undefined;
    pub iterations: u64 = 0;
}

pub struct BenchSuite {
    pub name: []const u8 = "Benchmark Suite";
    pub warmup_iters: usize = 5;
    pub min_iters: usize = 10;
    pub max_iters: usize = 100000;
    pub min_time_ns: u64 = 1000000;
    pub cases: [32]BenchCaseResult = undefined;
    pub case_count: usize = 0;

    pub static fn init(
        name: []const u8 = "Benchmark Suite",
        warmup_iters: usize = 5,
        min_iters: usize = 10,
        min_time_ns: u64 = 1000000,
        max_iters: usize = 100000
    ): BenchSuite {
        return BenchSuite {
            name: name,
            warmup_iters: warmup_iters,
            min_iters: if (min_iters == 0) 1 else min_iters,
            max_iters: if (max_iters == 0) 1 else max_iters,
            min_time_ns: min_time_ns,
            cases: undefined,
            case_count: 0,
        };
    }

    /// Runs `func` with warmup and adaptive iteration count, recording latency statistics.
    pub fn run(*_, case_name: []const u8, @comptime func: auto) {
        let w: usize = 0;
        while (w < _.warmup_iters) {
            func();
            w += 1;
        }

        let stats = BenchStats.init();
        let iters: usize = 0;
        const suite_start = HwTimer.nowNs();
        while (iters < _.max_iters) {
            const t0 = HwTimer.nowNs();
            func();
            const t1 = HwTimer.nowNs();
            const dt = if (t1 >= t0) (t1 - t0) else 0;
            stats.record(dt);
            iters += 1;
            const total_elapsed = if (t1 >= suite_start) (t1 - suite_start) else 0;
            if (iters >= _.min_iters && total_elapsed >= _.min_time_ns) {
                break;
            }
        }
        _.recordCase(case_name, stats);
    }

    /// Registers a pre-collected `BenchStats` case into the suite.
    pub fn recordCase(*_, case_name: []const u8, stats: BenchStats) {
        if (_.case_count >= 32) {
            return;
        }
        _.cases[_.case_count] = BenchCaseResult {
            name: case_name,
            stats: stats,
            iterations: stats.count as u64,
        };
        _.case_count += 1;
    }

    pub fn caseCount(): usize {
        return _.case_count;
    }

    pub fn getCase(index: usize): BenchCaseResult? {
        if (index >= _.case_count) {
            return null;
        }
        return _.cases[index];
    }

    fn writeBytes(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    /// Exports a Markdown comparison table for all recorded benchmark cases.
    pub fn toMarkdown(out_buf: []u8): []const u8 {
        let p: usize = 0;
        p = BenchSuite.writeBytes(out_buf, p, "### ");
        p = BenchSuite.writeBytes(out_buf, p, _.name);
        p = BenchSuite.writeBytes(out_buf, p, "\n\n| Benchmark | Iters | Mean (ns) | p50 (ns) | p95 (ns) | p99 (ns) | Ops/sec |\n| :--- | ---: | ---: | ---: | ---: | ---: | ---: |\n");

        let i: usize = 0;
        while (i < _.case_count) {
            const c = _.cases[i];
            const row = std.fmt.bufPrint(
                out_buf[p..],
                "| {s} | {d} | {d} | {d} | {d} | {d} | {d} |\n",
                .{
                    c.name,
                    c.iterations,
                    c.stats.meanNs(),
                    c.stats.p50Ns(),
                    c.stats.p95Ns(),
                    c.stats.p99Ns(),
                    c.stats.opsPerSec(),
                },
            ) catch "";
            p += row.len;
            i += 1;
        }
        return out_buf[0..p];
    }

    /// Exports full quantile and throughput metrics for all recorded benchmark cases as JSON.
    pub fn toJson(out_buf: []u8): []const u8 {
        let p: usize = 0;
        p = BenchSuite.writeBytes(out_buf, p, "{\"suite\":\"");
        p = BenchSuite.writeBytes(out_buf, p, _.name);
        p = BenchSuite.writeBytes(out_buf, p, "\",\"cases\":[");

        let i: usize = 0;
        while (i < _.case_count) {
            if (i > 0) {
                p = BenchSuite.writeBytes(out_buf, p, ",");
            }
            const c = _.cases[i];
            const entry = std.fmt.bufPrint(
                out_buf[p..],
                "{{\"name\":\"{s}\",\"iterations\":{d},\"min_ns\":{d},\"max_ns\":{d},\"mean_ns\":{d},\"p50_ns\":{d},\"p75_ns\":{d},\"p90_ns\":{d},\"p95_ns\":{d},\"p99_ns\":{d},\"p999_ns\":{d},\"std_dev_ns\":{d},\"ops_per_sec\":{d}}}",
                .{
                    c.name,
                    c.iterations,
                    c.stats.minNs(),
                    c.stats.maxNs(),
                    c.stats.meanNs(),
                    c.stats.p50Ns(),
                    c.stats.p75Ns(),
                    c.stats.p90Ns(),
                    c.stats.p95Ns(),
                    c.stats.p99Ns(),
                    c.stats.p999Ns(),
                    c.stats.stdDevNs(),
                    c.stats.opsPerSec(),
                },
            ) catch "";
            p += entry.len;
            i += 1;
        }
        p = BenchSuite.writeBytes(out_buf, p, "]}");
        return out_buf[0..p];
    }
}
