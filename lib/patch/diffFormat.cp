// Source: Absorbed from /data/dev/bun/src/test_runner/diff_format.zig | Synced: 2026-11-22T12:49:17Z
// lib/patch/diff_format.cp
// Bun Test-Runner Style Colored Diff Renderer with Line Numbers, Context Trimming & Inline Prefix/Suffix Highlight

const std = @import("std");
import { DiffOpKind, DiffOp, LineDiffer } from "./myers.cp";

const ANSI_RED: []const u8 = "\x1b[31m";
const ANSI_GREEN: []const u8 = "\x1b[32m";
const ANSI_DIM: []const u8 = "\x1b[2m";
const ANSI_INVERT: []const u8 = "\x1b[7m";
const ANSI_INVERT_OFF: []const u8 = "\x1b[27m";
const ANSI_RESET: []const u8 = "\x1b[0m";

pub struct DiffFormatOptions {
    use_color: bool = true,
    context_lines: usize = 3,
    show_line_numbers: bool = true,
    expected_label: []const u8 = "Expected",
    received_label: []const u8 = "Received",
}

struct InlineSpan {
    prefix_len: usize,
    suffix_len: usize,
}

fn computeInlineSpan(expected_line: []const u8, received_line: []const u8): InlineSpan {
    const min_len = if (expected_line.len < received_line.len) expected_line.len else received_line.len;
    let prefix: usize = 0;
    while (prefix < min_len && expected_line[prefix] == received_line[prefix]) {
        prefix += 1;
    }

    let suffix: usize = 0;
    while (prefix + suffix < expected_line.len &&
           prefix + suffix < received_line.len &&
           expected_line[expected_line.len - 1 - suffix] == received_line[received_line.len - 1 - suffix])
    {
        suffix += 1;
    }

    return InlineSpan {
        prefix_len: prefix,
        suffix_len: suffix,
    };
}

fn digitCount(n: usize): usize {
    if (n == 0) return 1;
    let v = n;
    let d: usize = 0;
    while (v > 0) {
        v /= 10;
        d += 1;
    }
    return d;
}

fn appendGutter(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    line_no: usize,
    width: usize,
    show_line_numbers: bool,
    use_color: bool,
): ! {
    if (!show_line_numbers) return;

    if (use_color) try out.appendSlice(allocator, ANSI_DIM);
    if (line_no == 0) {
        let i: usize = 0;
        while (i < width) {
            try out.append(allocator, ' ');
            i += 1;
        }
    } else {
        let num_buf: [32]u8 = undefined;
        const num_str = std.fmt.bufPrint(&num_buf, "{d}", .{line_no}) catch "";
        let pad: usize = if (width > num_str.len) width - num_str.len else 0;
        while (pad > 0) {
            try out.append(allocator, ' ');
            pad -= 1;
        }
        try out.appendSlice(allocator, num_str);
    }
    try out.appendSlice(allocator, " | ");
    if (use_color) try out.appendSlice(allocator, ANSI_RESET);
}

fn appendHighlightedLine(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    prefix_sym: []const u8,
    color_code: []const u8,
    line_text: []const u8,
    prefix_len: usize,
    suffix_len: usize,
    line_no: usize,
    gutter_width: usize,
    opts: DiffFormatOptions,
) ! {
    try appendGutter(allocator, out, line_no, gutter_width, opts.show_line_numbers, opts.use_color);

    if (opts.use_color) try out.appendSlice(allocator, color_code);
    try out.appendSlice(allocator, prefix_sym);

    const has_inline_highlight = (prefix_len > 0 || suffix_len > 0) && (prefix_len + suffix_len <= line_text.len);
    if (!has_inline_highlight) {
        try out.appendSlice(allocator, line_text);
        if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
        try out.append(allocator, '\n');
        return;
    }

    const mid_start = prefix_len;
    const mid_end = line_text.len - suffix_len;

    try out.appendSlice(allocator, line_text[0..mid_start]);
    if (mid_end > mid_start) {
        if (opts.use_color) {
            try out.appendSlice(allocator, ANSI_INVERT);
            try out.appendSlice(allocator, line_text[mid_start..mid_end]);
            try out.appendSlice(allocator, ANSI_INVERT_OFF);
        } else {
            try out.appendSlice(allocator, "«");
            try out.appendSlice(allocator, line_text[mid_start..mid_end]);
            try out.appendSlice(allocator, "»");
        }
    }
    try out.appendSlice(allocator, line_text[mid_end..]);

    if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
    try out.append(allocator, '\n');
}

fn appendEqualLine(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    line_text: []const u8,
    line_no: usize,
    gutter_width: usize,
    opts: DiffFormatOptions,
) ! {
    try appendGutter(allocator, out, line_no, gutter_width, opts.show_line_numbers, opts.use_color);
    if (opts.use_color) try out.appendSlice(allocator, ANSI_DIM);
    try out.appendSlice(allocator, "  ");
    try out.appendSlice(allocator, line_text);
    if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
    try out.append(allocator, '\n');
}

fn appendSkippedMarker(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    skipped_count: usize,
    gutter_width: usize,
    opts: DiffFormatOptions,
) ! {
    if (opts.show_line_numbers) {
        if (opts.use_color) try out.appendSlice(allocator, ANSI_DIM);
        let i: usize = 0;
        while (i < gutter_width) {
            try out.append(allocator, ' ');
            i += 1;
        }
        try out.appendSlice(allocator, " | ");
        if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
    }
    if (opts.use_color) try out.appendSlice(allocator, ANSI_DIM);
    let msg_buf: [64]u8 = undefined;
    const msg = std.fmt.bufPrint(&msg_buf, "... {d} unchanged lines ...", .{skipped_count}) catch "... unchanged lines ...";
    try out.appendSlice(allocator, msg);
    if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
    try out.append(allocator, '\n');
}

pub struct DiffFormatter {
    options: DiffFormatOptions = DiffFormatOptions {},

    pub static fn init(options: DiffFormatOptions = DiffFormatOptions {}): DiffFormatter {
        return DiffFormatter { options: options };
    }

    pub fn render(
        allocator: std.mem.Allocator,
        expected: []const u8,
        received: []const u8,
    ): []u8! {
        return DiffFormatter.format(allocator, expected, received, _.options);
    }

    /// Formats a `bun test`-style colored diff comparing `expected` (green `+`) vs `received` (red `-`).
    /// Supports line number gutters, context window trimming (`... N unchanged lines ...`),
    /// and intra-line common prefix/suffix difference highlighting.
    pub static fn format(
        allocator: std.mem.Allocator,
        expected: []const u8,
        received: []const u8,
        opts: DiffFormatOptions = DiffFormatOptions {},
    ): []u8! {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        if (std.mem.eql(u8, expected, received)) {
            return try out.toOwnedSlice(allocator);
        }

        // Legend header matching bun test: green + Expected, red - Received
        if (opts.use_color) try out.appendSlice(allocator, ANSI_GREEN);
        try out.appendSlice(allocator, "+ ");
        try out.appendSlice(allocator, opts.expected_label);
        if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
        try out.append(allocator, '\n');

        if (opts.use_color) try out.appendSlice(allocator, ANSI_RED);
        try out.appendSlice(allocator, "- ");
        try out.appendSlice(allocator, opts.received_label);
        if (opts.use_color) try out.appendSlice(allocator, ANSI_RESET);
        try out.appendSlice(allocator, "\n\n");

        // Fast path: single-line comparison with intra-line common prefix/suffix highlight
        if (std.mem.indexOfScalar(u8, expected, '\n') == null &&
            std.mem.indexOfScalar(u8, received, '\n') == null)
        {
            const span = computeInlineSpan(expected, received);
            try appendHighlightedLine(allocator, &out, "+ ", ANSI_GREEN, expected, span.prefix_len, span.suffix_len, 1, 1, opts);
            try appendHighlightedLine(allocator, &out, "- ", ANSI_RED, received, span.prefix_len, span.suffix_len, 1, 1, opts);
            return try out.toOwnedSlice(allocator);
        }

        // Multi-line Myers O(ND) diff: old = received (-), new = expected (+)
        let ops_list = try LineDiffer.diffLinesAlloc(allocator, received, expected);
        defer ops_list.deinit(allocator);
        const ops = ops_list.items;

        if (ops.len == 0) {
            return try out.toOwnedSlice(allocator);
        }

        // Compute gutter width from max line number
        let max_line_no: usize = 1;
        for (op of ops) {
            if (op.old_line_no > max_line_no) max_line_no = op.old_line_no;
            if (op.new_line_no > max_line_no) max_line_no = op.new_line_no;
        }
        const gutter_w = digitCount(max_line_no);

        // Compute visibility mask for context trimming
        let visible = try allocator.alloc(bool, ops.len);
        defer allocator.free(visible);
        @memset(visible, false);

        let i: usize = 0;
        while (i < ops.len) {
            if (ops[i].kind != DiffOpKind.Equal) {
                const win_start = if (i > opts.context_lines) i - opts.context_lines else 0;
                const win_end = if (i + opts.context_lines + 1 < ops.len) i + opts.context_lines + 1 else ops.len;
                let k: usize = win_start;
                while (k < win_end) {
                    visible[k] = true;
                    k += 1;
                }
            }
            i += 1;
        }

        // Emit lines, collapsing hidden Equal runs into "... N unchanged lines ..."
        // and pairing adjacent Delete/Insert runs for intra-line prefix/suffix highlight
        let idx: usize = 0;
        while (idx < ops.len) {
            if (!visible[idx]) {
                let run_len: usize = 0;
                while (idx < ops.len && !visible[idx]) {
                    run_len += 1;
                    idx += 1;
                }
                try appendSkippedMarker(allocator, &out, run_len, gutter_w, opts);
                continue;
            }

            if (ops[idx].kind == DiffOpKind.Equal) {
                const ln = if (ops[idx].new_line_no > 0) ops[idx].new_line_no else ops[idx].old_line_no;
                try appendEqualLine(allocator, &out, ops[idx].line, ln, gutter_w, opts);
                idx += 1;
                continue;
            }

            // Collect contiguous Delete followed by Insert hunk to pair modified lines
            const del_start = idx;
            while (idx < ops.len && ops[idx].kind == DiffOpKind.Delete) {
                idx += 1;
            }
            const del_end = idx;

            const ins_start = idx;
            while (idx < ops.len && ops[idx].kind == DiffOpKind.Insert) {
                idx += 1;
            }
            const ins_end = idx;

            const del_count = del_end - del_start;
            const ins_count = ins_end - ins_start;
            const paired = if (del_count < ins_count) del_count else ins_count;

            // Render Expected (+) first, then Received (-), matching bun test convention
            let ii: usize = 0;
            while (ii < ins_count) {
                const ins_op = ops[ins_start + ii];
                if (ii < paired) {
                    const span = computeInlineSpan(ins_op.line, ops[del_start + ii].line);
                    try appendHighlightedLine(
                        allocator,
                        &out,
                        "+ ",
                        ANSI_GREEN,
                        ins_op.line,
                        span.prefix_len,
                        span.suffix_len,
                        ins_op.new_line_no,
                        gutter_w,
                        opts,
                    );
                } else {
                    try appendHighlightedLine(
                        allocator,
                        &out,
                        "+ ",
                        ANSI_GREEN,
                        ins_op.line,
                        0,
                        0,
                        ins_op.new_line_no,
                        gutter_w,
                        opts,
                    );
                }
                ii += 1;
            }

            let di: usize = 0;
            while (di < del_count) {
                const del_op = ops[del_start + di];
                if (di < paired) {
                    const span = computeInlineSpan(ops[ins_start + di].line, del_op.line);
                    try appendHighlightedLine(
                        allocator,
                        &out,
                        "- ",
                        ANSI_RED,
                        del_op.line,
                        span.prefix_len,
                        span.suffix_len,
                        del_op.old_line_no,
                        gutter_w,
                        opts,
                    );
                } else {
                    try appendHighlightedLine(
                        allocator,
                        &out,
                        "- ",
                        ANSI_RED,
                        del_op.line,
                        0,
                        0,
                        del_op.old_line_no,
                        gutter_w,
                        opts,
                    );
                }
                di += 1;
            }
        }

        return try out.toOwnedSlice(allocator);
    }
}
