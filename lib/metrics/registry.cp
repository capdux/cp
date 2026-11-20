// Source: cp/lib/metrics/registry.cp (Prometheus Text Exposition 0.0.4 & JSON Registry)
// Pure CP observability metrics registry with Prometheus 0.0.4 text exposition and JSON snapshot export.

const std = @import("std");
import {
    MetricsError,
    MetricLabel,
    MetricNameValidator,
    Counter,
    Gauge,
} from "./primitives.cp";
import {
    HistogramState,
    MetricsHistogram,
} from "./histogram.cp";

/// Discriminant for registered metric kinds.
pub enum MetricKind: u8 {
    Counter = 0,
    Gauge = 1,
    Histogram = 2,
}

struct OwnedLabel {
    key: []u8 = [],
    value: []u8 = [],
}

struct MetricEntry {
    kind: MetricKind = MetricKind.Counter,
    name: []u8 = [],
    help: []u8 = [],
    labels: []OwnedLabel = [],
    counter_ptr: (*std.atomic.Value(u64)!)? = null,
    gauge_ptr: (*std.atomic.Value(u64)!)? = null,
    hist_ptr: (*HistogramState!)? = null,
}

fn freeOwnedLabels(alloc: std.mem.Allocator, labels: []OwnedLabel) {
    let i: usize = 0;
    while (i < labels.len) : (i += 1) {
        alloc.free(labels[i].key);
        alloc.free(labels[i].value);
    }
    if (labels.len > 0) {
        alloc.free(labels);
    }
}

fn dupeLabels(alloc: std.mem.Allocator, labels: []const MetricLabel): ([]OwnedLabel) ! MetricsError {
    if (labels.len == 0) return [];
    let out = alloc.alloc(OwnedLabel, labels.len) catch return MetricsError.OutOfMemory;
    let initialized: usize = 0;
    errdefer {
        let k: usize = 0;
        while (k < initialized) : (k += 1) {
            alloc.free(out[k].key);
            alloc.free(out[k].value);
        }
        alloc.free(out);
    }

    let i: usize = 0;
    while (i < labels.len) : (i += 1) {
        const k_dup = alloc.dupe(u8, labels[i].key) catch return MetricsError.OutOfMemory;
        const v_dup = alloc.dupe(u8, labels[i].value) catch {
            alloc.free(k_dup);
            return MetricsError.OutOfMemory;
        };
        out[i] = OwnedLabel{
            key: k_dup,
            value: v_dup,
        };
        initialized += 1;
    }
    return out;
}

fn labelsMatchOrderInsensitive(owned: []const OwnedLabel, given: []const MetricLabel): bool {
    if (owned.len != given.len) return false;
    let i: usize = 0;
    while (i < owned.len) : (i += 1) {
        let found = false;
        let j: usize = 0;
        while (j < given.len) : (j += 1) {
            if (std.mem.eql(u8, owned[i].key, given[j].key) &&
                std.mem.eql(u8, owned[i].value, given[j].value))
            {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

fn appendPrometheusHelpEscaped(out: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): void ! MetricsError {
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const ch = text[i];
        if (ch == '\\') {
            out.appendSlice(alloc, "\\\\") catch return MetricsError.OutOfMemory;
        } else if (ch == '\n') {
            out.appendSlice(alloc, "\\n") catch return MetricsError.OutOfMemory;
        } else {
            out.append(alloc, ch) catch return MetricsError.OutOfMemory;
        }
    }
}

fn appendPrometheusLabelValEscaped(out: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): void ! MetricsError {
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const ch = text[i];
        if (ch == '\\') {
            out.appendSlice(alloc, "\\\\") catch return MetricsError.OutOfMemory;
        } else if (ch == '"') {
            out.appendSlice(alloc, "\\\"") catch return MetricsError.OutOfMemory;
        } else if (ch == '\n') {
            out.appendSlice(alloc, "\\n") catch return MetricsError.OutOfMemory;
        } else {
            out.append(alloc, ch) catch return MetricsError.OutOfMemory;
        }
    }
}

fn appendJsonStringEscaped(out: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): void ! MetricsError {
    out.append(alloc, '"') catch return MetricsError.OutOfMemory;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const ch = text[i];
        if (ch == '"') {
            out.appendSlice(alloc, "\\\"") catch return MetricsError.OutOfMemory;
        } else if (ch == '\\') {
            out.appendSlice(alloc, "\\\\") catch return MetricsError.OutOfMemory;
        } else if (ch == '\n') {
            out.appendSlice(alloc, "\\n") catch return MetricsError.OutOfMemory;
        } else if (ch == '\r') {
            out.appendSlice(alloc, "\\r") catch return MetricsError.OutOfMemory;
        } else if (ch == '\t') {
            out.appendSlice(alloc, "\\t") catch return MetricsError.OutOfMemory;
        } else if (ch < 0x20) {
            let hex_buf: [6]u8 = undefined;
            const s = std.fmt.bufPrint(&hex_buf, "\\u{x:0>4}", .{ch}) catch return MetricsError.OutOfMemory;
            out.appendSlice(alloc, s) catch return MetricsError.OutOfMemory;
        } else {
            out.append(alloc, ch) catch return MetricsError.OutOfMemory;
        }
    }
    out.append(alloc, '"') catch return MetricsError.OutOfMemory;
}

fn appendPromFloat(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: f64): void ! MetricsError {
    if (std.math.isNan(val)) {
        out.appendSlice(alloc, "NaN") catch return MetricsError.OutOfMemory;
    } else if (std.math.isPositiveInf(val)) {
        out.appendSlice(alloc, "+Inf") catch return MetricsError.OutOfMemory;
    } else if (std.math.isNegativeInf(val)) {
        out.appendSlice(alloc, "-Inf") catch return MetricsError.OutOfMemory;
    } else {
        let buf: [64]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{val}) catch return MetricsError.OutOfMemory;
        out.appendSlice(alloc, s) catch return MetricsError.OutOfMemory;
    }
}

fn appendJsonFloat(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: f64): void ! MetricsError {
    if (std.math.isNan(val)) {
        out.appendSlice(alloc, "\"NaN\"") catch return MetricsError.OutOfMemory;
    } else if (std.math.isPositiveInf(val)) {
        out.appendSlice(alloc, "\"+Inf\"") catch return MetricsError.OutOfMemory;
    } else if (std.math.isNegativeInf(val)) {
        out.appendSlice(alloc, "\"-Inf\"") catch return MetricsError.OutOfMemory;
    } else {
        let buf: [64]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{val}) catch return MetricsError.OutOfMemory;
        out.appendSlice(alloc, s) catch return MetricsError.OutOfMemory;
    }
}

fn appendPromU64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): void ! MetricsError {
    let buf: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "{d}", .{val}) catch return MetricsError.OutOfMemory;
    out.appendSlice(alloc, s) catch return MetricsError.OutOfMemory;
}

fn appendPromLabelBlock(out: *std.ArrayList(u8), alloc: std.mem.Allocator, labels: []const OwnedLabel): void ! MetricsError {
    if (labels.len > 0) {
        out.append(alloc, '{') catch return MetricsError.OutOfMemory;
        let i: usize = 0;
        while (i < labels.len) : (i += 1) {
            if (i > 0) {
                out.append(alloc, ',') catch return MetricsError.OutOfMemory;
            }
            out.appendSlice(alloc, labels[i].key) catch return MetricsError.OutOfMemory;
            out.appendSlice(alloc, "=\"") catch return MetricsError.OutOfMemory;
            appendPrometheusLabelValEscaped(out, alloc, labels[i].value)?;
            out.append(alloc, '"') catch return MetricsError.OutOfMemory;
        }
        out.append(alloc, '}') catch return MetricsError.OutOfMemory;
    }
}

/// Thread-safe, ownership-safe registry for Counters, Gauges, and Histograms.
/// Supports Prometheus Text Exposition Format 0.0.4 and structured JSON snapshot export.
pub struct MetricsRegistry {
    allocator: std.mem.Allocator = undefined,
    entries: std.ArrayList(MetricEntry) = .empty,

    /// Initializes an empty `MetricsRegistry`.
    pub static fn init(alloc: std.mem.Allocator): MetricsRegistry {
        return MetricsRegistry{
            allocator: alloc,
            entries: .empty,
        };
    }

    /// Releases all registered metric names, help strings, labels, and backing states.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const e = _.entries.items[i];
            _.allocator.free(e.name);
            _.allocator.free(e.help);
            freeOwnedLabels(_.allocator, e.labels);
            if (p := e.counter_ptr) {
                unsafe {
                    _.allocator.destroy(p);
                }
            }
            if (p := e.gauge_ptr) {
                unsafe {
                    _.allocator.destroy(p);
                }
            }
            if (p := e.hist_ptr) {
                unsafe {
                    p.deinit();
                    _.allocator.destroy(p);
                }
            }
        }
        _.entries.deinit(_.allocator);
    }

    /// Returns the number of registered metric time series.
    pub fn len(): usize {
        return _.entries.items.len;
    }

    fn checkConflict(name: []const u8, kind: MetricKind, labels: []const MetricLabel): void ! MetricsError {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const e = _.entries.items[i];
            if (std.mem.eql(u8, e.name, name)) {
                if (e.kind != kind) {
                    return MetricsError.DuplicateMetric;
                }
                if (labelsMatchOrderInsensitive(e.labels, labels)) {
                    return MetricsError.DuplicateMetric;
                }
            }
        }
    }

    /// Validates and registers a `Counter` metric series, returning an atomic `Counter` handle.
    pub fn registerCounter(*_, name: []const u8, help: []const u8, labels: []const MetricLabel): Counter ! MetricsError {
        MetricNameValidator.validateMetricName(name)?;
        MetricNameValidator.validateLabels(labels)?;
        _.checkConflict(name, MetricKind.Counter, labels)?;

        const name_owned = _.allocator.dupe(u8, name) catch return MetricsError.OutOfMemory;
        errdefer _.allocator.free(name_owned);

        const help_owned = _.allocator.dupe(u8, help) catch return MetricsError.OutOfMemory;
        errdefer _.allocator.free(help_owned);

        const labels_owned = dupeLabels(_.allocator, labels)?;
        errdefer freeOwnedLabels(_.allocator, labels_owned);

        let cell_ptr = _.allocator.create(std.atomic.Value(u64)) catch return MetricsError.OutOfMemory;
        unsafe {
            cell_ptr.* = std.atomic.Value(u64).init(0);
        }
        errdefer _.allocator.destroy(cell_ptr);

        _.entries.append(_.allocator, MetricEntry{
            kind: MetricKind.Counter,
            name: name_owned,
            help: help_owned,
            labels: labels_owned,
            counter_ptr: cell_ptr,
            gauge_ptr: null,
            hist_ptr: null,
        }) catch return MetricsError.OutOfMemory;

        return Counter.fromAtomicPtr(cell_ptr);
    }

    /// Validates and registers a `Gauge` metric series, returning an atomic `Gauge` handle.
    pub fn registerGauge(*_, name: []const u8, help: []const u8, labels: []const MetricLabel): Gauge ! MetricsError {
        MetricNameValidator.validateMetricName(name)?;
        MetricNameValidator.validateLabels(labels)?;
        _.checkConflict(name, MetricKind.Gauge, labels)?;

        const name_owned = _.allocator.dupe(u8, name) catch return MetricsError.OutOfMemory;
        errdefer _.allocator.free(name_owned);

        const help_owned = _.allocator.dupe(u8, help) catch return MetricsError.OutOfMemory;
        errdefer _.allocator.free(help_owned);

        const labels_owned = dupeLabels(_.allocator, labels)?;
        errdefer freeOwnedLabels(_.allocator, labels_owned);

        let cell_ptr = _.allocator.create(std.atomic.Value(u64)) catch return MetricsError.OutOfMemory;
        unsafe {
            cell_ptr.* = std.atomic.Value(u64).init(0);
        }
        errdefer _.allocator.destroy(cell_ptr);

        _.entries.append(_.allocator, MetricEntry{
            kind: MetricKind.Gauge,
            name: name_owned,
            help: help_owned,
            labels: labels_owned,
            counter_ptr: null,
            gauge_ptr: cell_ptr,
            hist_ptr: null,
        }) catch return MetricsError.OutOfMemory;

        return Gauge.fromAtomicPtr(cell_ptr);
    }

    /// Validates and registers a `MetricsHistogram` series.
    /// Rejects reserved `"le"` label key and invalid/non-monotonic bounds.
    /// When `bounds` is empty, `DEFAULT_HTTP_BUCKETS` is used.
    pub fn registerHistogram(
        *_,
        name: []const u8,
        help: []const u8,
        labels: []const MetricLabel,
        bounds: []const f64,
    ): MetricsHistogram ! MetricsError {
        MetricNameValidator.validateMetricName(name)?;
        MetricNameValidator.validateLabels(labels)?;
        let li: usize = 0;
        while (li < labels.len) : (li += 1) {
            if (std.mem.eql(u8, labels[li].key, "le")) {
                return MetricsError.InvalidLabelName;
            }
        }
        _.checkConflict(name, MetricKind.Histogram, labels)?;

        let state_val = HistogramState.init(_.allocator, bounds)?;
        errdefer state_val.deinit();

        let state_ptr = _.allocator.create(HistogramState) catch return MetricsError.OutOfMemory;
        unsafe {
            state_ptr.* = state_val;
        }
        errdefer _.allocator.destroy(state_ptr);

        const name_owned = _.allocator.dupe(u8, name) catch return MetricsError.OutOfMemory;
        errdefer _.allocator.free(name_owned);

        const help_owned = _.allocator.dupe(u8, help) catch return MetricsError.OutOfMemory;
        errdefer _.allocator.free(help_owned);

        const labels_owned = dupeLabels(_.allocator, labels)?;
        errdefer freeOwnedLabels(_.allocator, labels_owned);

        _.entries.append(_.allocator, MetricEntry{
            kind: MetricKind.Histogram,
            name: name_owned,
            help: help_owned,
            labels: labels_owned,
            counter_ptr: null,
            gauge_ptr: null,
            hist_ptr: state_ptr,
        }) catch return MetricsError.OutOfMemory;

        return MetricsHistogram.fromStatePtr(state_ptr);
    }

    fn exportEntryPrometheus(out: *std.ArrayList(u8), alloc: std.mem.Allocator, e: MetricEntry): void ! MetricsError {
        if (e.kind == MetricKind.Counter) {
            out.appendSlice(alloc, e.name) catch return MetricsError.OutOfMemory;
            appendPromLabelBlock(out, alloc, e.labels)?;
            out.append(alloc, ' ') catch return MetricsError.OutOfMemory;
            let val: u64 = 0;
            if (p := e.counter_ptr) {
                unsafe {
                    val = p.load(.monotonic);
                }
            }
            appendPromU64(out, alloc, val)?;
            out.append(alloc, '\n') catch return MetricsError.OutOfMemory;
        } else if (e.kind == MetricKind.Gauge) {
            out.appendSlice(alloc, e.name) catch return MetricsError.OutOfMemory;
            appendPromLabelBlock(out, alloc, e.labels)?;
            out.append(alloc, ' ') catch return MetricsError.OutOfMemory;
            let val: f64 = 0.0;
            if (p := e.gauge_ptr) {
                unsafe {
                    const bits = p.load(.acquire);
                    val = @as(f64, @bitCast(bits));
                }
            }
            appendPromFloat(out, alloc, val)?;
            out.append(alloc, '\n') catch return MetricsError.OutOfMemory;
        } else if (hp := e.hist_ptr) {
            unsafe {
                const h_bounds = hp.bounds();
                const h_counts = hp.bucketCounts();
                let b_idx: usize = 0;
                while (b_idx < h_bounds.len) : (b_idx += 1) {
                    out.appendSlice(alloc, e.name) catch return MetricsError.OutOfMemory;
                    out.appendSlice(alloc, "_bucket{") catch return MetricsError.OutOfMemory;
                    let li: usize = 0;
                    while (li < e.labels.len) : (li += 1) {
                        out.appendSlice(alloc, e.labels[li].key) catch return MetricsError.OutOfMemory;
                        out.appendSlice(alloc, "=\"") catch return MetricsError.OutOfMemory;
                        appendPrometheusLabelValEscaped(out, alloc, e.labels[li].value)?;
                        out.appendSlice(alloc, "\",") catch return MetricsError.OutOfMemory;
                    }
                    out.appendSlice(alloc, "le=\"") catch return MetricsError.OutOfMemory;
                    appendPromFloat(out, alloc, h_bounds[b_idx])?;
                    out.appendSlice(alloc, "\"} ") catch return MetricsError.OutOfMemory;
                    appendPromU64(out, alloc, h_counts[b_idx])?;
                    out.append(alloc, '\n') catch return MetricsError.OutOfMemory;
                }

                // +Inf bucket
                out.appendSlice(alloc, e.name) catch return MetricsError.OutOfMemory;
                out.appendSlice(alloc, "_bucket{") catch return MetricsError.OutOfMemory;
                let li2: usize = 0;
                while (li2 < e.labels.len) : (li2 += 1) {
                    out.appendSlice(alloc, e.labels[li2].key) catch return MetricsError.OutOfMemory;
                    out.appendSlice(alloc, "=\"") catch return MetricsError.OutOfMemory;
                    appendPrometheusLabelValEscaped(out, alloc, e.labels[li2].value)?;
                    out.appendSlice(alloc, "\",") catch return MetricsError.OutOfMemory;
                }
                out.appendSlice(alloc, "le=\"+Inf\"} ") catch return MetricsError.OutOfMemory;
                appendPromU64(out, alloc, h_counts[h_bounds.len])?;
                out.append(alloc, '\n') catch return MetricsError.OutOfMemory;

                // _sum
                out.appendSlice(alloc, e.name) catch return MetricsError.OutOfMemory;
                out.appendSlice(alloc, "_sum") catch return MetricsError.OutOfMemory;
                appendPromLabelBlock(out, alloc, e.labels)?;
                out.append(alloc, ' ') catch return MetricsError.OutOfMemory;
                appendPromFloat(out, alloc, hp.sum())?;
                out.append(alloc, '\n') catch return MetricsError.OutOfMemory;

                // _count
                out.appendSlice(alloc, e.name) catch return MetricsError.OutOfMemory;
                out.appendSlice(alloc, "_count") catch return MetricsError.OutOfMemory;
                appendPromLabelBlock(out, alloc, e.labels)?;
                out.append(alloc, ' ') catch return MetricsError.OutOfMemory;
                appendPromU64(out, alloc, hp.count())?;
                out.append(alloc, '\n') catch return MetricsError.OutOfMemory;
            }
        }
    }

    /// Exports all registered metrics in Prometheus Text Exposition Format 0.0.4.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn exportPrometheus(alloc: std.mem.Allocator): ([]u8) ! MetricsError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        const n = _.entries.items.len;
        if (n == 0) {
            return out.toOwnedSlice(alloc) catch return MetricsError.OutOfMemory;
        }

        let visited = alloc.alloc(bool, n) catch return MetricsError.OutOfMemory;
        defer alloc.free(visited);
        @memset(visited, false);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (visited[i]) continue;
            const head = _.entries.items[i];

            // Emit # HELP and # TYPE header once per metric family
            out.appendSlice(alloc, "# HELP ") catch return MetricsError.OutOfMemory;
            out.appendSlice(alloc, head.name) catch return MetricsError.OutOfMemory;
            out.append(alloc, ' ') catch return MetricsError.OutOfMemory;
            appendPrometheusHelpEscaped(&out, alloc, head.help)?;
            out.append(alloc, '\n') catch return MetricsError.OutOfMemory;

            out.appendSlice(alloc, "# TYPE ") catch return MetricsError.OutOfMemory;
            out.appendSlice(alloc, head.name) catch return MetricsError.OutOfMemory;
            const type_str: []const u8 = switch (head.kind) {
                .Counter => " counter\n",
                .Gauge => " gauge\n",
                .Histogram => " histogram\n",
            };
            out.appendSlice(alloc, type_str) catch return MetricsError.OutOfMemory;

            // Emit all series belonging to this metric family in registration order
            let j: usize = i;
            while (j < n) : (j += 1) {
                if (!visited[j] && std.mem.eql(u8, _.entries.items[j].name, head.name)) {
                    visited[j] = true;
                    MetricsRegistry.exportEntryPrometheus(&out, alloc, _.entries.items[j])?;
                }
            }
        }

        return out.toOwnedSlice(alloc) catch return MetricsError.OutOfMemory;
    }

    fn appendJsonLabels(out: *std.ArrayList(u8), alloc: std.mem.Allocator, labels: []const OwnedLabel): void ! MetricsError {
        out.append(alloc, '{') catch return MetricsError.OutOfMemory;
        let i: usize = 0;
        while (i < labels.len) : (i += 1) {
            if (i > 0) {
                out.append(alloc, ',') catch return MetricsError.OutOfMemory;
            }
            appendJsonStringEscaped(out, alloc, labels[i].key)?;
            out.append(alloc, ':') catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(out, alloc, labels[i].value)?;
        }
        out.append(alloc, '}') catch return MetricsError.OutOfMemory;
    }

    /// Exports a structured JSON snapshot of all registered counters, gauges, and histograms.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn exportJson(alloc: std.mem.Allocator): ([]u8) ! MetricsError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"counters\":[") catch return MetricsError.OutOfMemory;
        let first_c = true;
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const e = _.entries.items[i];
            if (e.kind != MetricKind.Counter) continue;
            if (!first_c) {
                out.append(alloc, ',') catch return MetricsError.OutOfMemory;
            }
            first_c = false;
            out.appendSlice(alloc, "{\"name\":") catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(&out, alloc, e.name)?;
            out.appendSlice(alloc, ",\"help\":") catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(&out, alloc, e.help)?;
            out.appendSlice(alloc, ",\"labels\":") catch return MetricsError.OutOfMemory;
            MetricsRegistry.appendJsonLabels(&out, alloc, e.labels)?;
            out.appendSlice(alloc, ",\"value\":") catch return MetricsError.OutOfMemory;
            let val: u64 = 0;
            if (p := e.counter_ptr) {
                unsafe {
                    val = p.load(.monotonic);
                }
            }
            appendPromU64(&out, alloc, val)?;
            out.append(alloc, '}') catch return MetricsError.OutOfMemory;
        }

        out.appendSlice(alloc, "],\"gauges\":[") catch return MetricsError.OutOfMemory;
        let first_g = true;
        i = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const e = _.entries.items[i];
            if (e.kind != MetricKind.Gauge) continue;
            if (!first_g) {
                out.append(alloc, ',') catch return MetricsError.OutOfMemory;
            }
            first_g = false;
            out.appendSlice(alloc, "{\"name\":") catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(&out, alloc, e.name)?;
            out.appendSlice(alloc, ",\"help\":") catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(&out, alloc, e.help)?;
            out.appendSlice(alloc, ",\"labels\":") catch return MetricsError.OutOfMemory;
            MetricsRegistry.appendJsonLabels(&out, alloc, e.labels)?;
            out.appendSlice(alloc, ",\"value\":") catch return MetricsError.OutOfMemory;
            let val: f64 = 0.0;
            if (p := e.gauge_ptr) {
                unsafe {
                    const bits = p.load(.acquire);
                    val = @as(f64, @bitCast(bits));
                }
            }
            appendJsonFloat(&out, alloc, val)?;
            out.append(alloc, '}') catch return MetricsError.OutOfMemory;
        }

        out.appendSlice(alloc, "],\"histograms\":[") catch return MetricsError.OutOfMemory;
        let first_h = true;
        i = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const e = _.entries.items[i];
            if (e.kind != MetricKind.Histogram) continue;
            if (!first_h) {
                out.append(alloc, ',') catch return MetricsError.OutOfMemory;
            }
            first_h = false;
            out.appendSlice(alloc, "{\"name\":") catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(&out, alloc, e.name)?;
            out.appendSlice(alloc, ",\"help\":") catch return MetricsError.OutOfMemory;
            appendJsonStringEscaped(&out, alloc, e.help)?;
            out.appendSlice(alloc, ",\"labels\":") catch return MetricsError.OutOfMemory;
            MetricsRegistry.appendJsonLabels(&out, alloc, e.labels)?;

            if (hp := e.hist_ptr) {
                unsafe {
                    out.appendSlice(alloc, ",\"count\":") catch return MetricsError.OutOfMemory;
                    appendPromU64(&out, alloc, hp.count())?;
                    out.appendSlice(alloc, ",\"sum\":") catch return MetricsError.OutOfMemory;
                    appendJsonFloat(&out, alloc, hp.sum())?;
                    out.appendSlice(alloc, ",\"buckets\":[") catch return MetricsError.OutOfMemory;
                    const h_bounds = hp.bounds();
                    const h_counts = hp.bucketCounts();
                    let b_idx: usize = 0;
                    while (b_idx < h_bounds.len) : (b_idx += 1) {
                        if (b_idx > 0) {
                            out.append(alloc, ',') catch return MetricsError.OutOfMemory;
                        }
                        out.appendSlice(alloc, "{\"le\":") catch return MetricsError.OutOfMemory;
                        appendJsonFloat(&out, alloc, h_bounds[b_idx])?;
                        out.appendSlice(alloc, ",\"count\":") catch return MetricsError.OutOfMemory;
                        appendPromU64(&out, alloc, h_counts[b_idx])?;
                        out.append(alloc, '}') catch return MetricsError.OutOfMemory;
                    }
                    if (h_bounds.len > 0) {
                        out.append(alloc, ',') catch return MetricsError.OutOfMemory;
                    }
                    out.appendSlice(alloc, "{\"le\":\"+Inf\",\"count\":") catch return MetricsError.OutOfMemory;
                    appendPromU64(&out, alloc, h_counts[h_bounds.len])?;
                    out.appendSlice(alloc, "}]") catch return MetricsError.OutOfMemory;
                }
            }
            out.append(alloc, '}') catch return MetricsError.OutOfMemory;
        }

        out.appendSlice(alloc, "]}") catch return MetricsError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return MetricsError.OutOfMemory;
    }
}
