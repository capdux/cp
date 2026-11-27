// Source: cp/lib/prometheus/prometheus.cp (Pure CP Prometheus / OpenMetrics Registry, Histogram & Text Exposition Codec)
// Zero-C-dependency Prometheus metric registry, cumulative histogram, text exposition serializer & parser.

const std = @import("std");

/// Unified error set returned by `cap:prometheus` registry, validation, and exposition codec operations.
pub err PromError {
    InvalidMetricName,
    InvalidLabelName,
    NegativeCounterIncrement,
    DuplicateMetric,
    UnsortedBuckets,
    ParseError,
    OutOfMemory,
}

/// Prometheus metric family types.
pub enum MetricType {
    Counter,
    Gauge,
    Histogram,
}

/// Key-value label pair attached to a metric time series.
pub struct LabelPair {
    name: []const u8;
    value: []const u8;
}

/// Single parsed metric sample returned by `parseExposition`.
pub struct ParsedSample {
    name: []const u8;
    labels: []LabelPair;
    value: f64;
    help: ([]const u8)?;
    metric_type: MetricType?;
}

struct OwnedLabel {
    name: []u8;
    value: []u8;
}

struct SeriesEntry {
    labels: []OwnedLabel;
    value: f64;
    bucket_counts: []u64;
    hist_sum: f64;
    hist_count: u64;
}

struct MetricEntry {
    name: []u8;
    help: []u8;
    metric_type: MetricType;
    buckets: []f64;
    series: std.ArrayListUnmanaged(SeriesEntry);
}

struct MetaRecord {
    name: []const u8;
    help: ([]u8)?;
    metric_type: MetricType?;
}

struct WordRead {
    word: []const u8;
    next: usize;
}

struct LabelParseResult {
    labels: []LabelPair;
    next: usize;
}

type PromVec16 = @Vector(16, u8);

const PROM_MASK_METRIC_START: u8 = 1 << 0;
const PROM_MASK_METRIC_CONT: u8 = 1 << 1;
const PROM_MASK_LABEL_START: u8 = 1 << 2;
const PROM_MASK_LABEL_CONT: u8 = 1 << 3;

fn makePromCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let m: u8 = 0;
        const is_alpha = (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z');
        const is_digit = i >= '0' && i <= '9';
        if (is_alpha || i == '_') {
            m |= PROM_MASK_LABEL_START | PROM_MASK_LABEL_CONT | PROM_MASK_METRIC_START | PROM_MASK_METRIC_CONT;
        }
        if (is_digit) {
            m |= PROM_MASK_LABEL_CONT | PROM_MASK_METRIC_CONT;
        }
        if (i == ':') {
            m |= PROM_MASK_METRIC_START | PROM_MASK_METRIC_CONT;
        }
        lut[i] = m;
    }
    return lut;
}

const PROM_CHAR_LUT: [u8; 256] = makePromCharLut();

fn isValidMetricName(name: []const u8): bool {
    if (name.len == 0) return false;
    if ((PROM_CHAR_LUT[usize(name[0])] & PROM_MASK_METRIC_START) == 0) {
        return false;
    }
    let i: usize = 1;
    const lo_a: PromVec16 = @splat('a' as u8);
    const hi_z: PromVec16 = @splat('z' as u8);
    const lo_A: PromVec16 = @splat('A' as u8);
    const hi_Z: PromVec16 = @splat('Z' as u8);
    const lo_0: PromVec16 = @splat('0' as u8);
    const hi_9: PromVec16 = @splat('9' as u8);
    const us_v: PromVec16 = @splat('_' as u8);
    const col_v: PromVec16 = @splat(':' as u8);
    while (i + 16 <= name.len) : (i += 16) {
        const v: PromVec16 = name[i .. i + 16][0..16].*;
        const ok = ((v >= lo_a) & (v <= hi_z)) | ((v >= lo_A) & (v <= hi_Z)) |
            ((v >= lo_0) & (v <= hi_9)) | (v == us_v) | (v == col_v);
        if (!@reduce(.And, ok)) return false;
    }
    while (i < name.len) : (i += 1) {
        if ((PROM_CHAR_LUT[usize(name[i])] & PROM_MASK_METRIC_CONT) == 0) {
            return false;
        }
    }
    return true;
}

fn isValidLabelName(name: []const u8): bool {
    if (name.len == 0) return false;
    // Reserved prefix `__` is forbidden for user-defined labels.
    if (name.len >= 2 && name[0] == '_' && name[1] == '_') {
        return false;
    }
    return isValidLabelNameForParser(name);
}

fn isValidLabelNameForParser(name: []const u8): bool {
    if (name.len == 0) return false;
    if ((PROM_CHAR_LUT[usize(name[0])] & PROM_MASK_LABEL_START) == 0) {
        return false;
    }
    let i: usize = 1;
    const lo_a: PromVec16 = @splat('a' as u8);
    const hi_z: PromVec16 = @splat('z' as u8);
    const lo_A: PromVec16 = @splat('A' as u8);
    const hi_Z: PromVec16 = @splat('Z' as u8);
    const lo_0: PromVec16 = @splat('0' as u8);
    const hi_9: PromVec16 = @splat('9' as u8);
    const us_v: PromVec16 = @splat('_' as u8);
    while (i + 16 <= name.len) : (i += 16) {
        const v: PromVec16 = name[i .. i + 16][0..16].*;
        const ok = ((v >= lo_a) & (v <= hi_z)) | ((v >= lo_A) & (v <= hi_Z)) |
            ((v >= lo_0) & (v <= hi_9)) | (v == us_v);
        if (!@reduce(.And, ok)) return false;
    }
    while (i < name.len) : (i += 1) {
        if ((PROM_CHAR_LUT[usize(name[i])] & PROM_MASK_LABEL_CONT) == 0) {
            return false;
        }
    }
    return true;
}

fn validateLabels(labels: []const LabelPair): void ! PromError {
    let i: usize = 0;
    while (i < labels.len) : (i += 1) {
        if (!isValidLabelName(labels[i].name)) {
            return PromError.InvalidLabelName;
        }
    }
}

fn dupeBytes(allocator: std.mem.Allocator, src: []const u8): []u8 ! PromError {
    return allocator.dupe(u8, src) catch return PromError.OutOfMemory;
}

fn freeOwnedLabels(allocator: std.mem.Allocator, labels: []OwnedLabel) {
    let i: usize = 0;
    while (i < labels.len) : (i += 1) {
        allocator.free(labels[i].name);
        allocator.free(labels[i].value);
    }
    allocator.free(labels);
}

fn cloneLabels(allocator: std.mem.Allocator, labels: []const LabelPair): []OwnedLabel ! PromError {
    let out = allocator.alloc(OwnedLabel, labels.len) catch return PromError.OutOfMemory;
    let i: usize = 0;
    while (i < labels.len) : (i += 1) {
        const k = allocator.dupe(u8, labels[i].name) catch {
            let j: usize = 0;
            while (j < i) : (j += 1) {
                allocator.free(out[j].name);
                allocator.free(out[j].value);
            }
            allocator.free(out);
            return PromError.OutOfMemory;
        };
        const v = allocator.dupe(u8, labels[i].value) catch {
            allocator.free(k);
            let j: usize = 0;
            while (j < i) : (j += 1) {
                allocator.free(out[j].name);
                allocator.free(out[j].value);
            }
            allocator.free(out);
            return PromError.OutOfMemory;
        };
        out[i] = OwnedLabel{ name: k, value: v };
    }
    return out;
}

fn labelsMatch(a: []const OwnedLabel, b: []const LabelPair): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!std.mem.eql(u8, a[i].name, b[i].name)) return false;
        if (!std.mem.eql(u8, a[i].value, b[i].value)) return false;
    }
    return true;
}

fn appendEscapedHelp(buf: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): void ! PromError {
    const bs_v: PromVec16 = @splat('\\' as u8);
    const nl_v: PromVec16 = @splat('\n' as u8);
    let i: usize = 0;
    let run_start: usize = 0;
    while (i < s.len) {
        while (i + 16 <= s.len) {
            const v: PromVec16 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == bs_v) | (v == nl_v))) break;
            i += 16;
        }
        if (i >= s.len) break;
        const c = s[i];
        if (c == '\\' || c == '\n') {
            if (i > run_start) {
                buf.appendSlice(allocator, s[run_start..i]) catch return PromError.OutOfMemory;
            }
            if (c == '\\') {
                buf.appendSlice(allocator, "\\\\") catch return PromError.OutOfMemory;
            } else {
                buf.appendSlice(allocator, "\\n") catch return PromError.OutOfMemory;
            }
            i += 1;
            run_start = i;
        } else {
            i += 1;
        }
    }
    if (run_start < s.len) {
        buf.appendSlice(allocator, s[run_start..s.len]) catch return PromError.OutOfMemory;
    }
}

fn appendEscapedLabelValue(buf: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): void ! PromError {
    const bs_v: PromVec16 = @splat('\\' as u8);
    const dq_v: PromVec16 = @splat('"' as u8);
    const nl_v: PromVec16 = @splat('\n' as u8);
    let i: usize = 0;
    let run_start: usize = 0;
    while (i < s.len) {
        while (i + 16 <= s.len) {
            const v: PromVec16 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == bs_v) | (v == dq_v) | (v == nl_v))) break;
            i += 16;
        }
        if (i >= s.len) break;
        const c = s[i];
        if (c == '\\' || c == '"' || c == '\n') {
            if (i > run_start) {
                buf.appendSlice(allocator, s[run_start..i]) catch return PromError.OutOfMemory;
            }
            if (c == '\\') {
                buf.appendSlice(allocator, "\\\\") catch return PromError.OutOfMemory;
            } else if (c == '"') {
                buf.appendSlice(allocator, "\\\"") catch return PromError.OutOfMemory;
            } else {
                buf.appendSlice(allocator, "\\n") catch return PromError.OutOfMemory;
            }
            i += 1;
            run_start = i;
        } else {
            i += 1;
        }
    }
    if (run_start < s.len) {
        buf.appendSlice(allocator, s[run_start..s.len]) catch return PromError.OutOfMemory;
    }
}

fn appendFloat(buf: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, val: f64): void ! PromError {
    if (std.math.isNan(val)) {
        buf.appendSlice(allocator, "NaN") catch return PromError.OutOfMemory;
        return;
    }
    if (std.math.isPositiveInf(val)) {
        buf.appendSlice(allocator, "+Inf") catch return PromError.OutOfMemory;
        return;
    }
    if (std.math.isNegativeInf(val)) {
        buf.appendSlice(allocator, "-Inf") catch return PromError.OutOfMemory;
        return;
    }
    let tmp: [64]u8 = undefined;
    const formatted = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return PromError.OutOfMemory;
    buf.appendSlice(allocator, formatted) catch return PromError.OutOfMemory;
}

fn appendU64(buf: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, val: u64): void ! PromError {
    let tmp: [32]u8 = undefined;
    const formatted = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return PromError.OutOfMemory;
    buf.appendSlice(allocator, formatted) catch return PromError.OutOfMemory;
}

fn appendLabels(buf: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, labels: []const OwnedLabel, extra_le: ([]const u8)?): void ! PromError {
    const has_extra = extra_le != null;
    if (labels.len == 0 && !has_extra) return;

    buf.append(allocator, '{') catch return PromError.OutOfMemory;
    let i: usize = 0;
    while (i < labels.len) : (i += 1) {
        if (i > 0) {
            buf.append(allocator, ',') catch return PromError.OutOfMemory;
        }
        buf.appendSlice(allocator, labels[i].name) catch return PromError.OutOfMemory;
        buf.appendSlice(allocator, "=\"") catch return PromError.OutOfMemory;
        appendEscapedLabelValue(buf, allocator, labels[i].value)?;
        buf.append(allocator, '"') catch return PromError.OutOfMemory;
    }
    if (le_val := extra_le) {
        if (labels.len > 0) {
            buf.append(allocator, ',') catch return PromError.OutOfMemory;
        }
        buf.appendSlice(allocator, "le=\"") catch return PromError.OutOfMemory;
        buf.appendSlice(allocator, le_val) catch return PromError.OutOfMemory;
        buf.append(allocator, '"') catch return PromError.OutOfMemory;
    }
    buf.append(allocator, '}') catch return PromError.OutOfMemory;
}

/// Prometheus metric registry supporting Counter, Gauge, Histogram, and text/plain v0.0.4 exposition.
pub class PromRegistry {
    allocator: std.mem.Allocator;
    metrics: std.ArrayListUnmanaged(MetricEntry);

    /// Initializes an empty Prometheus registry using `allocator`.
    pub static fn init(allocator: std.mem.Allocator): PromRegistry {
        return PromRegistry{
            allocator: allocator,
            metrics: std.ArrayListUnmanaged(MetricEntry).empty,
        };
    }

    /// Releases all registered metrics, series, labels, and internal buffers.
    pub fn deinit(*_) {
        let m_idx: usize = 0;
        while (m_idx < _.metrics.items.len) : (m_idx += 1) {
            const entry = &_.metrics.items[m_idx];
            _.allocator.free(entry.name);
            _.allocator.free(entry.help);
            _.allocator.free(entry.buckets);
            let s_idx: usize = 0;
            while (s_idx < entry.series.items.len) : (s_idx += 1) {
                const s = &entry.series.items[s_idx];
                freeOwnedLabels(_.allocator, s.labels);
                _.allocator.free(s.bucket_counts);
            }
            entry.series.deinit(_.allocator);
        }
        _.metrics.deinit(_.allocator);
    }

    fn findMetric(*_, name: []const u8): (*MetricEntry)? {
        let i: usize = 0;
        while (i < _.metrics.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.metrics.items[i].name, name)) {
                return &_.metrics.items[i];
            }
        }
        return null;
    }

    fn getOrCreateSeries(*_, entry: *MetricEntry, labels: []const LabelPair): *SeriesEntry ! PromError {
        let i: usize = 0;
        while (i < entry.series.items.len) : (i += 1) {
            if (labelsMatch(entry.series.items[i].labels, labels)) {
                return &entry.series.items[i];
            }
        }
        const owned_labels = cloneLabels(_.allocator, labels)?;
        let b_len: usize = 0;
        if (entry.metric_type == MetricType.Histogram) {
            b_len = entry.buckets.len + 1;
        }
        const bucket_counts = _.allocator.alloc(u64, b_len) catch {
            freeOwnedLabels(_.allocator, owned_labels);
            return PromError.OutOfMemory;
        };
        if (b_len > 0) {
            @memset(bucket_counts, 0);
        }
        entry.series.append(_.allocator, SeriesEntry{
            labels: owned_labels,
            value: 0.0,
            bucket_counts: bucket_counts,
            hist_sum: 0.0,
            hist_count: 0,
        }) catch {
            freeOwnedLabels(_.allocator, owned_labels);
            _.allocator.free(bucket_counts);
            return PromError.OutOfMemory;
        };
        return &entry.series.items[entry.series.items.len - 1];
    }

    /// Registers a new Counter metric with `name` and `help`.
    pub fn registerCounter(*_, name: []const u8, help: []const u8): void ! PromError {
        if (!isValidMetricName(name)) return PromError.InvalidMetricName;
        if (_.findMetric(name) != null) return PromError.DuplicateMetric;

        const owned_name = dupeBytes(_.allocator, name)?;
        const owned_help = _.allocator.dupe(u8, help) catch {
            _.allocator.free(owned_name);
            return PromError.OutOfMemory;
        };
        const empty_buckets = _.allocator.alloc(f64, 0) catch {
            _.allocator.free(owned_name);
            _.allocator.free(owned_help);
            return PromError.OutOfMemory;
        };
        _.metrics.append(_.allocator, MetricEntry{
            name: owned_name,
            help: owned_help,
            metric_type: MetricType.Counter,
            buckets: empty_buckets,
            series: std.ArrayListUnmanaged(SeriesEntry).empty,
        }) catch {
            _.allocator.free(owned_name);
            _.allocator.free(owned_help);
            _.allocator.free(empty_buckets);
            return PromError.OutOfMemory;
        };
    }

    /// Increments a Counter metric series by `delta` (must be >= 0).
    pub fn incCounter(*_, name: []const u8, labels: []const LabelPair, delta: f64): void ! PromError {
        if (delta < 0.0 || std.math.isNan(delta)) return PromError.NegativeCounterIncrement;
        validateLabels(labels)?;
        const entry = _.findMetric(name) orelse return PromError.InvalidMetricName;
        if (entry.metric_type != MetricType.Counter) return PromError.InvalidMetricName;
        const s = _.getOrCreateSeries(entry, labels)?;
        s.value += delta;
    }

    /// Registers a new Gauge metric with `name` and `help`.
    pub fn registerGauge(*_, name: []const u8, help: []const u8): void ! PromError {
        if (!isValidMetricName(name)) return PromError.InvalidMetricName;
        if (_.findMetric(name) != null) return PromError.DuplicateMetric;

        const owned_name = dupeBytes(_.allocator, name)?;
        const owned_help = _.allocator.dupe(u8, help) catch {
            _.allocator.free(owned_name);
            return PromError.OutOfMemory;
        };
        const empty_buckets = _.allocator.alloc(f64, 0) catch {
            _.allocator.free(owned_name);
            _.allocator.free(owned_help);
            return PromError.OutOfMemory;
        };
        _.metrics.append(_.allocator, MetricEntry{
            name: owned_name,
            help: owned_help,
            metric_type: MetricType.Gauge,
            buckets: empty_buckets,
            series: std.ArrayListUnmanaged(SeriesEntry).empty,
        }) catch {
            _.allocator.free(owned_name);
            _.allocator.free(owned_help);
            _.allocator.free(empty_buckets);
            return PromError.OutOfMemory;
        };
    }

    /// Sets a Gauge metric series to `value`.
    pub fn setGauge(*_, name: []const u8, labels: []const LabelPair, value: f64): void ! PromError {
        validateLabels(labels)?;
        const entry = _.findMetric(name) orelse return PromError.InvalidMetricName;
        if (entry.metric_type != MetricType.Gauge) return PromError.InvalidMetricName;
        const s = _.getOrCreateSeries(entry, labels)?;
        s.value = value;
    }

    /// Adds `delta` (positive or negative) to a Gauge metric series.
    pub fn addGauge(*_, name: []const u8, labels: []const LabelPair, delta: f64): void ! PromError {
        validateLabels(labels)?;
        const entry = _.findMetric(name) orelse return PromError.InvalidMetricName;
        if (entry.metric_type != MetricType.Gauge) return PromError.InvalidMetricName;
        const s = _.getOrCreateSeries(entry, labels)?;
        s.value += delta;
    }

    /// Registers a new Histogram metric with strictly ascending finite `buckets`.
    pub fn registerHistogram(*_, name: []const u8, help: []const u8, buckets: []const f64): void ! PromError {
        if (!isValidMetricName(name)) return PromError.InvalidMetricName;
        if (_.findMetric(name) != null) return PromError.DuplicateMetric;

        let i: usize = 0;
        while (i < buckets.len) : (i += 1) {
            if (std.math.isNan(buckets[i]) || std.math.isPositiveInf(buckets[i]) || std.math.isNegativeInf(buckets[i])) {
                return PromError.UnsortedBuckets;
            }
            if (i > 0 && buckets[i] <= buckets[i - 1]) {
                return PromError.UnsortedBuckets;
            }
        }

        const owned_name = dupeBytes(_.allocator, name)?;
        const owned_help = _.allocator.dupe(u8, help) catch {
            _.allocator.free(owned_name);
            return PromError.OutOfMemory;
        };
        const owned_buckets = _.allocator.dupe(f64, buckets) catch {
            _.allocator.free(owned_name);
            _.allocator.free(owned_help);
            return PromError.OutOfMemory;
        };

        _.metrics.append(_.allocator, MetricEntry{
            name: owned_name,
            help: owned_help,
            metric_type: MetricType.Histogram,
            buckets: owned_buckets,
            series: std.ArrayListUnmanaged(SeriesEntry).empty,
        }) catch {
            _.allocator.free(owned_name);
            _.allocator.free(owned_help);
            _.allocator.free(owned_buckets);
            return PromError.OutOfMemory;
        };
    }

    /// Records an observation `value` into a Histogram metric series, updating cumulative buckets, `_sum`, and `_count`.
    pub fn observeHistogram(*_, name: []const u8, labels: []const LabelPair, value: f64): void ! PromError {
        validateLabels(labels)?;
        const entry = _.findMetric(name) orelse return PromError.InvalidMetricName;
        if (entry.metric_type != MetricType.Histogram) return PromError.InvalidMetricName;
        const s = _.getOrCreateSeries(entry, labels)?;

        let i: usize = 0;
        while (i < entry.buckets.len) : (i += 1) {
            if (value <= entry.buckets[i]) {
                s.bucket_counts[i] += 1;
            }
        }
        s.bucket_counts[entry.buckets.len] += 1;
        s.hist_sum += value;
        s.hist_count += 1;
    }

    /// Serializes all registered metrics into Prometheus `text/plain; version=0.0.4` format.
    pub fn exportText(*_, allocator: std.mem.Allocator): []u8 ! PromError {
        let buf = std.ArrayListUnmanaged(u8).empty;
        errdefer buf.deinit(allocator);

        let m_idx: usize = 0;
        while (m_idx < _.metrics.items.len) : (m_idx += 1) {
            const entry = &_.metrics.items[m_idx];
            buf.appendSlice(allocator, "# HELP ") catch return PromError.OutOfMemory;
            buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
            buf.append(allocator, ' ') catch return PromError.OutOfMemory;
            appendEscapedHelp(&buf, allocator, entry.help)?;
            buf.append(allocator, '\n') catch return PromError.OutOfMemory;

            buf.appendSlice(allocator, "# TYPE ") catch return PromError.OutOfMemory;
            buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
            if (entry.metric_type == MetricType.Counter) {
                buf.appendSlice(allocator, " counter\n") catch return PromError.OutOfMemory;
            } else if (entry.metric_type == MetricType.Gauge) {
                buf.appendSlice(allocator, " gauge\n") catch return PromError.OutOfMemory;
            } else {
                buf.appendSlice(allocator, " histogram\n") catch return PromError.OutOfMemory;
            }

            let s_idx: usize = 0;
            while (s_idx < entry.series.items.len) : (s_idx += 1) {
                const s = &entry.series.items[s_idx];
                if (entry.metric_type == MetricType.Histogram) {
                    let b_idx: usize = 0;
                    let le_buf: [64]u8 = undefined;
                    while (b_idx < entry.buckets.len) : (b_idx += 1) {
                        const le_str = std.fmt.bufPrint(&le_buf, "{d}", .{entry.buckets[b_idx]}) catch return PromError.OutOfMemory;
                        buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
                        buf.appendSlice(allocator, "_bucket") catch return PromError.OutOfMemory;
                        appendLabels(&buf, allocator, s.labels, le_str)?;
                        buf.append(allocator, ' ') catch return PromError.OutOfMemory;
                        appendU64(&buf, allocator, s.bucket_counts[b_idx])?;
                        buf.append(allocator, '\n') catch return PromError.OutOfMemory;
                    }
                    // +Inf bucket
                    buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
                    buf.appendSlice(allocator, "_bucket") catch return PromError.OutOfMemory;
                    appendLabels(&buf, allocator, s.labels, "+Inf")?;
                    buf.append(allocator, ' ') catch return PromError.OutOfMemory;
                    appendU64(&buf, allocator, s.bucket_counts[entry.buckets.len])?;
                    buf.append(allocator, '\n') catch return PromError.OutOfMemory;

                    // _sum
                    buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
                    buf.appendSlice(allocator, "_sum") catch return PromError.OutOfMemory;
                    appendLabels(&buf, allocator, s.labels, null)?;
                    buf.append(allocator, ' ') catch return PromError.OutOfMemory;
                    appendFloat(&buf, allocator, s.hist_sum)?;
                    buf.append(allocator, '\n') catch return PromError.OutOfMemory;

                    // _count
                    buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
                    buf.appendSlice(allocator, "_count") catch return PromError.OutOfMemory;
                    appendLabels(&buf, allocator, s.labels, null)?;
                    buf.append(allocator, ' ') catch return PromError.OutOfMemory;
                    appendU64(&buf, allocator, s.hist_count)?;
                    buf.append(allocator, '\n') catch return PromError.OutOfMemory;
                } else {
                    buf.appendSlice(allocator, entry.name) catch return PromError.OutOfMemory;
                    appendLabels(&buf, allocator, s.labels, null)?;
                    buf.append(allocator, ' ') catch return PromError.OutOfMemory;
                    appendFloat(&buf, allocator, s.value)?;
                    buf.append(allocator, '\n') catch return PromError.OutOfMemory;
                }
            }
        }

        return buf.toOwnedSlice(allocator) catch return PromError.OutOfMemory;
    }
}

fn trimLine(line: []const u8): []const u8 {
    return std.mem.trim(u8, line, " \t\r");
}

fn skipSpaces(s: []const u8, start: usize): usize {
    let i = start;
    const sp_v: PromVec16 = @splat(' ' as u8);
    const tb_v: PromVec16 = @splat('\t' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: PromVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.And, (v == sp_v) | (v == tb_v))) break;
    }
    while (i < s.len && (s[i] == ' ' || s[i] == '\t')) : (i += 1) {}
    return i;
}

fn readWord(s: []const u8, start: usize): WordRead {
    const begin = skipSpaces(s, start);
    let end = begin;
    const sp_v: PromVec16 = @splat(' ' as u8);
    const tb_v: PromVec16 = @splat('\t' as u8);
    while (end + 16 <= s.len) : (end += 16) {
        const v: PromVec16 = s[end .. end + 16][0..16].*;
        if (@reduce(.Or, (v == sp_v) | (v == tb_v))) break;
    }
    while (end < s.len && s[end] != ' ' && s[end] != '\t') : (end += 1) {}
    return WordRead{ word: s[begin..end], next: end };
}

fn unescapeHelpText(allocator: std.mem.Allocator, raw: []const u8): []u8 ! PromError {
    let buf = std.ArrayListUnmanaged(u8).empty;
    errdefer buf.deinit(allocator);
    const bs_v: PromVec16 = @splat('\\' as u8);
    let i: usize = 0;
    let run_start: usize = 0;
    while (i < raw.len) {
        while (i + 16 <= raw.len) {
            const v: PromVec16 = raw[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == bs_v)) break;
            i += 16;
        }
        if (i >= raw.len) break;
        if (raw[i] == '\\' && i + 1 < raw.len) {
            const nxt = raw[i + 1];
            if (nxt == '\\' || nxt == 'n') {
                if (i > run_start) {
                    buf.appendSlice(allocator, raw[run_start..i]) catch return PromError.OutOfMemory;
                }
                buf.append(allocator, if (nxt == '\\') '\\' else '\n') catch return PromError.OutOfMemory;
                i += 2;
                run_start = i;
                continue;
            }
        }
        i += 1;
    }
    if (run_start < raw.len) {
        buf.appendSlice(allocator, raw[run_start..raw.len]) catch return PromError.OutOfMemory;
    }
    return buf.toOwnedSlice(allocator) catch return PromError.OutOfMemory;
}

fn findOrCreateMeta(allocator: std.mem.Allocator, metas: *std.ArrayListUnmanaged(MetaRecord), name: []const u8): *MetaRecord ! PromError {
    let i: usize = 0;
    while (i < metas.items.len) : (i += 1) {
        if (std.mem.eql(u8, metas.items[i].name, name)) {
            return &metas.items[i];
        }
    }
    metas.append(allocator, MetaRecord{
        name: name,
        help: null,
        metric_type: null,
    }) catch return PromError.OutOfMemory;
    return &metas.items[metas.items.len - 1];
}

fn lookupMeta(metas: []const MetaRecord, sample_name: []const u8): (*const MetaRecord)? {
    let i: usize = 0;
    while (i < metas.len) : (i += 1) {
        if (std.mem.eql(u8, metas[i].name, sample_name)) {
            return &metas[i];
        }
    }
    // Fallback for histogram derived suffixes: _bucket, _sum, _count
    let base: ([]const u8)? = null;
    if (std.mem.endsWith(u8, sample_name, "_bucket")) {
        base = sample_name[0 .. sample_name.len - 7];
    } else if (std.mem.endsWith(u8, sample_name, "_count")) {
        base = sample_name[0 .. sample_name.len - 6];
    } else if (std.mem.endsWith(u8, sample_name, "_sum")) {
        base = sample_name[0 .. sample_name.len - 4];
    }
    if (b := base) {
        let j: usize = 0;
        while (j < metas.len) : (j += 1) {
            if (std.mem.eql(u8, metas[j].name, b)) {
                return &metas[j];
            }
        }
    }
    return null;
}

fn parsePromFloat(s: []const u8): f64 ! PromError {
    if (s.len == 0) return PromError.ParseError;
    if (std.mem.eql(u8, s, "+Inf") || std.mem.eql(u8, s, "Inf") || std.mem.eql(u8, s, "+inf") || std.mem.eql(u8, s, "inf")) {
        return std.math.inf(f64);
    }
    if (std.mem.eql(u8, s, "-Inf") || std.mem.eql(u8, s, "-inf")) {
        return -std.math.inf(f64);
    }
    if (std.mem.eql(u8, s, "NaN") || std.mem.eql(u8, s, "nan")) {
        return std.math.nan(f64);
    }
    return std.fmt.parseFloat(f64, s) catch return PromError.ParseError;
}

fn parseLabelList(allocator: std.mem.Allocator, s: []const u8, start: usize): LabelParseResult ! PromError {
    let out = std.ArrayListUnmanaged(LabelPair).empty;
    errdefer {
        let k: usize = 0;
        while (k < out.items.len) : (k += 1) {
            allocator.free(out.items[k].name);
            allocator.free(out.items[k].value);
        }
        out.deinit(allocator);
    }

    let pos = skipSpaces(s, start + 1); // skip '{'
    while (pos < s.len) {
        pos = skipSpaces(s, pos);
        if (pos >= s.len) return PromError.ParseError;
        if (s[pos] == '}') {
            pos += 1;
            const slice = out.toOwnedSlice(allocator) catch return PromError.OutOfMemory;
            return LabelParseResult{ labels: slice, next: pos };
        }

        const key_start = pos;
        while (pos < s.len && s[pos] != '=' && s[pos] != ' ' && s[pos] != '\t' && s[pos] != '}') : (pos += 1) {}
        const key_slice = s[key_start..pos];
        if (!isValidLabelNameForParser(key_slice)) return PromError.ParseError;

        pos = skipSpaces(s, pos);
        if (pos >= s.len || s[pos] != '=') return PromError.ParseError;
        pos += 1;
        pos = skipSpaces(s, pos);
        if (pos >= s.len || s[pos] != '"') return PromError.ParseError;
        pos += 1; // skip opening '"'

        let val_buf = std.ArrayListUnmanaged(u8).empty;
        errdefer val_buf.deinit(allocator);
        let closed = false;
        while (pos < s.len) {
            const c = s[pos];
            if (c == '"') {
                pos += 1;
                closed = true;
                break;
            }
            if (c == '\\') {
                pos += 1;
                if (pos >= s.len) return PromError.ParseError;
                const esc = s[pos];
                if (esc == '\\') {
                    val_buf.append(allocator, '\\') catch return PromError.OutOfMemory;
                } else if (esc == '"') {
                    val_buf.append(allocator, '"') catch return PromError.OutOfMemory;
                } else if (esc == 'n') {
                    val_buf.append(allocator, '\n') catch return PromError.OutOfMemory;
                } else {
                    return PromError.ParseError;
                }
                pos += 1;
            } else {
                val_buf.append(allocator, c) catch return PromError.OutOfMemory;
                pos += 1;
            }
        }
        if (!closed) return PromError.ParseError;

        const owned_val = val_buf.toOwnedSlice(allocator) catch return PromError.OutOfMemory;
        const owned_key = allocator.dupe(u8, key_slice) catch {
            allocator.free(owned_val);
            return PromError.OutOfMemory;
        };
        out.append(allocator, LabelPair{ name: owned_key, value: owned_val }) catch {
            allocator.free(owned_key);
            allocator.free(owned_val);
            return PromError.OutOfMemory;
        };

        pos = skipSpaces(s, pos);
        if (pos < s.len && s[pos] == ',') {
            pos += 1;
        } else if (pos < s.len && s[pos] == '}') {
            // handled at top of loop
        } else {
            return PromError.ParseError;
        }
    }
    return PromError.ParseError;
}

/// Parses Prometheus `text/plain; version=0.0.4` exposition text into an owned slice of `ParsedSample`.
/// Caller must release the returned slice using `freeParsedSamples`.
pub fn parseExposition(allocator: std.mem.Allocator, text: []const u8): []ParsedSample ! PromError {
    let metas = std.ArrayListUnmanaged(MetaRecord).empty;
    defer {
        let i: usize = 0;
        while (i < metas.items.len) : (i += 1) {
            if (h := metas.items[i].help) {
                allocator.free(h);
            }
        }
        metas.deinit(allocator);
    }

    let samples = std.ArrayListUnmanaged(ParsedSample).empty;
    errdefer {
        let i: usize = 0;
        while (i < samples.items.len) : (i += 1) {
            allocator.free(samples.items[i].name);
            let j: usize = 0;
            while (j < samples.items[i].labels.len) : (j += 1) {
                allocator.free(samples.items[i].labels[j].name);
                allocator.free(samples.items[i].labels[j].value);
            }
            allocator.free(samples.items[i].labels);
            if (h := samples.items[i].help) {
                allocator.free(h);
            }
        }
        samples.deinit(allocator);
    }

    let iter = std.mem.splitScalar(u8, text, '\n');
    while (raw_line := iter.next()) {
        const line = trimLine(raw_line);
        if (line.len == 0) continue;

        if (line[0] == '#') {
            const first = readWord(line, 1);
            if (std.mem.eql(u8, first.word, "HELP")) {
                const second = readWord(line, first.next);
                if (second.word.len > 0) {
                    const m = findOrCreateMeta(allocator, &metas, second.word)?;
                    const help_start = skipSpaces(line, second.next);
                    const unescaped = unescapeHelpText(allocator, line[help_start..])?;
                    if (old_h := m.help) {
                        allocator.free(old_h);
                    }
                    m.help = unescaped;
                }
            } else if (std.mem.eql(u8, first.word, "TYPE")) {
                const second = readWord(line, first.next);
                const third = readWord(line, second.next);
                if (second.word.len > 0 && third.word.len > 0) {
                    const m = findOrCreateMeta(allocator, &metas, second.word)?;
                    if (std.mem.eql(u8, third.word, "counter")) {
                        m.metric_type = MetricType.Counter;
                    } else if (std.mem.eql(u8, third.word, "gauge")) {
                        m.metric_type = MetricType.Gauge;
                    } else if (std.mem.eql(u8, third.word, "histogram")) {
                        m.metric_type = MetricType.Histogram;
                    }
                }
            }
            continue;
        }

        // Parse sample line: name[{labels}] value [timestamp]
        let pos: usize = 0;
        while (pos < line.len && line[pos] != '{' && line[pos] != ' ' && line[pos] != '\t') : (pos += 1) {}
        const metric_name = line[0..pos];
        if (!isValidMetricName(metric_name)) return PromError.ParseError;

        let labels: []LabelPair = undefined;
        pos = skipSpaces(line, pos);
        if (pos < line.len && line[pos] == '{') {
            const parsed_lbl = parseLabelList(allocator, line, pos)?;
            labels = parsed_lbl.labels;
            pos = parsed_lbl.next;
        } else {
            labels = allocator.alloc(LabelPair, 0) catch return PromError.OutOfMemory;
        }

        let clean_labels = true;
        errdefer {
            if (clean_labels) {
                let k: usize = 0;
                while (k < labels.len) : (k += 1) {
                    allocator.free(labels[k].name);
                    allocator.free(labels[k].value);
                }
                allocator.free(labels);
            }
        }

        const val_tok = readWord(line, pos);
        if (val_tok.word.len == 0) return PromError.ParseError;
        const val = parsePromFloat(val_tok.word)?;

        const ts_tok = readWord(line, val_tok.next);
        if (ts_tok.word.len > 0) {
            * = std.fmt.parseFloat(f64, ts_tok.word) catch return PromError.ParseError;
            const extra_tok = readWord(line, ts_tok.next);
            if (extra_tok.word.len > 0) return PromError.ParseError;
        }

        const owned_name = allocator.dupe(u8, metric_name) catch return PromError.OutOfMemory;
        errdefer allocator.free(owned_name);

        let help_copy: ([]const u8)? = null;
        let m_type: MetricType? = null;
        if (meta_rec := lookupMeta(metas.items, metric_name)) {
            m_type = meta_rec.metric_type;
            if (h := meta_rec.help) {
                help_copy = allocator.dupe(u8, h) catch return PromError.OutOfMemory;
            }
        }

        samples.append(allocator, ParsedSample{
            name: owned_name,
            labels: labels,
            value: val,
            help: help_copy,
            metric_type: m_type,
        }) catch {
            if (hc := help_copy) {
                allocator.free(hc);
            }
            return PromError.OutOfMemory;
        };
        clean_labels = false;
    }

    return samples.toOwnedSlice(allocator) catch return PromError.OutOfMemory;
}

/// Frees all memory owned by a `ParsedSample` slice returned from `parseExposition`.
pub fn freeParsedSamples(allocator: std.mem.Allocator, samples: []ParsedSample) {
    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        allocator.free(samples[i].name);
        let j: usize = 0;
        while (j < samples[i].labels.len) : (j += 1) {
            allocator.free(samples[i].labels[j].name);
            allocator.free(samples[i].labels[j].value);
        }
        allocator.free(samples[i].labels);
        if (h := samples[i].help) {
            allocator.free(h);
        }
    }
    allocator.free(samples);
}
