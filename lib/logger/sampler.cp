// Source: cp/lib/logger/sampler.cp (Deterministic Ratio & Rate-Limited Log Sampler)
// lib/logger/sampler.cp
// Deterministic hash-based RatioSampler (0.0..1.0), window-based BurstSampler
// (full burst N then 1/M downsampling), and fixed-capacity zero-leak RingLogBuffer.

const std = @import("std");
import { StructuredRecord, LogFormatter } from "./structured.cp";
import { TraceParent } from "./trace.cp";

fn smAppendBytes(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn smU64FromHalves(hi: u64, lo: u64): u64 {
    return (hi << 32) | lo;
}

fn smSplitMix64(x: u64): u64 {
    const c1 = smU64FromHalves(0x9e3779b9, 0x7f4a7c15);
    const c2 = smU64FromHalves(0xbf58476d, 0x1ce4e5b9);
    const c3 = smU64FromHalves(0x94d049bb, 0x133111eb);
    let z = x +% c1;
    z = (z ^ (z >> 30)) *% c2;
    z = (z ^ (z >> 27)) *% c3;
    return z ^ (z >> 31);
}

fn smNowMonoNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    zig {
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    }
    if (ts.sec <= 0) return 0;
    return (ts.sec as u64) * 1000000000 + (ts.nsec as u64);
}

/// Deterministic hash-based proportional sampler (0.0 .. 1.0) keyed by TraceId or arbitrary string.
pub struct RatioSampler {
    ratio: f64 = 1.0;
    seed: u64 = 0;

    static fn init(ratio: f64): RatioSampler {
        return RatioSampler.initWithSeed(ratio, 0);
    }

    static fn initWithSeed(ratio: f64, seed: u64): RatioSampler {
        let clamped = ratio;
        if (std.math.isNan(clamped) || clamped <= 0.0) {
            clamped = 0.0;
        } else if (clamped >= 1.0) {
            clamped = 1.0;
        }
        return RatioSampler {
            ratio: clamped,
            seed: seed,
        };
    }

    static fn hashKey(key: []const u8, seed: u64): u64 {
        const fnv_offset: u64 = smU64FromHalves(0xcbf29ce4, 0x84222325);
        const fnv_prime: u64 = 1099511628211;
        let h: u64 = fnv_offset ^ seed;
        let i: usize = 0;
        while (i < key.len) {
            const b: u64 = u64(key[i]);
            h = (h ^ b) *% fnv_prime;
            i += 1;
        }
        return smSplitMix64(h);
    }

    fn shouldSample(key: []const u8): bool {
        if (_.ratio <= 0.0) return false;
        if (_.ratio >= 1.0) return true;
        const h = RatioSampler.hashKey(key, _.seed);
        const unit = ((h >> 11) as f64) / 9007199254740992.0;
        return unit < _.ratio;
    }

    fn shouldSampleTrace(tp: TraceParent): bool {
        return _.shouldSample(tp.trace_id[0..32]);
    }
}

/// Windowed burst + 1/M downsampling log rate limiter preventing exception storms from filling disk.
pub struct BurstSampler {
    burst_limit: u64 = 100;
    downsample_rate: u64 = 10;
    window_ns: u64 = 1000000000;
    window_start_ns: u64 = 0;
    window_count: u64 = 0;
    accepted_count: u64 = 0;
    dropped_count: u64 = 0;

    static fn init(burst_limit: u64, downsample_rate: u64, window_ns: u64): BurstSampler {
        return BurstSampler {
            burst_limit: burst_limit,
            downsample_rate: downsample_rate,
            window_ns: if (window_ns == 0) 1000000000 else window_ns,
            window_start_ns: 0,
            window_count: 0,
            accepted_count: 0,
            dropped_count: 0,
        };
    }

    /// Evaluates whether a log event at `now_ns` should be emitted.
    /// Allows the first `burst_limit` events in each `window_ns` window, then 1 out of `downsample_rate`.
    fn shouldSampleAt(*_, now_ns: u64): bool {
        if (_.window_start_ns == 0 || now_ns < _.window_start_ns || (now_ns - _.window_start_ns) >= _.window_ns) {
            _.window_start_ns = now_ns;
            _.window_count = 0;
        }
        const idx = _.window_count;
        _.window_count += 1;

        if (idx < _.burst_limit) {
            _.accepted_count += 1;
            return true;
        }
        if (_.downsample_rate == 0) {
            _.dropped_count += 1;
            return false;
        }
        const excess = idx - _.burst_limit;
        if (((excess + 1) % _.downsample_rate) == 0) {
            _.accepted_count += 1;
            return true;
        }
        _.dropped_count += 1;
        return false;
    }

    fn shouldSample(*_): bool {
        return _.shouldSampleAt(smNowMonoNs());
    }

    fn reset(*_) {
        _.window_start_ns = 0;
        _.window_count = 0;
        _.accepted_count = 0;
        _.dropped_count = 0;
    }

    fn acceptedCount(): u64 {
        return _.accepted_count;
    }

    fn droppedCount(): u64 {
        return _.dropped_count;
    }
}

/// Fixed-capacity zero-heap ring buffer storing recent `StructuredRecord` entries for crash dumps or replay.
pub struct RingLogBuffer<const CAP: usize> {
    records: [CAP]StructuredRecord = undefined;
    write_idx: usize = 0;
    count: usize = 0;
    total_pushed: u64 = 0;
    overwritten: u64 = 0;

    static fn init(): RingLogBuffer<CAP> {
        return RingLogBuffer<CAP> {
            records: undefined,
            write_idx: 0,
            count: 0,
            total_pushed: 0,
            overwritten: 0,
        };
    }

    fn push(*_, rec: StructuredRecord) {
        if (CAP == 0) {
            _.overwritten += 1;
            return;
        }
        if (_.count >= CAP) {
            _.overwritten += 1;
        } else {
            _.count += 1;
        }
        _.records[_.write_idx] = rec;
        _.write_idx = (_.write_idx + 1) % CAP;
        _.total_pushed += 1;
    }

    fn len(): usize {
        return _.count;
    }

    fn capacity(): usize {
        return if (_.count <= CAP) CAP else CAP;
    }

    fn isFull(): bool {
        return _.count == CAP;
    }

    fn totalPushed(): u64 {
        return _.total_pushed;
    }

    fn overwrittenCount(): u64 {
        return _.overwritten;
    }

    /// Returns the `logical_idx`-th record in chronological order (0 = oldest retained).
    fn get(logical_idx: usize): StructuredRecord? {
        if (logical_idx >= _.count || CAP == 0) return null;
        const start = if (_.count < CAP) 0 else _.write_idx;
        const phys = (start + logical_idx) % CAP;
        return _.records[phys];
    }

    fn latest(): StructuredRecord? {
        if (_.count == 0) return null;
        return _.get(_.count - 1);
    }

    fn clear(*_) {
        _.write_idx = 0;
        _.count = 0;
        _.total_pushed = 0;
        _.overwritten = 0;
    }

    /// Exports all retained records in chronological order as a JSON array.
    fn dumpJson(out_buf: []u8, redact_keys: []const []const u8): []const u8 {
        let pos: usize = 0;
        pos = smAppendBytes(out_buf, pos, "[");
        let i: usize = 0;
        while (i < _.count && pos < out_buf.len) {
            if (i > 0) {
                pos = smAppendBytes(out_buf, pos, ",");
            }
            if (rec := _.get(i)) {
                const chunk = LogFormatter.formatJson(rec, out_buf[pos..], redact_keys);
                pos += chunk.len;
            }
            i += 1;
        }
        pos = smAppendBytes(out_buf, pos, "]");
        return out_buf[0..pos];
    }

    /// Exports all retained records in chronological order as newline-delimited Logfmt lines.
    fn dumpLogfmt(out_buf: []u8, redact_keys: []const []const u8): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.count && pos < out_buf.len) {
            if (rec := _.get(i)) {
                const chunk = LogFormatter.formatLogfmt(rec, out_buf[pos..], redact_keys);
                pos += chunk.len;
                pos = smAppendBytes(out_buf, pos, "\n");
            }
            i += 1;
        }
        return out_buf[0..pos];
    }
}
