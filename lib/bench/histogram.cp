// Source: Absorbed from /data/dev/bun/src/perf/perf.cp | Synced: 2026-11-23T22:06:00Z
// lib/bench/histogram.cp
// Fixed-Bucket Nanosecond Latency Histogram with Min/Max/Mean & Percentile Estimation

/// 8-bucket logarithmic latency histogram:
///   Bucket 0: < 100 ns
///   Bucket 1: < 1,000 ns (1 us)
///   Bucket 2: < 10,000 ns (10 us)
///   Bucket 3: < 100,000 ns (100 us)
///   Bucket 4: < 1,000,000 ns (1 ms)
///   Bucket 5: < 10,000,000 ns (10 ms)
///   Bucket 6: < 100,000,000 ns (100 ms)
///   Bucket 7: >= 100,000,000 ns
pub struct LatencyHistogram {
    buckets: [8]u64 = undefined;
    bounds: [8]u64 = undefined;
    total_count: u64 = 0;
    total_ns: u64 = 0;
    min_ns: u64 = 0;
    max_ns: u64 = 0;

    pub static fn init(): LatencyHistogram {
        let h = LatencyHistogram {
            buckets: undefined,
            bounds: undefined,
            total_count: 0,
            total_ns: 0,
            min_ns: 0,
            max_ns: 0,
        };
        let i: usize = 0;
        while (i < 8) {
            h.buckets[i] = 0;
            i += 1;
        }
        h.bounds[0] = 100;
        h.bounds[1] = 1000;
        h.bounds[2] = 10000;
        h.bounds[3] = 100000;
        h.bounds[4] = 1000000;
        h.bounds[5] = 10000000;
        h.bounds[6] = 100000000;
        h.bounds[7] = 0x7FFFFFFFFFFFFFFF;
        return h;
    }

    pub static fn bucketIndexFor(ns: u64): usize {
        if (ns < 100) { return 0; }
        if (ns < 1000) { return 1; }
        if (ns < 10000) { return 2; }
        if (ns < 100000) { return 3; }
        if (ns < 1000000) { return 4; }
        if (ns < 10000000) { return 5; }
        if (ns < 100000000) { return 6; }
        return 7;
    }

    pub fn record(*_, ns: u64) {
        const b = LatencyHistogram.bucketIndexFor(ns);
        _.buckets[b] += 1;
        if (_.total_count == 0 || ns < _.min_ns) {
            _.min_ns = ns;
        }
        if (ns > _.max_ns) {
            _.max_ns = ns;
        }
        _.total_count += 1;
        _.total_ns += ns;
    }

    pub fn count(): u64 {
        return _.total_count;
    }

    pub fn bucketCount(idx: usize): u64 {
        if (idx >= 8) {
            return 0;
        }
        return _.buckets[idx];
    }

    pub fn minNs(): u64 {
        return _.min_ns;
    }

    pub fn maxNs(): u64 {
        return _.max_ns;
    }

    pub fn meanNs(): u64 {
        if (_.total_count == 0) {
            return 0;
        }
        return _.total_ns / _.total_count;
    }

    /// Estimates the upper-bound latency (in ns) of the bucket containing the given percentile `pct` (0..100).
    pub fn estimatePercentileBoundNs(pct: u64): u64 {
        if (_.total_count == 0) {
            return 0;
        }
        const clamped = if (pct > 100) (100 as u64) else pct;
        let target = (_.total_count * clamped + 99) / 100;
        if (target == 0) {
            target = 1;
        }
        let cumulative: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            cumulative += _.buckets[i];
            if (cumulative >= target) {
                return if (i == 7) _.max_ns else _.bounds[i];
            }
            i += 1;
        }
        return _.max_ns;
    }

    pub fn reset(*_) {
        let i: usize = 0;
        while (i < 8) {
            _.buckets[i] = 0;
            i += 1;
        }
        _.total_count = 0;
        _.total_ns = 0;
        _.min_ns = 0;
        _.max_ns = 0;
    }
}
