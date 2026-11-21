// Source: Absorbed from /data/dev/bun/src/perf/hw_timer.cp | Synced: 2026-11-20T06:08:45Z
// lib/bench/timer.cp
// High-Resolution Monotonic Hardware Timer (Stopwatch) & Benchmark Quantile Statistics

const std = @import("std");

pub struct HwTimer {
    /// Returns monotonic nanoseconds from the OS high-resolution clock (`CLOCK_MONOTONIC`).
    pub static fn nowNs(): u64 {
        let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        zig {
            _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        }
        return ((ts.sec as u64) * 1000000000) + (ts.nsec as u64);
    }

    pub static fn nowUs(): u64 {
        return HwTimer.nowNs() / 1000;
    }

    pub static fn nowMs(): u64 {
        return HwTimer.nowNs() / 1000000;
    }
}

pub struct Stopwatch {
    pub start_ns: u64 = 0;
    pub lap_ns: u64 = 0;

    pub static fn start(): Stopwatch {
        const now = HwTimer.nowNs();
        return Stopwatch {
            start_ns: now,
            lap_ns: now,
        };
    }

    pub fn readNs(): u64 {
        const now = HwTimer.nowNs();
        return if (now >= _.start_ns) (now - _.start_ns) else 0;
    }

    pub fn readUs(): u64 {
        return _.readNs() / 1000;
    }

    pub fn readMs(): u64 {
        return _.readNs() / 1000000;
    }

    pub fn lap(*_): u64 {
        const now = HwTimer.nowNs();
        const delta = if (now >= _.lap_ns) (now - _.lap_ns) else 0;
        _.lap_ns = now;
        return delta;
    }

    pub fn reset(*_) {
        const now = HwTimer.nowNs();
        _.start_ns = now;
        _.lap_ns = now;
    }
}

pub struct BenchStats {
    pub samples: [512]u64 = undefined;
    pub sample_count: usize = 0;
    pub count: usize = 0;
    pub total_ns: u64 = 0;
    pub min_ns: u64 = 0;
    pub max_ns: u64 = 0;
    pub welford_mean: f64 = 0.0;
    pub welford_m2: f64 = 0.0;

    pub static fn init(): BenchStats {
        return BenchStats {
            samples: undefined,
            sample_count: 0,
            count: 0,
            total_ns: 0,
            min_ns: 0,
            max_ns: 0,
            welford_mean: 0.0,
            welford_m2: 0.0,
        };
    }

    pub fn record(*_, sample_ns: u64) {
        _.count += 1;
        _.total_ns += sample_ns;
        if (_.count == 1 || sample_ns < _.min_ns) {
            _.min_ns = sample_ns;
        }
        if (sample_ns > _.max_ns) {
            _.max_ns = sample_ns;
        }

        // Welford's online algorithm for numerical stability of variance / stdDev
        const x = sample_ns as f64;
        const n_f = _.count as f64;
        const delta = x - _.welford_mean;
        _.welford_mean += delta / n_f;
        const delta2 = x - _.welford_mean;
        _.welford_m2 += delta * delta2;

        if (_.sample_count < 512) {
            // Insert in sorted ascending order for O(1) quantile queries
            let pos: usize = _.sample_count;
            while (pos > 0 && _.samples[pos - 1] > sample_ns) {
                _.samples[pos] = _.samples[pos - 1];
                pos -= 1;
            }
            _.samples[pos] = sample_ns;
            _.sample_count += 1;
        }
    }

    pub fn minNs(): u64 {
        return _.min_ns;
    }

    pub fn maxNs(): u64 {
        return _.max_ns;
    }

    pub fn meanNs(): u64 {
        if (_.count == 0) {
            return 0;
        }
        return _.total_ns / (_.count as u64);
    }

    pub fn percentileNs(pct: usize): u64 {
        if (_.sample_count == 0) {
            return 0;
        }
        const clamped = if (pct > 100) (100 as usize) else pct;
        let idx = ((_.sample_count - 1) * clamped) / 100;
        if (idx >= _.sample_count) {
            idx = _.sample_count - 1;
        }
        return _.samples[idx];
    }

    pub fn perMilleNs(permille: usize): u64 {
        if (_.sample_count == 0) {
            return 0;
        }
        const clamped = if (permille > 1000) (1000 as usize) else permille;
        let idx = ((_.sample_count - 1) * clamped) / 1000;
        if (idx >= _.sample_count) {
            idx = _.sample_count - 1;
        }
        return _.samples[idx];
    }

    pub fn medianNs(): u64 {
        return _.percentileNs(50);
    }

    pub fn p50Ns(): u64 {
        return _.percentileNs(50);
    }

    pub fn p75Ns(): u64 {
        return _.percentileNs(75);
    }

    pub fn p90Ns(): u64 {
        return _.percentileNs(90);
    }

    pub fn p95Ns(): u64 {
        return _.percentileNs(95);
    }

    pub fn p99Ns(): u64 {
        return _.percentileNs(99);
    }

    pub fn p999Ns(): u64 {
        return _.perMilleNs(999);
    }

    /// Returns sample standard deviation in nanoseconds as `f64` (Welford's online algorithm).
    pub fn stdDevNsF64(): f64 {
        if (_.count < 2) {
            return 0.0;
        }
        const variance = _.welford_m2 / ((_.count - 1) as f64);
        if (variance <= 0.0) {
            return 0.0;
        }
        return std.math.sqrt(variance);
    }

    /// Returns sample standard deviation in nanoseconds rounded down to `u64`.
    pub fn stdDevNs(): u64 {
        return _.stdDevNsF64() as u64;
    }

    // Ergonomic short-name aliases
    pub fn min(): u64 { return _.minNs(); }
    pub fn max(): u64 { return _.maxNs(); }
    pub fn mean(): u64 { return _.meanNs(); }
    pub fn median(): u64 { return _.medianNs(); }
    pub fn p75(): u64 { return _.p75Ns(); }
    pub fn p90(): u64 { return _.p90Ns(); }
    pub fn p95(): u64 { return _.p95Ns(); }
    pub fn p99(): u64 { return _.p99Ns(); }
    pub fn p999(): u64 { return _.p999Ns(); }
    pub fn stdDev(): u64 { return _.stdDevNs(); }

    /// Computes operations per second based on mean sample duration in nanoseconds.
    pub fn opsPerSec(): u64 {
        const m = _.meanNs();
        if (m == 0) {
            return 0;
        }
        return 1000000000 / m;
    }

    pub fn opsPerSecF64(): f64 {
        if (_.count == 0 || _.total_ns == 0) {
            return 0.0;
        }
        const mean_f = (_.total_ns as f64) / (_.count as f64);
        if (mean_f <= 0.0) {
            return 0.0;
        }
        return 1000000000.0 / mean_f;
    }
}
