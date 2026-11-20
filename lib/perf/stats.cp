// Source: cp/lib/perf/stats.cp (Hotspot aggregation: count, total, min, max, avg, p95, p99)

import std from "std";
import { PerfEvent } from "./types.cp";

pub struct PerfStatSummary {
    name: []const u8,
    count: usize,
    total_ns: u64,
    min_ns: u64,
    max_ns: u64,
    avg_ns: u64,
    p95_ns: u64,
    p99_ns: u64,
}

struct StatBucket {
    name: []const u8,
    durations: std.ArrayList(u64),
}

fn percentileIndex(percentile: usize, len: usize): usize {
    if (len == 0) {
        return 0;
    }
    const rank = (percentile * len + 99) / 100;
    if (rank == 0) {
        return 0;
    }
    const idx = rank - 1;
    if (idx >= len) {
        return len - 1;
    }
    return idx;
}

pub fn summarizeByName(alloc: std.mem.Allocator, events: []const PerfEvent): ![]PerfStatSummary {
    let buckets: std.ArrayList(StatBucket) = .empty;
    errdefer {
        for (*b of buckets.items) {
            b.durations.deinit(alloc);
        }
        buckets.deinit(alloc);
    }

    for (ev of events) {
        if (ev.phase != .Complete && ev.phase != .End) {
            continue;
        }
        let found_idx: usize? = null;
        let idx: usize = 0;
        for (b of buckets.items) {
            if (std.mem.eql(u8, b.name, ev.name)) {
                found_idx = idx;
                break;
            }
            idx += 1;
        }
        if (matched := found_idx) {
            try buckets.items[matched].durations.append(alloc, ev.dur_ns);
        } else {
            let durs: std.ArrayList(u64) = .empty;
            errdefer durs.deinit(alloc);
            try durs.append(alloc, ev.dur_ns);
            try buckets.append(alloc, StatBucket{
                name: ev.name,
                durations: durs,
            });
        }
    }

    let out: std.ArrayList(PerfStatSummary) = .empty;
    errdefer {
        for (s of out.items) {
            alloc.free(s.name);
        }
        out.deinit(alloc);
    }

    for (*b of buckets.items) {
        const n = b.durations.items.len;
        std.mem.sort(u64, b.durations.items, {}, std.sort.asc(u64));

        let total_ns: u64 = 0;
        for (d of b.durations.items) {
            total_ns += d;
        }
        const min_ns = b.durations.items[0];
        const max_ns = b.durations.items[n - 1];
        const avg_ns = total_ns / @as(u64, @intCast(n));
        const p95_ns = b.durations.items[percentileIndex(95, n)];
        const p99_ns = b.durations.items[percentileIndex(99, n)];

        const name_copy = try alloc.dupe(u8, b.name);
        errdefer alloc.free(name_copy);

        try out.append(alloc, PerfStatSummary{
            name: name_copy,
            count: n,
            total_ns: total_ns,
            min_ns: min_ns,
            max_ns: max_ns,
            avg_ns: avg_ns,
            p95_ns: p95_ns,
            p99_ns: p99_ns,
        });
    }

    for (*b of buckets.items) {
        b.durations.deinit(alloc);
    }
    buckets.deinit(alloc);

    return try out.toOwnedSlice(alloc);
}

pub fn freeSummaries(alloc: std.mem.Allocator, summaries: []PerfStatSummary) {
    for (s of summaries) {
        alloc.free(s.name);
    }
    alloc.free(summaries);
}
