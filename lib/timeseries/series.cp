// Source: cp/lib/timeseries/series.cp (Time-Series Storage, Rollup Downsampling, SMA/EMA & Z-Score Anomaly Detection)
// Pure CP zero-C-dependency time-series engine with O(log N) binary-search range queries,
// retention-window eviction, aligned bucket downsampling, counter rate with reset handling,
// SMA/EMA smoothing, Welford summary statistics, Z-score anomaly detection, and compact JSON export.

const std = @import("std");

/// Error set for time-series operations.
pub err TimeSeriesError {
    OutOfOrderTimestamp,
    InvalidStep,
    InvalidWindow,
    EmptySeries,
    InvalidValue,
    OutOfMemory,
}

/// Aggregation operator for aligned bucket downsampling.
pub enum AggOp {
    Avg,
    Min,
    Max,
    Sum,
    Count,
    First,
    Last,
    P95,

    pub fn toString(): string {
        switch (_) {
            .Avg => return "avg",
            .Min => return "min",
            .Max => return "max",
            .Sum => return "sum",
            .Count => return "count",
            .First => return "first",
            .Last => return "last",
            .P95 => return "p95",
        }
    }
}

/// Single time-series observation point.
pub struct DataPoint {
    ts_ms: i64 = 0;
    value: f64 = 0.0;
}

/// Detected anomaly point with computed Z-score.
pub struct AnomalyPoint {
    ts_ms: i64 = 0;
    value: f64 = 0.0;
    z_score: f64 = 0.0;
}

/// Descriptive summary of a time-series.
pub struct SeriesSummary {
    count: usize = 0;
    min: f64 = 0.0;
    max: f64 = 0.0;
    sum: f64 = 0.0;
    mean: f64 = 0.0;
    stddev: f64 = 0.0;
    first: f64 = 0.0;
    last: f64 = 0.0;
}

fn lowerBound(points: []const DataPoint, target_ts: i64): usize {
    let lo: usize = 0;
    let hi: usize = points.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        if (points[mid].ts_ms < target_ts) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

fn upperBound(points: []const DataPoint, target_ts: i64): usize {
    let lo: usize = 0;
    let hi: usize = points.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        if (points[mid].ts_ms <= target_ts) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

fn quantileSorted(sorted: []const f64, q: f64): f64 {
    if (sorted.len == 0) return 0.0;
    if (sorted.len == 1) return sorted[0];
    const pos = q * ((sorted.len - 1) as f64);
    const lo = pos as usize;
    const hi = if (lo + 1 < sorted.len) lo + 1 else lo;
    const frac = pos - (lo as f64);
    return sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
}

fn aggregateBucket(alloc: std.mem.Allocator, bucket: []const DataPoint, op: AggOp): f64 ! TimeSeriesError {
    if (bucket.len == 0) return TimeSeriesError.EmptySeries;

    switch (op) {
        .Avg => {
            let sum: f64 = 0.0;
            for (p of bucket) {
                sum += p.value;
            }
            return sum / (bucket.len as f64);
        },
        .Min => {
            let m = bucket[0].value;
            for (i of 1..bucket.len) {
                if (bucket[i].value < m) m = bucket[i].value;
            }
            return m;
        },
        .Max => {
            let m = bucket[0].value;
            for (i of 1..bucket.len) {
                if (bucket[i].value > m) m = bucket[i].value;
            }
            return m;
        },
        .Sum => {
            let sum: f64 = 0.0;
            for (p of bucket) {
                sum += p.value;
            }
            return sum;
        },
        .Count => {
            return bucket.len as f64;
        },
        .First => {
            return bucket[0].value;
        },
        .Last => {
            return bucket[bucket.len - 1].value;
        },
        .P95 => {
            let vals = alloc.alloc(f64, bucket.len) catch return TimeSeriesError.OutOfMemory;
            defer alloc.free(vals);
            for (i of 0..bucket.len) {
                vals[i] = bucket[i].value;
            }
            zig {
                std.sort.pdq(f64, vals, {}, std.sort.asc(f64));
            }
            return quantileSorted(vals, 0.95);
        },
    }
}

fn appendJsonEscaped(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, text: string): void ! TimeSeriesError {
    buf.append(alloc, '"') catch return TimeSeriesError.OutOfMemory;
    for (ch of text) {
        switch (ch) {
            '"' => {
                buf.appendSlice(alloc, "\\\"") catch return TimeSeriesError.OutOfMemory;
            },
            '\\' => {
                buf.appendSlice(alloc, "\\\\") catch return TimeSeriesError.OutOfMemory;
            },
            '\n' => {
                buf.appendSlice(alloc, "\\n") catch return TimeSeriesError.OutOfMemory;
            },
            '\r' => {
                buf.appendSlice(alloc, "\\r") catch return TimeSeriesError.OutOfMemory;
            },
            '\t' => {
                buf.appendSlice(alloc, "\\t") catch return TimeSeriesError.OutOfMemory;
            },
            else => {
                if (ch < 0x20) {
                    let hex_buf: [6]u8 = undefined;
                    let s: []const u8 = "";
                    zig {
                        s = std.fmt.bufPrint(&hex_buf, "\\u{x:0>4}", .{ch}) catch return TimeSeriesError.OutOfMemory;
                    }
                    buf.appendSlice(alloc, s) catch return TimeSeriesError.OutOfMemory;
                } else {
                    buf.append(alloc, ch) catch return TimeSeriesError.OutOfMemory;
                }
            },
        }
    }
    buf.append(alloc, '"') catch return TimeSeriesError.OutOfMemory;
}

/// Monotonic time-series container with retention eviction, range queries, downsampling,
/// rate calculation, smoothing, and anomaly detection.
pub struct TimeSeries {
    alloc: std.mem.Allocator;
    name: string;
    retention_ms: i64;
    points: std.ArrayListUnmanaged(DataPoint) = .empty;
    head: usize = 0;

    /// Initializes an empty TimeSeries. `retention_ms <= 0` disables automatic head eviction.
    pub static fn init(alloc: std.mem.Allocator, name: string, retention_ms: i64): TimeSeries {
        return TimeSeries {
            alloc: alloc,
            name: name,
            retention_ms: retention_ms,
            points: .empty,
            head: 0,
        };
    }

    /// Releases all owned buffers.
    pub fn deinit(*_) {
        _.points.deinit(_.alloc);
        _.head = 0;
    }

    /// Returns the active slice of stored points in chronological order.
    pub fn items(): []const DataPoint {
        return _.points.items[_.head..];
    }

    /// Returns the number of active points in the series.
    pub fn len(): usize {
        return _.points.items.len - _.head;
    }

    /// Appends a new `(ts_ms, val)` observation, enforcing finite values, monotonic timestamps,
    /// and optional retention-window eviction.
    pub fn push(*_, ts_ms: i64, val: f64): void ! TimeSeriesError {
        if (std.math.isNan(val) || std.math.isInf(val)) {
            return TimeSeriesError.InvalidValue;
        }

        const active = _.points.items[_.head..];
        if (active.len > 0 && ts_ms < active[active.len - 1].ts_ms) {
            return TimeSeriesError.OutOfOrderTimestamp;
        }

        _.points.append(_.alloc, DataPoint{ ts_ms: ts_ms, value: val }) catch return TimeSeriesError.OutOfMemory;

        if (_.retention_ms > 0) {
            const cutoff = ts_ms - _.retention_ms;
            while (_.head < _.points.items.len && _.points.items[_.head].ts_ms < cutoff) {
                _.head += 1;
            }
            // Periodically compact dead prefix to keep memory bounded in O(1) amortized time.
            if (_.head >= 64 && _.head * 2 >= _.points.items.len) {
                const rem = _.points.items.len - _.head;
                if (rem > 0) {
                    std.mem.copyForwards(DataPoint, _.points.items[0..rem], _.points.items[_.head..]);
                }
                _.points.items.len = rem;
                _.head = 0;
            }
        }
    }

    /// Queries points in the inclusive time range `[start_ms, end_ms]` using O(log N) binary search.
    pub fn queryRange(alloc: std.mem.Allocator, start_ms: i64, end_ms: i64): []DataPoint ! TimeSeriesError {
        if (end_ms < start_ms) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }
        const active = _.points.items[_.head..];
        const lo = lowerBound(active, start_ms);
        const hi = upperBound(active, end_ms);
        if (lo >= hi) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }
        const out = alloc.alloc(DataPoint, hi - lo) catch return TimeSeriesError.OutOfMemory;
        @memcpy(out, active[lo..hi]);
        return out;
    }

    /// Downsamples points in `[start_ms, end_ms]` into fixed `step_ms`-aligned buckets using `op`.
    pub fn downsample(
        alloc: std.mem.Allocator,
        start_ms: i64,
        end_ms: i64,
        step_ms: i64,
        op: AggOp,
    ): []DataPoint ! TimeSeriesError {
        if (step_ms <= 0) {
            return TimeSeriesError.InvalidStep;
        }
        if (end_ms < start_ms) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        const active = _.points.items[_.head..];
        const lo = lowerBound(active, start_ms);
        const hi = upperBound(active, end_ms);
        if (lo >= hi) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        let out: std.ArrayListUnmanaged(DataPoint) = .empty;
        errdefer out.deinit(alloc);

        const slice = active[lo..hi];
        let idx: usize = 0;
        while (idx < slice.len) {
            const bucket_idx = @divFloor(slice[idx].ts_ms - start_ms, step_ms);
            const bucket_ts = start_ms + bucket_idx * step_ms;
            let end_idx = idx + 1;
            while (end_idx < slice.len) {
                const next_bucket = @divFloor(slice[end_idx].ts_ms - start_ms, step_ms);
                if (next_bucket != bucket_idx) break;
                end_idx += 1;
            }
            const agg_val = aggregateBucket(alloc, slice[idx..end_idx], op)?;
            out.append(alloc, DataPoint{ ts_ms: bucket_ts, value: agg_val }) catch return TimeSeriesError.OutOfMemory;
            idx = end_idx;
        }

        return out.toOwnedSlice(alloc) catch return TimeSeriesError.OutOfMemory;
    }

    /// Computes per-second rate of change (`dv / dt_sec`) for monotonic counters in `[start_ms, end_ms]`,
    /// safely handling counter resets where `v[i] < v[i - 1]`.
    pub fn rate(alloc: std.mem.Allocator, start_ms: i64, end_ms: i64): []DataPoint ! TimeSeriesError {
        if (end_ms < start_ms) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }
        const active = _.points.items[_.head..];
        const lo = lowerBound(active, start_ms);
        const hi = upperBound(active, end_ms);
        if (hi <= lo || hi - lo < 2) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        const slice = active[lo..hi];
        let out: std.ArrayListUnmanaged(DataPoint) = .empty;
        errdefer out.deinit(alloc);

        for (i of 1..slice.len) {
            const dt_ms = slice[i].ts_ms - slice[i - 1].ts_ms;
            if (dt_ms <= 0) continue;
            const dt_sec = @as(f64, @floatFromInt(dt_ms)) / 1000.0;
            const prev_val = slice[i - 1].value;
            const cur_val = slice[i].value;
            // Counter reset: if current value dropped below previous, treat reset baseline as 0.
            const dv = if (cur_val < prev_val) (if (cur_val >= 0.0) cur_val else 0.0) else (cur_val - prev_val);
            out.append(alloc, DataPoint{ ts_ms: slice[i].ts_ms, value: dv / dt_sec }) catch return TimeSeriesError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return TimeSeriesError.OutOfMemory;
    }

    /// Computes Simple Moving Average (SMA) over `window` samples.
    pub fn sma(alloc: std.mem.Allocator, window: usize): []DataPoint ! TimeSeriesError {
        if (window == 0) {
            return TimeSeriesError.InvalidWindow;
        }
        const active = _.points.items[_.head..];
        if (active.len == 0) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        let out = alloc.alloc(DataPoint, active.len) catch return TimeSeriesError.OutOfMemory;
        let running_sum: f64 = 0.0;
        let i: usize = 0;
        while (i < active.len) : (i += 1) {
            running_sum += active[i].value;
            if (i >= window) {
                running_sum -= active[i - window].value;
            }
            const denom = if (i + 1 < window) (i + 1) else window;
            out[i] = DataPoint{
                ts_ms: active[i].ts_ms,
                value: running_sum / @as(f64, @floatFromInt(denom)),
            };
        }
        return out;
    }

    /// Computes Exponential Moving Average (EMA) with smoothing factor `alpha` in `(0, 1]`.
    pub fn ema(alloc: std.mem.Allocator, alpha: f64): []DataPoint ! TimeSeriesError {
        if (std.math.isNan(alpha) || std.math.isInf(alpha) || alpha <= 0.0 || alpha > 1.0) {
            return TimeSeriesError.InvalidValue;
        }
        const active = _.points.items[_.head..];
        if (active.len == 0) {
            return alloc.alloc(DataPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        let out = alloc.alloc(DataPoint, active.len) catch return TimeSeriesError.OutOfMemory;
        out[0] = active[0];
        const one_minus_alpha = 1.0 - alpha;
        for (i of 1..active.len) {
            const smoothed = alpha * active[i].value + one_minus_alpha * out[i - 1].value;
            out[i] = DataPoint{
                ts_ms: active[i].ts_ms,
                value: smoothed,
            };
        }
        return out;
    }

    /// Detects anomalous spikes where `|value - mean| / stddev >= z_threshold`.
    pub fn detectAnomaliesZScore(alloc: std.mem.Allocator, z_threshold: f64): []AnomalyPoint ! TimeSeriesError {
        if (std.math.isNan(z_threshold) || std.math.isInf(z_threshold) || z_threshold <= 0.0) {
            return TimeSeriesError.InvalidValue;
        }
        const active = _.points.items[_.head..];
        if (active.len < 2) {
            return alloc.alloc(AnomalyPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        const stats = _.summarize()?;
        if (stats.stddev <= 0.0) {
            return alloc.alloc(AnomalyPoint, 0) catch return TimeSeriesError.OutOfMemory;
        }

        let out: std.ArrayListUnmanaged(AnomalyPoint) = .empty;
        errdefer out.deinit(alloc);

        for (p of active) {
            const z = (p.value - stats.mean) / stats.stddev;
            const abs_z = if (z >= 0.0) z else -z;
            if (abs_z >= z_threshold) {
                out.append(alloc, AnomalyPoint{
                    ts_ms: p.ts_ms,
                    value: p.value,
                    z_score: z,
                }) catch return TimeSeriesError.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch return TimeSeriesError.OutOfMemory;
    }

    /// Computes descriptive summary statistics using Welford's numerically stable single-pass algorithm.
    pub fn summarize(): SeriesSummary ! TimeSeriesError {
        const active = _.points.items[_.head..];
        if (active.len == 0) {
            return TimeSeriesError.EmptySeries;
        }

        let min_v = active[0].value;
        let max_v = active[0].value;
        let sum: f64 = 0.0;
        let mean: f64 = 0.0;
        let m2: f64 = 0.0;

        for (i of 0..active.len) {
            const x = active[i].value;
            if (x < min_v) min_v = x;
            if (x > max_v) max_v = x;
            sum += x;
            const k = (i + 1) as f64;
            const delta = x - mean;
            mean += delta / k;
            const delta2 = x - mean;
            m2 += delta * delta2;
        }

        const variance = m2 / (active.len as f64);
        const stddev = @sqrt(if (variance > 0.0) variance else 0.0);

        return SeriesSummary {
            count: active.len,
            min: min_v,
            max: max_v,
            sum: sum,
            mean: mean,
            stddev: stddev,
            first: active[0].value,
            last: active[active.len - 1].value,
        };
    }

    /// Exports the time-series as compact JSON. Caller owns the returned slice and must free it with `_.alloc`.
    pub fn toJson(): string ! TimeSeriesError {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(_.alloc);

        const active = _.points.items[_.head..];
        let num_buf: [64]u8 = undefined;

        out.appendSlice(_.alloc, "{\"name\":") catch return TimeSeriesError.OutOfMemory;
        appendJsonEscaped(*out, _.alloc, _.name)?;

        out.appendSlice(_.alloc, ",\"retention_ms\":") catch return TimeSeriesError.OutOfMemory;
        const ret_ms = _.retention_ms;
        let ret_s: []const u8 = "";
        zig {
            ret_s = std.fmt.bufPrint(&num_buf, "{d}", .{ret_ms}) catch return TimeSeriesError.OutOfMemory;
        }
        out.appendSlice(_.alloc, ret_s) catch return TimeSeriesError.OutOfMemory;

        out.appendSlice(_.alloc, ",\"count\":") catch return TimeSeriesError.OutOfMemory;
        let cnt_s: []const u8 = "";
        zig {
            cnt_s = std.fmt.bufPrint(&num_buf, "{d}", .{active.len}) catch return TimeSeriesError.OutOfMemory;
        }
        out.appendSlice(_.alloc, cnt_s) catch return TimeSeriesError.OutOfMemory;

        out.appendSlice(_.alloc, ",\"points\":[") catch return TimeSeriesError.OutOfMemory;
        for (i of 0..active.len) {
            if (i > 0) {
                out.append(_.alloc, ',') catch return TimeSeriesError.OutOfMemory;
            }
            const p = active[i];
            let pt_s: []const u8 = "";
            zig {
                pt_s = std.fmt.bufPrint(&num_buf, "{{\"ts_ms\":{d},\"value\":{d}}}", .{ p.ts_ms, p.value }) catch return TimeSeriesError.OutOfMemory;
            }
            out.appendSlice(_.alloc, pt_s) catch return TimeSeriesError.OutOfMemory;
        }
        out.appendSlice(_.alloc, "]}") catch return TimeSeriesError.OutOfMemory;

        return out.toOwnedSlice(_.alloc) catch return TimeSeriesError.OutOfMemory;
    }
}
