// Source: cp/lib/analytics/json.cp (RFC 8259 JSON Array & NDJSON Batch Serializer for Telemetry Events)
// Zero external dependencies; uses std.ArrayList(u8).empty with Zig 0.17 allocator conventions.

const std = @import("std");
import {
    AnalyticsError,
    AnalyticsEvent,
    PropKind,
} from "./types.cp";

fn appendJsonString(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: []const u8): void! {
    buf.append(alloc, '"') catch return AnalyticsError.OutOfMemory;
    const hex = "0123456789abcdef";
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"') {
            buf.appendSlice(alloc, "\\\"") catch return AnalyticsError.OutOfMemory;
        } else if (c == '\\') {
            buf.appendSlice(alloc, "\\\\") catch return AnalyticsError.OutOfMemory;
        } else if (c == '\x08') {
            buf.appendSlice(alloc, "\\b") catch return AnalyticsError.OutOfMemory;
        } else if (c == '\x0c') {
            buf.appendSlice(alloc, "\\f") catch return AnalyticsError.OutOfMemory;
        } else if (c == '\n') {
            buf.appendSlice(alloc, "\\n") catch return AnalyticsError.OutOfMemory;
        } else if (c == '\r') {
            buf.appendSlice(alloc, "\\r") catch return AnalyticsError.OutOfMemory;
        } else if (c == '\t') {
            buf.appendSlice(alloc, "\\t") catch return AnalyticsError.OutOfMemory;
        } else if (c < 0x20) {
            let esc: [6]u8 = [ '\\', 'u', '0', '0', hex[(c >> 4) & 0x0f], hex[c & 0x0f] ];
            buf.appendSlice(alloc, &esc) catch return AnalyticsError.OutOfMemory;
        } else {
            buf.append(alloc, c) catch return AnalyticsError.OutOfMemory;
        }
    }
    buf.append(alloc, '"') catch return AnalyticsError.OutOfMemory;
}

fn appendInt(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: i64): void! {
    let tmp: [32]u8 = undefined;
    const formatted = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return AnalyticsError.OutOfMemory;
    buf.appendSlice(alloc, formatted) catch return AnalyticsError.OutOfMemory;
}

fn appendFloat(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: f64): void! {
    if (std.math.isNan(val) || std.math.isInf(val)) {
        buf.appendSlice(alloc, "null") catch return AnalyticsError.OutOfMemory;
        return;
    }
    let tmp: [64]u8 = undefined;
    const formatted = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return AnalyticsError.OutOfMemory;
    buf.appendSlice(alloc, formatted) catch return AnalyticsError.OutOfMemory;
}

fn appendEventJson(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), ev: AnalyticsEvent): void! {
    buf.appendSlice(alloc, "{\"name\":") catch return AnalyticsError.OutOfMemory;
    try appendJsonString(alloc, buf, ev.name);

    buf.appendSlice(alloc, ",\"distinct_id\":") catch return AnalyticsError.OutOfMemory;
    try appendJsonString(alloc, buf, ev.distinct_id);

    buf.appendSlice(alloc, ",\"session_id\":") catch return AnalyticsError.OutOfMemory;
    try appendJsonString(alloc, buf, ev.session_id);

    buf.appendSlice(alloc, ",\"timestamp_ms\":") catch return AnalyticsError.OutOfMemory;
    try appendInt(alloc, buf, ev.timestamp_ms);

    buf.appendSlice(alloc, ",\"properties\":{") catch return AnalyticsError.OutOfMemory;
    let i: usize = 0;
    while (i < ev.props.len) : (i += 1) {
        if (i > 0) {
            buf.append(alloc, ',') catch return AnalyticsError.OutOfMemory;
        }
        const prop = ev.props[i];
        try appendJsonString(alloc, buf, prop.key);
        buf.append(alloc, ':') catch return AnalyticsError.OutOfMemory;

        if (prop.value.kind == .String) {
            try appendJsonString(alloc, buf, prop.value.str_val);
        } else if (prop.value.kind == .Int) {
            try appendInt(alloc, buf, prop.value.int_val);
        } else if (prop.value.kind == .Float) {
            try appendFloat(alloc, buf, prop.value.float_val);
        } else {
            if (prop.value.bool_val) {
                buf.appendSlice(alloc, "true") catch return AnalyticsError.OutOfMemory;
            } else {
                buf.appendSlice(alloc, "false") catch return AnalyticsError.OutOfMemory;
            }
        }
    }
    buf.appendSlice(alloc, "}}") catch return AnalyticsError.OutOfMemory;
}

/// Serializes a slice of AnalyticsEvent into a compact RFC 8259 JSON array.
pub fn encodeBatchJson(alloc: std.mem.Allocator, events: []const AnalyticsEvent): ([]u8)! {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    buf.append(alloc, '[') catch return AnalyticsError.OutOfMemory;
    let i: usize = 0;
    while (i < events.len) : (i += 1) {
        if (i > 0) {
            buf.append(alloc, ',') catch return AnalyticsError.OutOfMemory;
        }
        try appendEventJson(alloc, &buf, events[i]);
    }
    buf.append(alloc, ']') catch return AnalyticsError.OutOfMemory;
    return buf.toOwnedSlice(alloc) catch return AnalyticsError.OutOfMemory;
}

/// Serializes a slice of AnalyticsEvent into NDJSON (one compact JSON object per line).
pub fn encodeBatchNdjson(alloc: std.mem.Allocator, events: []const AnalyticsEvent): ([]u8)! {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    while (i < events.len) : (i += 1) {
        try appendEventJson(alloc, &buf, events[i]);
        buf.append(alloc, '\n') catch return AnalyticsError.OutOfMemory;
    }
    return buf.toOwnedSlice(alloc) catch return AnalyticsError.OutOfMemory;
}
