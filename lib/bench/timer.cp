// lib/bench/timer.cp
// High-Resolution Monotonic Hardware Timer (Stopwatch) & Benchmark Percentile Statistics
// Absorbed from Bun src/perf/hw_timer.cp, system_timer.cp & perf.cp

const std = @import("std");

pub struct HwTimer {
    /// Returns monotonic nanoseconds from the OS high-resolution clock (`CLOCK_MONOTONIC`).
    pub static fn nowNs(): u64 {
        let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        __zig__ {
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
    start_ns: u64 = 0;
    lap_ns: u64 = 0;

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
    samples: [128]u64 = undefined;
    count: usize = 0;
    total_ns: u64 = 0;
    min_ns: u64 = 0;
    max_ns: u64 = 0;

    pub static fn init(): BenchStats {
        return BenchStats {
            samples: undefined,
            count: 0,
            total_ns: 0,
            min_ns: 0,
            max_ns: 0,
        };
    }

    pub fn record(*_, sample_ns: u64) {
        _.total_ns += sample_ns;
        if (_.count == 0 || sample_ns < _.min_ns) {
            _.min_ns = sample_ns;
        }
        if (sample_ns > _.max_ns) {
            _.max_ns = sample_ns;
        }
        if (_.count < 128) {
            // Insert in sorted ascending order for O(1) percentile queries
            let pos: usize = _.count;
            while (pos > 0 && _.samples[pos - 1] > sample_ns) {
                _.samples[pos] = _.samples[pos - 1];
                pos -= 1;
            }
            _.samples[pos] = sample_ns;
            _.count += 1;
        }
    }

    pub fn meanNs(): u64 {
        if (_.count == 0) {
            return 0;
        }
        return _.total_ns / (_.count as u64);
    }

    pub fn percentileNs(pct: usize): u64 {
        if (_.count == 0) {
            return 0;
        }
        const clamped = if (pct > 100) 100 else pct;
        let idx = ((_.count - 1) * clamped) / 100;
        if (idx >= _.count) {
            idx = _.count - 1;
        }
        return _.samples[idx];
    }

    pub fn p50Ns(): u64 {
        return _.percentileNs(50);
    }

    pub fn p95Ns(): u64 {
        return _.percentileNs(95);
    }

    pub fn p99Ns(): u64 {
        return _.percentileNs(99);
    }

    /// Computes operations per second based on mean sample duration in nanoseconds.
    pub fn opsPerSec(): u64 {
        const m = _.meanNs();
        if (m == 0) {
            return 0;
        }
        return 1000000000 / m;
    }
}
