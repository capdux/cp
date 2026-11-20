// Source: cp/lib/otlp/json.cp (OTLP/HTTP JSON exporter)
const std = @import("std");
import { writeHexLower } from "./context.cp";
import {
    AnyValue,
    KeyValue,
    MetricDataPoint,
    MetricKind,
    OtlpLogRecord,
    OtlpMetric,
    OtlpSpan,
    ResourceInfo,
} from "./model.cp";

fn appendJsonEscaped(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8) !void {
    try out.append(alloc, '"');
    const digits = "0123456789abcdef";
    for (c of s) {
        switch (c) {
            '"' => try out.appendSlice(alloc, "\\\""),
            '\\' => try out.appendSlice(alloc, "\\\\"),
            '\x08' => try out.appendSlice(alloc, "\\b"),
            '\x0c' => try out.appendSlice(alloc, "\\f"),
            '\n' => try out.appendSlice(alloc, "\\n"),
            '\r' => try out.appendSlice(alloc, "\\r"),
            '\t' => try out.appendSlice(alloc, "\\t"),
            else => {
                if (c < @as(u8, 0x20)) {
                    try out.appendSlice(alloc, "\\u00");
                    try out.append(alloc, digits[c >> 4]);
                    try out.append(alloc, digits[c & @as(u8, 0x0f)]);
                } else {
                    try out.append(alloc, c);
                }
            },
        }
    }
    try out.append(alloc, '"');
}

fn appendHexString(alloc: std.mem.Allocator, out: *std.ArrayList(u8), bytes: []const u8) !void {
    try out.append(alloc, '"');
    const start_len = out.items.len;
    try out.resize(alloc, start_len + bytes.len * 2);
    writeHexLower(out.items[start_len..], bytes);
    try out.append(alloc, '"');
}

fn appendU64String(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: u64) !void {
    let buf: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "\"{d}\"", .{val}) catch unreachable;
    try out.appendSlice(alloc, s);
}

fn appendI64String(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: i64) !void {
    let buf: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "\"{d}\"", .{val}) catch unreachable;
    try out.appendSlice(alloc, s);
}

fn appendUInt(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: u64) !void {
    let buf: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "{d}", .{val}) catch unreachable;
    try out.appendSlice(alloc, s);
}

fn appendFloat(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: f64) !void {
    let buf: [64]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "{d}", .{val}) catch unreachable;
    try out.appendSlice(alloc, s);
}

fn appendAnyValue(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: AnyValue) !void {
    try out.append(alloc, '{');
    switch (val) {
        .string_value => |sv| {
            try out.appendSlice(alloc, "\"stringValue\":");
            try appendJsonEscaped(alloc, out, sv);
        },
        .bool_value => |bv| {
            try out.appendSlice(alloc, "\"boolValue\":");
            try out.appendSlice(alloc, if (bv) "true" else "false");
        },
        .int_value => |iv| {
            try out.appendSlice(alloc, "\"intValue\":");
            try appendI64String(alloc, out, iv);
        },
        .double_value => |dv| {
            try out.appendSlice(alloc, "\"doubleValue\":");
            try appendFloat(alloc, out, dv);
        },
    }
    try out.append(alloc, '}');
}

fn appendAttributes(alloc: std.mem.Allocator, out: *std.ArrayList(u8), attrs: []const KeyValue) !void {
    try out.append(alloc, '[');
    let i: usize = 0;
    while (i < attrs.len) : (i += 1) {
        if (i > 0) {
            try out.append(alloc, ',');
        }
        try out.appendSlice(alloc, "{\"key\":");
        try appendJsonEscaped(alloc, out, attrs[i].key);
        try out.appendSlice(alloc, ",\"value\":");
        try appendAnyValue(alloc, out, attrs[i].value);
        try out.append(alloc, '}');
    }
    try out.append(alloc, ']');
}

fn appendResource(alloc: std.mem.Allocator, out: *std.ArrayList(u8), resource: *const ResourceInfo) !void {
    try out.appendSlice(alloc, "{\"attributes\":[{\"key\":\"service.name\",\"value\":{\"stringValue\":");
    try appendJsonEscaped(alloc, out, resource.service_name);
    try out.appendSlice(alloc, "}}");

    if (ver := resource.service_version) {
        try out.appendSlice(alloc, ",{\"key\":\"service.version\",\"value\":{\"stringValue\":");
        try appendJsonEscaped(alloc, out, ver);
        try out.appendSlice(alloc, "}}");
    }

    for (kv of resource.attributes) {
        try out.appendSlice(alloc, ",{\"key\":");
        try appendJsonEscaped(alloc, out, kv.key);
        try out.appendSlice(alloc, ",\"value\":");
        try appendAnyValue(alloc, out, kv.value);
        try out.append(alloc, '}');
    }

    try out.appendSlice(alloc, "]}");
}

pub fn encodeExportTraceRequestJson(
    alloc: std.mem.Allocator,
    resource: *const ResourceInfo,
    spans: []const OtlpSpan,
) ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "{\"resourceSpans\":[{\"resource\":");
    try appendResource(alloc, &out, resource);
    try out.appendSlice(alloc, ",\"scopeSpans\":[{\"scope\":{\"name\":\"cap.otlp\"},\"spans\":[");

    let i: usize = 0;
    while (i < spans.len) : (i += 1) {
        if (i > 0) {
            try out.append(alloc, ',');
        }
        const sp = spans[i];
        try out.appendSlice(alloc, "{\"traceId\":");
        try appendHexString(alloc, &out, &sp.trace_id);
        try out.appendSlice(alloc, ",\"spanId\":");
        try appendHexString(alloc, &out, &sp.span_id);
        if (pid := sp.parent_span_id) {
            try out.appendSlice(alloc, ",\"parentSpanId\":");
            try appendHexString(alloc, &out, &pid);
        }
        try out.appendSlice(alloc, ",\"name\":");
        try appendJsonEscaped(alloc, &out, sp.name);
        try out.appendSlice(alloc, ",\"kind\":");
        try appendUInt(alloc, &out, @intFromEnum(sp.kind));
        try out.appendSlice(alloc, ",\"startTimeUnixNano\":");
        try appendU64String(alloc, &out, sp.start_time_unix_nano);
        try out.appendSlice(alloc, ",\"endTimeUnixNano\":");
        try appendU64String(alloc, &out, sp.end_time_unix_nano);

        if (sp.attributes.len > 0) {
            try out.appendSlice(alloc, ",\"attributes\":");
            try appendAttributes(alloc, &out, sp.attributes);
        }

        if (sp.events.len > 0) {
            try out.appendSlice(alloc, ",\"events\":[");
            let ei: usize = 0;
            while (ei < sp.events.len) : (ei += 1) {
                if (ei > 0) {
                    try out.append(alloc, ',');
                }
                const ev = sp.events[ei];
                try out.appendSlice(alloc, "{\"timeUnixNano\":");
                try appendU64String(alloc, &out, ev.time_unix_nano);
                try out.appendSlice(alloc, ",\"name\":");
                try appendJsonEscaped(alloc, &out, ev.name);
                if (ev.attributes.len > 0) {
                    try out.appendSlice(alloc, ",\"attributes\":");
                    try appendAttributes(alloc, &out, ev.attributes);
                }
                try out.append(alloc, '}');
            }
            try out.append(alloc, ']');
        }

        if (sp.links.len > 0) {
            try out.appendSlice(alloc, ",\"links\":[");
            let li: usize = 0;
            while (li < sp.links.len) : (li += 1) {
                if (li > 0) {
                    try out.append(alloc, ',');
                }
                const lk = sp.links[li];
                try out.appendSlice(alloc, "{\"traceId\":");
                try appendHexString(alloc, &out, &lk.trace_id);
                try out.appendSlice(alloc, ",\"spanId\":");
                try appendHexString(alloc, &out, &lk.span_id);
                if (ts := lk.trace_state) {
                    try out.appendSlice(alloc, ",\"traceState\":");
                    try appendJsonEscaped(alloc, &out, ts);
                }
                if (lk.attributes.len > 0) {
                    try out.appendSlice(alloc, ",\"attributes\":");
                    try appendAttributes(alloc, &out, lk.attributes);
                }
                try out.append(alloc, '}');
            }
            try out.append(alloc, ']');
        }

        try out.appendSlice(alloc, ",\"status\":{\"code\":");
        try appendUInt(alloc, &out, @intFromEnum(sp.status.code));
        if (msg := sp.status.message) {
            try out.appendSlice(alloc, ",\"message\":");
            try appendJsonEscaped(alloc, &out, msg);
        }
        try out.appendSlice(alloc, "}}");
    }

    try out.appendSlice(alloc, "]}]}]}");
    return out.toOwnedSlice(alloc);
}

fn appendNumberDataPoint(alloc: std.mem.Allocator, out: *std.ArrayList(u8), dp: MetricDataPoint) !void {
    try out.appendSlice(alloc, "{\"timeUnixNano\":");
    try appendU64String(alloc, out, dp.time_unix_nano);
    if (iv := dp.value_int) {
        try out.appendSlice(alloc, ",\"asInt\":");
        try appendI64String(alloc, out, iv);
    } else if (dv := dp.value_double) {
        try out.appendSlice(alloc, ",\"asDouble\":");
        try appendFloat(alloc, out, dv);
    }
    if (dp.attributes.len > 0) {
        try out.appendSlice(alloc, ",\"attributes\":");
        try appendAttributes(alloc, out, dp.attributes);
    }
    try out.append(alloc, '}');
}

fn appendHistogramDataPoint(alloc: std.mem.Allocator, out: *std.ArrayList(u8), dp: MetricDataPoint) !void {
    try out.appendSlice(alloc, "{\"timeUnixNano\":");
    try appendU64String(alloc, out, dp.time_unix_nano);
    if (c := dp.count) {
        try out.appendSlice(alloc, ",\"count\":");
        try appendU64String(alloc, out, c);
    }
    if (s := dp.sum) {
        try out.appendSlice(alloc, ",\"sum\":");
        try appendFloat(alloc, out, s);
    }
    try out.appendSlice(alloc, ",\"bucketCounts\":[");
    let bi: usize = 0;
    while (bi < dp.bucket_counts.len) : (bi += 1) {
        if (bi > 0) {
            try out.append(alloc, ',');
        }
        try appendU64String(alloc, out, dp.bucket_counts[bi]);
    }
    try out.appendSlice(alloc, "],\"explicitBounds\":[");
    let ei: usize = 0;
    while (ei < dp.explicit_bounds.len) : (ei += 1) {
        if (ei > 0) {
            try out.append(alloc, ',');
        }
        try appendFloat(alloc, out, dp.explicit_bounds[ei]);
    }
    try out.append(alloc, ']');
    if (dp.attributes.len > 0) {
        try out.appendSlice(alloc, ",\"attributes\":");
        try appendAttributes(alloc, out, dp.attributes);
    }
    try out.append(alloc, '}');
}

pub fn encodeExportMetricsRequestJson(
    alloc: std.mem.Allocator,
    resource: *const ResourceInfo,
    metrics: []const OtlpMetric,
) ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "{\"resourceMetrics\":[{\"resource\":");
    try appendResource(alloc, &out, resource);
    try out.appendSlice(alloc, ",\"scopeMetrics\":[{\"scope\":{\"name\":\"cap.otlp\"},\"metrics\":[");

    let i: usize = 0;
    while (i < metrics.len) : (i += 1) {
        if (i > 0) {
            try out.append(alloc, ',');
        }
        const m = metrics[i];
        try out.appendSlice(alloc, "{\"name\":");
        try appendJsonEscaped(alloc, &out, m.name);
        if (m.description.len > 0) {
            try out.appendSlice(alloc, ",\"description\":");
            try appendJsonEscaped(alloc, &out, m.description);
        }
        if (m.unit.len > 0) {
            try out.appendSlice(alloc, ",\"unit\":");
            try appendJsonEscaped(alloc, &out, m.unit);
        }

        switch (m.kind) {
            MetricKind.COUNTER => {
                try out.appendSlice(alloc, ",\"sum\":{\"aggregationTemporality\":2,\"isMonotonic\":");
                try out.appendSlice(alloc, if (m.is_monotonic) "true" else "false");
                try out.appendSlice(alloc, ",\"dataPoints\":[");
                let di: usize = 0;
                while (di < m.data_points.len) : (di += 1) {
                    if (di > 0) {
                        try out.append(alloc, ',');
                    }
                    try appendNumberDataPoint(alloc, &out, m.data_points[di]);
                }
                try out.appendSlice(alloc, "]}");
            },
            MetricKind.GAUGE => {
                try out.appendSlice(alloc, ",\"gauge\":{\"dataPoints\":[");
                let di: usize = 0;
                while (di < m.data_points.len) : (di += 1) {
                    if (di > 0) {
                        try out.append(alloc, ',');
                    }
                    try appendNumberDataPoint(alloc, &out, m.data_points[di]);
                }
                try out.appendSlice(alloc, "]}");
            },
            MetricKind.HISTOGRAM => {
                try out.appendSlice(alloc, ",\"histogram\":{\"aggregationTemporality\":2,\"dataPoints\":[");
                let di: usize = 0;
                while (di < m.data_points.len) : (di += 1) {
                    if (di > 0) {
                        try out.append(alloc, ',');
                    }
                    try appendHistogramDataPoint(alloc, &out, m.data_points[di]);
                }
                try out.appendSlice(alloc, "]}");
            },
        }
        try out.append(alloc, '}');
    }

    try out.appendSlice(alloc, "]}]}]}");
    return out.toOwnedSlice(alloc);
}

pub fn encodeExportLogsRequestJson(
    alloc: std.mem.Allocator,
    resource: *const ResourceInfo,
    logs: []const OtlpLogRecord,
) ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "{\"resourceLogs\":[{\"resource\":");
    try appendResource(alloc, &out, resource);
    try out.appendSlice(alloc, ",\"scopeLogs\":[{\"scope\":{\"name\":\"cap.otlp\"},\"logRecords\":[");

    let i: usize = 0;
    while (i < logs.len) : (i += 1) {
        if (i > 0) {
            try out.append(alloc, ',');
        }
        const lr = logs[i];
        try out.appendSlice(alloc, "{\"timeUnixNano\":");
        try appendU64String(alloc, &out, lr.time_unix_nano);
        try out.appendSlice(alloc, ",\"severityNumber\":");
        try appendUInt(alloc, &out, @intFromEnum(lr.severity_number));
        try out.appendSlice(alloc, ",\"severityText\":");
        try appendJsonEscaped(alloc, &out, lr.severity_text);
        try out.appendSlice(alloc, ",\"body\":");
        try appendAnyValue(alloc, &out, lr.body);
        if (lr.attributes.len > 0) {
            try out.appendSlice(alloc, ",\"attributes\":");
            try appendAttributes(alloc, &out, lr.attributes);
        }
        if (tid := lr.trace_id) {
            try out.appendSlice(alloc, ",\"traceId\":");
            try appendHexString(alloc, &out, &tid);
        }
        if (sid := lr.span_id) {
            try out.appendSlice(alloc, ",\"spanId\":");
            try appendHexString(alloc, &out, &sid);
        }
        try out.append(alloc, '}');
    }

    try out.appendSlice(alloc, "]}]}]}");
    return out.toOwnedSlice(alloc);
}
