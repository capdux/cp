// Source: cp/lib/histogram/export.cp (Histogram Summary & Prometheus Exposition Formatting)
// Pure CP zero-C-dependency formatting helpers for `HdrHistogram`.

const std = @import("std");
import { HdrHistogram, HistogramError } from "./hdr.cp";

/// Format a human-readable summary string for `hist`. Caller owns returned slice.
pub fn formatSummary(alloc: std.mem.Allocator, hist: HdrHistogram): []u8 ! HistogramError {
    return hist.formatSummary(alloc);
}

/// Format Prometheus histogram exposition lines for `hist`. Caller owns returned slice.
pub fn formatPrometheusBuckets(
    alloc: std.mem.Allocator,
    hist: HdrHistogram,
    metric_name: []const u8,
    le_bounds: []const u64,
) : []u8 ! HistogramError {
    return hist.formatPrometheusBuckets(alloc, metric_name, le_bounds);
}
