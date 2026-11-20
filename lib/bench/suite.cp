// Source: cp/lib/bench/suite.cp (Microbenchmark Runner & BlackBox Barrier)
// Multi-Case Benchmark Runner with Warmup, Min-Duration Loops, BlackBox, Quantiles, Markdown & JSON Export

const std = @import("std");
import { HwTimer, BenchStats } from "./timer.cp";

/// Optimization barrier that prevents the compiler from dead-code eliminating `val`.
pub fn blackBox<T>(val: T): T {
    std.mem.doNotOptimizeAway(val);
    return val;
}

/// Configuration for `BenchRunner.measure`.
pub struct BenchOptions {
    pub warmup_iters: usize = 5;
    pub sample_count: usize = 50;
    pub min_sample_ns: u64 = 1000000;

    pub static fn default(): BenchOptions {
        return BenchOptions {
            warmup_iters: 5,
            sample_count: 50,
            min_sample_ns: 1000000,
        };
    }

    pub static fn init(
        warmup_iters: usize = 5,
        sample_count: usize = 50,
        min_sample_ns: u64 = 1000000
    ): BenchOptions {
        return BenchOptions {
            warmup_iters: warmup_iters,
            sample_count: sample_count,
            min_sample_ns: min_sample_ns,
        };
    }
}

/// Complete microbenchmark statistical summary with quantiles and throughput.
pub struct BenchResult {
    pub name: []const u8 = "";
    pub samples: []const u64 = [];
    pub sample_count: usize = 0;
    pub min_ns: u64 = 0;
    pub max_ns: u64 = 0;
    pub mean_ns: f64 = 0.0;
    pub stddev_ns: f64 = 0.0;
    pub p50_ns: u64 = 0;
    pub p95_ns: u64 = 0;
    pub p99_ns: u64 = 0;
    pub ops_per_sec: f64 = 0.0;

    pub fn effectiveSampleCount(): usize {
        if (_.sample_count > 0) {
            return _.sample_count;
        }
        return _.samples.len;
    }
}

const BENCH_RING_SLOTS: usize = 32;
const BENCH_RING_CAP: usize = 512;

@tls let g_bench_sample_ring: [BENCH_RING_SLOTS][BENCH_RING_CAP]u64 = undefined;
@tls let g_bench_ring_cursor: usize = 0;

fn sortU64Asc(buf: []u64) {
    if (buf.len <= 1) {
        return;
    }
    // Heapsort: O(N log N) worst-case, zero extra allocations.
    let i: usize = buf.len / 2;
    while (i > 0) {
        i -= 1;
        siftDown(buf, i, buf.len);
    }
    let end: usize = buf.len;
    while (end > 1) {
        end -= 1;
        const tmp = buf[0];
        buf[0] = buf[end];
        buf[end] = tmp;
        siftDown(buf, 0, end);
    }
}

fn siftDown(buf: []u64, start: usize, end: usize) {
    let root = start;
    while (true) {
        const left = root * 2 + 1;
        if (left >= end) {
            break;
        }
        let swap_idx = root;
        if (buf[swap_idx] < buf[left]) {
            swap_idx = left;
        }
        const right = left + 1;
        if (right < end && buf[swap_idx] < buf[right]) {
            swap_idx = right;
        }
        if (swap_idx == root) {
            break;
        }
        const tmp = buf[root];
        buf[root] = buf[swap_idx];
        buf[swap_idx] = tmp;
        root = swap_idx;
    }
}

fn quantileFromSorted(sorted: []const u64, pct: usize): u64 {
    if (sorted.len == 0) {
        return 0;
    }
    const clamped = if (pct > 100) (100 as usize) else pct;
    let idx = ((sorted.len - 1) * clamped) / 100;
    if (idx >= sorted.len) {
        idx = sorted.len - 1;
    }
    return sorted[idx];
}

fn computeResultFromSlice(
    name: []const u8,
    samples_slice: []const u64,
    total_count: usize,
    mean_override: f64?,
    stddev_override: f64?
): BenchResult {
    if (samples_slice.len == 0) {
        return BenchResult {
            name: name,
            samples: samples_slice,
            sample_count: total_count,
            min_ns: 0,
            max_ns: 0,
            mean_ns: 0.0,
            stddev_ns: 0.0,
            p50_ns: 0,
            p95_ns: 0,
            p99_ns: 0,
            ops_per_sec: 0.0,
        };
    }

    let min_v: u64 = samples_slice[0];
    let max_v: u64 = samples_slice[0];
    let w_mean: f64 = 0.0;
    let w_m2: f64 = 0.0;

    let i: usize = 0;
    while (i < samples_slice.len) {
        const s = samples_slice[i];
        if (s < min_v) { min_v = s; }
        if (s > max_v) { max_v = s; }
        const x = s as f64;
        const nf = (i + 1) as f64;
        const delta = x - w_mean;
        w_mean += delta / nf;
        const delta2 = x - w_mean;
        w_m2 += delta * delta2;
        i += 1;
    }

    const final_mean = mean_override ?? w_mean;
    let final_stddev: f64 = 0.0;
    if (ov := stddev_override) {
        final_stddev = ov;
    } else if (samples_slice.len >= 2) {
        const variance = w_m2 / ((samples_slice.len - 1) as f64);
        final_stddev = if (variance > 0.0) std.math.sqrt(variance) else 0.0;
    }

    // Sort a scratch copy for exact p50 / p95 / p99 quantiles without mutating input.
    let stack_buf: [1024]u64 = undefined;
    let p50: u64 = 0;
    let p95: u64 = 0;
    let p99: u64 = 0;

    if (samples_slice.len <= 1024) {
        let j: usize = 0;
        while (j < samples_slice.len) {
            stack_buf[j] = samples_slice[j];
            j += 1;
        }
        const work = stack_buf[0..samples_slice.len];
        sortU64Asc(work);
        p50 = quantileFromSorted(work, 50);
        p95 = quantileFromSorted(work, 95);
        p99 = quantileFromSorted(work, 99);
    } else {
        const alloc = std.heap.page_allocator;
        const opt_buf = alloc.alloc(u64, samples_slice.len) catch null;
        if (hb := opt_buf) {
            let heap_buf = hb;
            defer alloc.free(heap_buf);
            let j: usize = 0;
            while (j < samples_slice.len) {
                heap_buf[j] = samples_slice[j];
                j += 1;
            }
            sortU64Asc(heap_buf);
            p50 = quantileFromSorted(heap_buf, 50);
            p95 = quantileFromSorted(heap_buf, 95);
            p99 = quantileFromSorted(heap_buf, 99);
        } else {
            p50 = min_v;
            p95 = max_v;
            p99 = max_v;
        }
    }

    const ops = if (final_mean > 0.0) (1000000000.0 / final_mean) else 0.0;

    return BenchResult {
        name: name,
        samples: samples_slice,
        sample_count: if (total_count > 0) total_count else samples_slice.len,
        min_ns: min_v,
        max_ns: max_v,
        mean_ns: final_mean,
        stddev_ns: final_stddev,
        p50_ns: p50,
        p95_ns: p95,
        p99_ns: p99,
        ops_per_sec: ops,
    };
}

/// Microbenchmark runner supporting deterministic sample injection and calibrated live measurement.
pub struct BenchRunner {
    /// Computes complete statistics and quantiles from externally injected nanosecond samples.
    pub static fn runSamples(name: []const u8, durations_ns: []const u64): BenchResult {
        return computeResultFromSlice(name, durations_ns, durations_ns.len, null, null);
    }

    /// Runs `fn_ptr` with warmup and calibrated batch sizing, collecting `opts.sample_count` samples.
    pub static fn measure(name: []const u8, opts: BenchOptions, fn_ptr: auto): BenchResult {
        let w: usize = 0;
        while (w < opts.warmup_iters) {
            fn_ptr();
            w += 1;
        }

        if (opts.sample_count == 0) {
            return BenchResult {
                name: name,
                samples: [],
                sample_count: 0,
            };
        }

        let iters_per_sample: usize = 1;
        if (opts.min_sample_ns > 0) {
            while (true) {
                const c0 = HwTimer.nowNs();
                let k: usize = 0;
                while (k < iters_per_sample) {
                    fn_ptr();
                    k += 1;
                }
                const c1 = HwTimer.nowNs();
                const dt = if (c1 >= c0) (c1 - c0) else 0;
                if (dt >= opts.min_sample_ns || iters_per_sample >= 1048576) {
                    break;
                }
                iters_per_sample *= 2;
            }
        }

        const slot_idx = g_bench_ring_cursor % BENCH_RING_SLOTS;
        g_bench_ring_cursor += 1;
        const stored_cap = if (opts.sample_count < BENCH_RING_CAP) opts.sample_count else BENCH_RING_CAP;

        let w_mean: f64 = 0.0;
        let w_m2: f64 = 0.0;
        let s_idx: usize = 0;
        while (s_idx < opts.sample_count) {
            const t0 = HwTimer.nowNs();
            let k: usize = 0;
            while (k < iters_per_sample) {
                fn_ptr();
                k += 1;
            }
            const t1 = HwTimer.nowNs();
            const total_dt = if (t1 >= t0) (t1 - t0) else 0;
            const per_iter_f = (total_dt as f64) / (iters_per_sample as f64);
            const per_iter_u = total_dt / (iters_per_sample as u64);

            const nf = (s_idx + 1) as f64;
            const delta = per_iter_f - w_mean;
            w_mean += delta / nf;
            const delta2 = per_iter_f - w_mean;
            w_m2 += delta * delta2;

            if (s_idx < stored_cap) {
                g_bench_sample_ring[slot_idx][s_idx] = per_iter_u;
            }
            s_idx += 1;
        }

        let final_stddev: f64 = 0.0;
        if (opts.sample_count >= 2) {
            const variance = w_m2 / ((opts.sample_count - 1) as f64);
            final_stddev = if (variance > 0.0) std.math.sqrt(variance) else 0.0;
        }

        const slice = g_bench_sample_ring[slot_idx][0..stored_cap];
        return computeResultFromSlice(name, slice, opts.sample_count, w_mean, final_stddev);
    }
}

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
