// Source: cp/lib/perf/trace.cp (Chrome Trace Event JSON exporter & summary table formatter)

import std from "std";
import { PerfEvent } from "./types.cp";
import { PerfStatSummary } from "./stats.cp";

fn appendJsonEscaped(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), input: []const u8): !void {
    const hex = "0123456789abcdef";
    for (ch of input) {
        switch (ch) {
            '"' => try buf.appendSlice(alloc, "\\\""),
            '\\' => try buf.appendSlice(alloc, "\\\\"),
            '\n' => try buf.appendSlice(alloc, "\\n"),
            '\r' => try buf.appendSlice(alloc, "\\r"),
            '\t' => try buf.appendSlice(alloc, "\\t"),
            0x08 => try buf.appendSlice(alloc, "\\b"),
            0x0c => try buf.appendSlice(alloc, "\\f"),
            else => {
                if (ch < 0x20) {
                    try buf.appendSlice(alloc, "\\u00");
                    try buf.append(alloc, hex[ch >> 4]);
                    try buf.append(alloc, hex[ch & 0x0f]);
                } else {
                    try buf.append(alloc, ch);
                }
            },
        }
    }
}

fn appendMicros(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), ns: u64): !void {
    const us = ns / 1000;
    const rem = ns % 1000;
    if (rem == 0) {
        const s = try std.fmt.allocPrint(alloc, "{d}", .{us});
        defer alloc.free(s);
        try buf.appendSlice(alloc, s);
    } else {
        const s = try std.fmt.allocPrint(alloc, "{d}.{d:0>3}", .{ us, rem });
        defer alloc.free(s);
        try buf.appendSlice(alloc, s);
    }
}

pub fn exportChromeTraceJson(alloc: std.mem.Allocator, events: []const PerfEvent): ![]u8 {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);

    try buf.appendSlice(alloc, "{\"traceEvents\":[");

    let i: usize = 0;
    for (ev of events) {
        if (i > 0) {
            try buf.append(alloc, ',');
        }
        i += 1;
        try buf.appendSlice(alloc, "{\"name\":\"");
        try appendJsonEscaped(alloc, &buf, ev.name);
        try buf.appendSlice(alloc, "\",\"cat\":\"");
        const cat = if (ev.category.len > 0) ev.category else "perf";
        try appendJsonEscaped(alloc, &buf, cat);
        try buf.appendSlice(alloc, "\",\"ph\":\"");
        try buf.append(alloc, ev.phase.toChar());
        try buf.appendSlice(alloc, "\",\"ts\":");
        try appendMicros(alloc, &buf, ev.ts_ns);

        if (ev.phase == .Complete) {
            try buf.appendSlice(alloc, ",\"dur\":");
            try appendMicros(alloc, &buf, ev.dur_ns);
        }

        const pid_tid = try std.fmt.allocPrint(alloc, ",\"pid\":{d},\"tid\":{d}", .{ ev.pid, ev.tid });
        defer alloc.free(pid_tid);
        try buf.appendSlice(alloc, pid_tid);

        if (ev.phase == .Instant) {
            try buf.appendSlice(alloc, ",\"s\":\"t\"");
        } else if (ev.phase == .Counter) {
            const args_str = try std.fmt.allocPrint(alloc, ",\"args\":{{\"value\":{d}}}", .{ev.value});
            defer alloc.free(args_str);
            try buf.appendSlice(alloc, args_str);
        }

        try buf.append(alloc, '}');
    }

    try buf.appendSlice(alloc, "]}");
    return try buf.toOwnedSlice(alloc);
}

pub fn formatSummaryTable(alloc: std.mem.Allocator, summaries: []const PerfStatSummary): ![]u8 {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);

    try buf.appendSlice(alloc, "Name\tCount\tTotal(ns)\tMin(ns)\tAvg(ns)\tMax(ns)\tP95(ns)\tP99(ns)\n");
    for (s of summaries) {
        const line = try std.fmt.allocPrint(
            alloc,
            "{s}\t{d}\t{d}\t{d}\t{d}\t{d}\t{d}\t{d}\n",
            .{ s.name, s.count, s.total_ns, s.min_ns, s.avg_ns, s.max_ns, s.p95_ns, s.p99_ns },
        );
        defer alloc.free(line);
        try buf.appendSlice(alloc, line);
    }
    return try buf.toOwnedSlice(alloc);
}
