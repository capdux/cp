// Source: Absorbed from /data/dev/bun/src/test_runner/pretty_format.cp | Synced: 2026-11-22T12:33:04Z
// ============================================================================
// lib/test_runner/reporter.cp — PrettyFormat & TAP v13 / JUnit XML / JSON Reporters
// ============================================================================

const std = @import("std");
import { TestStatus, TestCase, TestRunSummary } from "./suite";

pub enum ReportFormat: u8 {
    Tap = 0,
    JUnit = 1,
    Json = 2,
}

pub struct PrettyFormatOptions {
    indent_size: usize = 2,
    max_depth: usize = 8,
    quote_strings: bool = true,
}

pub enum PrettyKind: u8 {
    Null = 0,
    Bool = 1,
    Int = 2,
    Float = 3,
    String = 4,
    Array = 5,
    Object = 6,
}

pub struct PrettyField {
    key: []const u8 = "",
    value: PrettyNode = PrettyNode{},
}

/// Heterogeneous value node for dynamic `PrettyFormat.formatNode` rendering.
pub struct PrettyNode {
    kind: PrettyKind = .Null,
    bool_val: bool = false,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    str_val: []const u8 = "",
    items: []const PrettyNode = [],
    fields: []const PrettyField = [],

    pub static fn nullNode(): PrettyNode {
        return PrettyNode{ kind: .Null };
    }

    pub static fn boolNode(v: bool): PrettyNode {
        return PrettyNode{ kind: .Bool, bool_val: v };
    }

    pub static fn intNode(v: i64): PrettyNode {
        return PrettyNode{ kind: .Int, int_val: v };
    }

    pub static fn floatNode(v: f64): PrettyNode {
        return PrettyNode{ kind: .Float, float_val: v };
    }

    pub static fn strNode(v: []const u8): PrettyNode {
        return PrettyNode{ kind: .String, str_val: v };
    }

    pub static fn arrayNode(items: []const PrettyNode): PrettyNode {
        return PrettyNode{ kind: .Array, items: items };
    }

    pub static fn objectNode(fields: []const PrettyField): PrettyNode {
        return PrettyNode{ kind: .Object, fields: fields };
    }
}

fn appendSpaces(allocator: std.mem.Allocator, out: *std.ArrayList(u8), count: usize): ! {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        out.append(allocator, ' ')?;
    }
}

fn appendQuotedString(allocator: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): ! {
    out.append(allocator, '"')?;
    for (c of s) {
        switch (c) {
            '"' => out.appendSlice(allocator, "\\\"")?,
            '\\' => out.appendSlice(allocator, "\\\\")?,
            '\n' => out.appendSlice(allocator, "\\n")?,
            '\r' => out.appendSlice(allocator, "\\r")?,
            '\t' => out.appendSlice(allocator, "\\t")?,
            else => out.append(allocator, c)?,
        }
    }
    out.append(allocator, '"')?;
}

/// Indented pretty-formatter for strings, numbers, booleans, slices, and key-value objects.
pub struct PrettyFormat {
    pub static fn formatString(allocator: std.mem.Allocator, s: []const u8, quote: bool): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        if (quote) {
            appendQuotedString(allocator, &out, s)?;
        } else {
            out.appendSlice(allocator, s)?;
        }
        return out.toOwnedSlice(allocator);
    }

    pub static fn formatInt(allocator: std.mem.Allocator, v: i64): ([]u8)! {
        return std.fmt.allocPrint(allocator, "{d}", .{v});
    }

    pub static fn formatFloat(allocator: std.mem.Allocator, v: f64): ([]u8)! {
        return std.fmt.allocPrint(allocator, "{d}", .{v});
    }

    pub static fn formatBool(allocator: std.mem.Allocator, v: bool): ([]u8)! {
        return allocator.dupe(u8, if (v) "true" else "false");
    }

    static fn writeNode(
        allocator: std.mem.Allocator,
        out: *std.ArrayList(u8),
        node: PrettyNode,
        opts: PrettyFormatOptions,
        depth: usize,
    ): ! {
        switch (node.kind) {
            .Null => out.appendSlice(allocator, "null")?,
            .Bool => out.appendSlice(allocator, if (node.bool_val) "true" else "false")?,
            .Int => {
                let buf: [32]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{node.int_val}) catch "0";
                out.appendSlice(allocator, s)?;
            },
            .Float => {
                let buf: [64]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{node.float_val}) catch "0";
                out.appendSlice(allocator, s)?;
            },
            .String => {
                if (opts.quote_strings) {
                    appendQuotedString(allocator, out, node.str_val)?;
                } else {
                    out.appendSlice(allocator, node.str_val)?;
                }
            },
            .Array => {
                if (node.items.len == 0) {
                    out.appendSlice(allocator, "[]")?;
                    return;
                }
                if (depth >= opts.max_depth) {
                    out.appendSlice(allocator, "[Array]")?;
                    return;
                }
                out.appendSlice(allocator, "[\n")?;
                let i: usize = 0;
                while (i < node.items.len) : (i += 1) {
                    appendSpaces(allocator, out, (depth + 1) * opts.indent_size)?;
                    PrettyFormat.writeNode(allocator, out, node.items[i], opts, depth + 1)?;
                    if (i + 1 < node.items.len) out.append(allocator, ',')?;
                    out.append(allocator, '\n')?;
                }
                appendSpaces(allocator, out, depth * opts.indent_size)?;
                out.append(allocator, ']')?;
            },
            .Object => {
                if (node.fields.len == 0) {
                    out.appendSlice(allocator, "{}")?;
                    return;
                }
                if (depth >= opts.max_depth) {
                    out.appendSlice(allocator, "[Object]")?;
                    return;
                }
                out.appendSlice(allocator, "{\n")?;
                let i: usize = 0;
                while (i < node.fields.len) : (i += 1) {
                    const f = node.fields[i];
                    appendSpaces(allocator, out, (depth + 1) * opts.indent_size)?;
                    out.appendSlice(allocator, f.key)?;
                    out.appendSlice(allocator, ": ")?;
                    PrettyFormat.writeNode(allocator, out, f.value, opts, depth + 1)?;
                    if (i + 1 < node.fields.len) out.append(allocator, ',')?;
                    out.append(allocator, '\n')?;
                }
                appendSpaces(allocator, out, depth * opts.indent_size)?;
                out.append(allocator, '}')?;
            },
        }
    }

    pub static fn formatNode(
        allocator: std.mem.Allocator,
        node: PrettyNode,
        opts: PrettyFormatOptions,
    ): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        PrettyFormat.writeNode(allocator, &out, node, opts, 0)?;
        return out.toOwnedSlice(allocator);
    }

    pub static fn formatObject(
        allocator: std.mem.Allocator,
        fields: []const PrettyField,
        opts: PrettyFormatOptions,
    ): ([]u8)! {
        return PrettyFormat.formatNode(allocator, PrettyNode.objectNode(fields), opts);
    }

    pub static fn formatSlice(
        allocator: std.mem.Allocator,
        items: []const PrettyNode,
        opts: PrettyFormatOptions,
    ): ([]u8)! {
        return PrettyFormat.formatNode(allocator, PrettyNode.arrayNode(items), opts);
    }

    /// Comptime generic formatter for arbitrary CP values (strings, ints, floats, bools, slices, structs).
    pub static fn formatValue(allocator: std.mem.Allocator, val: auto): ([]u8)! {
        return PrettyFormat.formatValueWithOptions(allocator, val, PrettyFormatOptions{}, 0);
    }

    static fn formatValueWithOptions(
        allocator: std.mem.Allocator,
        val: auto,
        opts: PrettyFormatOptions,
        depth: usize,
    ): ([]u8)! {
        let is_handled = false;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        zig {
            const T = @TypeOf(val);
            const ti = @typeInfo(T);
            switch (ti) {
                .bool => {
                    is_handled = true;
                    try out.appendSlice(allocator, if (val) "true" else "false");
                },
                .int, .comptime_int, .float, .comptime_float => {
                    is_handled = true;
                    const s = try std.fmt.allocPrint(allocator, "{d}", .{val});
                    defer allocator.free(s);
                    try out.appendSlice(allocator, s);
                },
                .optional => {
                    is_handled = true;
                    if (val) |inner| {
                        const s = try PrettyFormat.formatValueWithOptions(allocator, inner, opts, depth);
                        defer allocator.free(s);
                        try out.appendSlice(allocator, s);
                    } else {
                        try out.appendSlice(allocator, "null");
                    }
                },
                .@"enum" => {
                    is_handled = true;
                    try out.appendSlice(allocator, @tagName(val));
                },
                .pointer => |ptr| {
                    if (ptr.size == .slice) {
                        is_handled = true;
                        if (ptr.child == u8) {
                            if (opts.quote_strings) {
                                try appendQuotedString(allocator, &out, val);
                            } else {
                                try out.appendSlice(allocator, val);
                            }
                        } else if (val.len == 0) {
                            try out.appendSlice(allocator, "[]");
                        } else if (depth >= opts.max_depth) {
                            try out.appendSlice(allocator, "[Slice]");
                        } else {
                            try out.appendSlice(allocator, "[\n");
                            for (val, 0..) |item, idx| {
                                try appendSpaces(allocator, &out, (depth + 1) * opts.indent_size);
                                const item_s = try PrettyFormat.formatValueWithOptions(allocator, item, opts, depth + 1);
                                defer allocator.free(item_s);
                                try out.appendSlice(allocator, item_s);
                                if (idx + 1 < val.len) try out.append(allocator, ',');
                                try out.append(allocator, '\n');
                            }
                            try appendSpaces(allocator, &out, depth * opts.indent_size);
                            try out.append(allocator, ']');
                        }
                    }
                },
                .@"struct" => |st| {
                    is_handled = true;
                    if (st.fields.len == 0) {
                        try out.appendSlice(allocator, "{}");
                    } else if (depth >= opts.max_depth) {
                        try out.appendSlice(allocator, "[Object]");
                    } else {
                        try out.appendSlice(allocator, "{\n");
                        inline for (st.fields, 0..) |fld, idx| {
                            try appendSpaces(allocator, &out, (depth + 1) * opts.indent_size);
                            try out.appendSlice(allocator, fld.name);
                            try out.appendSlice(allocator, ": ");
                            const fv = try PrettyFormat.formatValueWithOptions(allocator, @field(val, fld.name), opts, depth + 1);
                            defer allocator.free(fv);
                            try out.appendSlice(allocator, fv);
                            if (idx + 1 < st.fields.len) try out.append(allocator, ',');
                            try out.append(allocator, '\n');
                        }
                        try appendSpaces(allocator, &out, depth * opts.indent_size);
                        try out.append(allocator, '}');
                    }
                },
                else => {},
            }
        }
        if (is_handled) {
            return out.toOwnedSlice(allocator);
        }
        out.deinit(allocator);
        return std.fmt.allocPrint(allocator, "{any}", .{val});
    }
}

// ============================================================================
// TestReporter — TAP v13, JUnit XML & JSON Structured CI Reporters
// ============================================================================

fn appendXmlEscaped(allocator: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): ! {
    for (c of s) {
        switch (c) {
            '&' => out.appendSlice(allocator, "&amp;")?,
            '<' => out.appendSlice(allocator, "&lt;")?,
            '>' => out.appendSlice(allocator, "&gt;")?,
            '"' => out.appendSlice(allocator, "&quot;")?,
            '\'' => out.appendSlice(allocator, "&apos;")?,
            else => out.append(allocator, c)?,
        }
    }
}

fn appendJsonEscaped(allocator: std.mem.Allocator, out: *std.ArrayList(u8), s: []const u8): ! {
    out.append(allocator, '"')?;
    for (c of s) {
        if (c == '"') {
            out.appendSlice(allocator, "\\\"")?;
        } else if (c == '\\') {
            out.appendSlice(allocator, "\\\\")?;
        } else if (c == '\n') {
            out.appendSlice(allocator, "\\n")?;
        } else if (c == '\r') {
            out.appendSlice(allocator, "\\r")?;
        } else if (c == '\t') {
            out.appendSlice(allocator, "\\t")?;
        } else if (c < 0x20) {
            const hex = "0123456789abcdef";
            let seq: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF]];
            out.appendSlice(allocator, seq[0..6])?;
        } else {
            out.append(allocator, c)?;
        }
    }
    out.append(allocator, '"')?;
}

fn formatSecondsFromNs(ns: u64, buf: []u8): []const u8 {
    const secs = (ns as f64) / 1000000000.0;
    return std.fmt.bufPrint(buf, "{d:.6}", .{secs}) catch "0.000000";
}

/// Multi-format CI test report generator supporting TAP v13, JUnit XML, and JSON.
pub struct TestReporter {
    /// Formats `summary` as TAP version 13 output.
    pub static fn formatTap(allocator: std.mem.Allocator, summary: TestRunSummary): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        const header = std.fmt.allocPrint(allocator, "TAP version 13\n1..{d}\n", .{summary.total})?;
        defer allocator.free(header);
        out.appendSlice(allocator, header)?;

        let idx: usize = 0;
        while (idx < summary.results.len) : (idx += 1) {
            const tc = summary.results[idx];
            const num = idx + 1;
            const desc = if (tc.full_name.len > 0) tc.full_name else tc.name;
            switch (tc.status) {
                .Passed => {
                    const line = std.fmt.allocPrint(allocator, "ok {d} - {s}\n", .{ num, desc })?;
                    defer allocator.free(line);
                    out.appendSlice(allocator, line)?;
                },
                .Skipped => {
                    const line = std.fmt.allocPrint(allocator, "ok {d} - {s} # SKIP\n", .{ num, desc })?;
                    defer allocator.free(line);
                    out.appendSlice(allocator, line)?;
                },
                .Todo => {
                    const line = std.fmt.allocPrint(allocator, "ok {d} - {s} # TODO\n", .{ num, desc })?;
                    defer allocator.free(line);
                    out.appendSlice(allocator, line)?;
                },
                .Failed, .TimedOut, .Pending => {
                    const line = std.fmt.allocPrint(allocator, "not ok {d} - {s}\n", .{ num, desc })?;
                    defer allocator.free(line);
                    out.appendSlice(allocator, line)?;
                    if (tc.failure_message.len > 0) {
                        out.appendSlice(allocator, "  ---\n  message: ")?;
                        appendJsonEscaped(allocator, &out, tc.failure_message)?;
                        out.appendSlice(allocator, "\n  severity: fail\n  ...\n")?;
                    }
                },
            }
        }
        return out.toOwnedSlice(allocator);
    }

    /// Formats `summary` as standard JUnit XML with full XML entity escaping.
    pub static fn formatJUnit(
        allocator: std.mem.Allocator,
        summary: TestRunSummary,
        suite_name: []const u8,
    ): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        let time_buf: [32]u8 = undefined;
        const total_time = formatSecondsFromNs(summary.duration_ns, &time_buf);
        const fail_total = summary.failed + summary.timed_out;
        const skip_total = summary.skipped + summary.todo;

        out.appendSlice(allocator, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")?;
        out.appendSlice(allocator, "<testsuites name=\"")?;
        appendXmlEscaped(allocator, &out, suite_name)?;
        const hdr_attrs = std.fmt.allocPrint(
            allocator,
            "\" tests=\"{d}\" failures=\"{d}\" skipped=\"{d}\" time=\"{s}\">\n",
            .{ summary.total, fail_total, skip_total, total_time },
        )?;
        defer allocator.free(hdr_attrs);
        out.appendSlice(allocator, hdr_attrs)?;

        out.appendSlice(allocator, "  <testsuite name=\"")?;
        appendXmlEscaped(allocator, &out, suite_name)?;
        out.appendSlice(allocator, hdr_attrs)?;

        for (tc of summary.results) {
            let tc_time_buf: [32]u8 = undefined;
            const tc_time = formatSecondsFromNs(tc.duration_ns, &tc_time_buf);
            const cls = if (tc.suite_name.len > 0) tc.suite_name else suite_name;

            out.appendSlice(allocator, "    <testcase classname=\"")?;
            appendXmlEscaped(allocator, &out, cls)?;
            out.appendSlice(allocator, "\" name=\"")?;
            appendXmlEscaped(allocator, &out, tc.name)?;
            out.appendSlice(allocator, "\" time=\"")?;
            out.appendSlice(allocator, tc_time)?;

            if (tc.status == .Passed) {
                out.appendSlice(allocator, "\" />\n")?;
            } else if (tc.status == .Skipped || tc.status == .Todo) {
                out.appendSlice(allocator, "\">\n      <skipped />\n    </testcase>\n")?;
            } else {
                const msg = if (tc.failure_message.len > 0) tc.failure_message else "Test failed";
                out.appendSlice(allocator, "\">\n      <failure message=\"")?;
                appendXmlEscaped(allocator, &out, msg)?;
                out.appendSlice(allocator, "\">")?;
                appendXmlEscaped(allocator, &out, msg)?;
                out.appendSlice(allocator, "</failure>\n    </testcase>\n")?;
            }
        }

        out.appendSlice(allocator, "  </testsuite>\n</testsuites>\n")?;
        return out.toOwnedSlice(allocator);
    }

    /// Formats `summary` as structured JSON report text.
    pub static fn formatJson(allocator: std.mem.Allocator, summary: TestRunSummary): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        const head = std.fmt.allocPrint(
            allocator,
            "{{\"seed\":{d},\"order\":\"{s}\",\"summary\":{{\"total\":{d},\"passed\":{d},\"failed\":{d},\"skipped\":{d},\"todo\":{d},\"timedOut\":{d},\"durationNs\":{d},\"success\":{s}}},\"tests\":[",
            .{
                summary.seed,
                summary.order_mode.label(),
                summary.total,
                summary.passed,
                summary.failed,
                summary.skipped,
                summary.todo,
                summary.timed_out,
                summary.duration_ns,
                if (summary.isSuccess()) "true" else "false",
            },
        )?;
        defer allocator.free(head);
        out.appendSlice(allocator, head)?;

        let idx: usize = 0;
        while (idx < summary.results.len) : (idx += 1) {
            if (idx > 0) out.append(allocator, ',')?;
            const tc = summary.results[idx];
            out.appendSlice(allocator, "{\"name\":")?;
            appendJsonEscaped(allocator, &out, tc.name)?;
            out.appendSlice(allocator, ",\"fullName\":")?;
            appendJsonEscaped(allocator, &out, tc.full_name)?;
            out.appendSlice(allocator, ",\"suite\":")?;
            appendJsonEscaped(allocator, &out, tc.suite_name)?;
            out.appendSlice(allocator, ",\"status\":\"")?;
            out.appendSlice(allocator, tc.status.label())?;
            const mid = std.fmt.allocPrint(
                allocator,
                "\",\"durationNs\":{d},\"assertions\":{d},\"failureMessage\":",
                .{ tc.duration_ns, tc.assertions },
            )?;
            defer allocator.free(mid);
            out.appendSlice(allocator, mid)?;
            appendJsonEscaped(allocator, &out, tc.failure_message)?;
            out.append(allocator, '}')?;
        }

        out.appendSlice(allocator, "]}\n")?;
        return out.toOwnedSlice(allocator);
    }

    pub static fn format(
        allocator: std.mem.Allocator,
        summary: TestRunSummary,
        fmt: ReportFormat,
        suite_name: []const u8,
    ): ([]u8)! {
        return switch (fmt) {
            .Tap => TestReporter.formatTap(allocator, summary),
            .JUnit => TestReporter.formatJUnit(allocator, summary, suite_name),
            .Json => TestReporter.formatJson(allocator, summary),
        };
    }
}
