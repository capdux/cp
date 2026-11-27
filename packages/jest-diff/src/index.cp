// @cap/jest-diff — Pure CP zero-allocation Jest-style unified diff formatter.
// Clean-room MIT implementation inspired by npm `jest-diff`.
// Reuses `@cap/similar-diff` for line-level LCS diffing and `@cap/picocolors` for ANSI coloring.

import {
    DIFF_EQUAL,
    DIFF_DELETE,
    DIFF_INSERT,
    DiffOp,
    diffLines,
    hasChanges
} from "../../similar/src/index.cp";
import { PicoFormatter } from "../../picocolors/src/index.cp";

pub const NO_DIFF_MESSAGE: string = "Compared values have no visual difference.";

const JDF_MAX_LINES: usize = 128;
const JDF_MAX_OPS: usize = 256;
const JDF_LINE_BUF_CAP: usize = 512;

struct JdfLine {
    start: usize,
    end: usize,
}

fn jdfSplitLines(text: string, out_lines: []JdfLine): usize {
    if (text.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        if (text[i] == ('\n' as u8)) {
            let line_end: usize = i;
            if (line_end > line_start && text[line_end - 1] == ('\r' as u8)) {
                line_end = line_end - 1;
            }
            if (count < out_lines.len) {
                out_lines[count] = JdfLine { start: line_start, end: line_end };
            }
            count = count + 1;
            line_start = i + 1;
        }
        i = i + 1;
    }
    if (line_start < text.len) {
        let line_end: usize = text.len;
        if (line_end > line_start && text[line_end - 1] == ('\r' as u8)) {
            line_end = line_end - 1;
        }
        if (count < out_lines.len) {
            out_lines[count] = JdfLine { start: line_start, end: line_end };
        }
        count = count + 1;
    }
    return count;
}

fn jdfWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize {
    let w: usize = pos;
    let i: usize = start;
    while (i < end && w < out.len) {
        out[w] = src[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn jdfWriteStr(out: []u8, pos: usize, s: string): usize {
    return jdfWriteBytes(out, pos, s, 0, s.len);
}

fn jdfWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn jdfWriteUsize(out: []u8, pos: usize, val: usize): usize {
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    let v: u64 = val as u64;
    if (v == (0 as u64)) {
        return jdfWriteByte(out, pos, '0' as u8);
    }
    while (v > (0 as u64)) {
        let digit: u8 = ((v % (10 as u64)) as u8) + ('0' as u8);
        tmp[len] = digit;
        len = len + 1;
        v = v / (10 as u64);
    }
    let w: usize = pos;
    let k: usize = len;
    while (k > 0 && w < out.len) {
        k = k - 1;
        out[w] = tmp[k];
        w = w + 1;
    }
    return w;
}

// Color kind: 0 = gray (context/meta), 1 = red (delete/expected), 2 = green (insert/received)
fn jdfEmitStyledLine(
    fmt: PicoFormatter,
    color_kind: u8,
    line_text: string,
    out: []u8,
    pos: usize
): usize {
    if (pos >= out.len) {
        return pos;
    }
    let written: usize = 0;
    if (color_kind == (1 as u8)) {
        written = fmt.red(line_text, out[pos..out.len]);
    } else if (color_kind == (2 as u8)) {
        written = fmt.green(line_text, out[pos..out.len]);
    } else {
        written = fmt.gray(line_text, out[pos..out.len]);
    }
    return jdfWriteByte(out, pos + written, '\n' as u8);
}

/// Configuration options for Jest-style unified diff output.
pub struct DiffOptions {
    pub a_annotation: string,
    pub b_annotation: string,
    pub context_lines: usize,
    pub expand: bool,
    pub include_change_counts: bool,
    pub use_color: bool,

    pub static fn defaults(): DiffOptions {
        return DiffOptions {
            a_annotation: "- Expected",
            b_annotation: "+ Received",
            context_lines: 5,
            expand: false,
            include_change_counts: false,
            use_color: false
        };
    }

    pub static fn colored(): DiffOptions {
        return DiffOptions {
            a_annotation: "- Expected",
            b_annotation: "+ Received",
            context_lines: 5,
            expand: false,
            include_change_counts: false,
            use_color: true
        };
    }
}

/// Summary counts of a line-level diff comparison.
pub struct DiffSummary {
    pub insertions: usize,
    pub deletions: usize,
    pub equal: usize,
    pub has_diff: bool,
}

/// Computes line-level diff statistics between `expected` and `received`.
pub fn diffStats(expected: string, received: string): DiffSummary {
    let dummy: [0]DiffOp = undefined;
    let stats = diffLines(expected, received, dummy[0..0]);
    return DiffSummary {
        insertions: stats.inserted_lines,
        deletions: stats.deleted_lines,
        equal: stats.equal_lines,
        has_diff: stats.deleted_lines > 0 || stats.inserted_lines > 0
    };
}

/// Formats a Jest-style unified line diff into `out` and returns bytes written.
/// Emits `NO_DIFF_MESSAGE` when `expected` and `received` have no differences.
pub fn diffLinesUnified(
    expected: string,
    received: string,
    opts: DiffOptions,
    out: []u8
): usize {
    if (out.len == 0) {
        return 0;
    }

    let fmt = PicoFormatter.new(opts.use_color);

    if (!hasChanges(expected, received)) {
        if (opts.use_color) {
            return fmt.gray(NO_DIFF_MESSAGE, out);
        }
        return jdfWriteStr(out, 0, NO_DIFF_MESSAGE);
    }

    let exp_lines: [JDF_MAX_LINES]JdfLine = undefined;
    let rec_lines: [JDF_MAX_LINES]JdfLine = undefined;
    let raw_exp_n: usize = jdfSplitLines(expected, exp_lines[0..JDF_MAX_LINES]);
    let raw_rec_n: usize = jdfSplitLines(received, rec_lines[0..JDF_MAX_LINES]);
    let exp_n: usize = if (raw_exp_n > JDF_MAX_LINES) { JDF_MAX_LINES } else { raw_exp_n };
    let rec_n: usize = if (raw_rec_n > JDF_MAX_LINES) { JDF_MAX_LINES } else { raw_rec_n };

    let ops: [JDF_MAX_OPS]DiffOp = undefined;
    let stats = diffLines(expected, received, ops[0..JDF_MAX_OPS]);
    let op_limit: usize = if (stats.total_ops > JDF_MAX_OPS) { JDF_MAX_OPS } else { stats.total_ops };

    let w: usize = 0;
    let line_buf: [JDF_LINE_BUF_CAP]u8 = undefined;

    // Header annotation line A (Expected)
    let hdr_a_len: usize = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], 0, opts.a_annotation);
    if (opts.include_change_counts) {
        hdr_a_len = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], hdr_a_len, "  - ");
        hdr_a_len = jdfWriteUsize(line_buf[0..JDF_LINE_BUF_CAP], hdr_a_len, stats.deleted_lines);
    }
    w = jdfEmitStyledLine(fmt, 1 as u8, line_buf[0..hdr_a_len], out, w);

    // Header annotation line B (Received)
    let hdr_b_len: usize = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], 0, opts.b_annotation);
    if (opts.include_change_counts) {
        hdr_b_len = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], hdr_b_len, "  + ");
        hdr_b_len = jdfWriteUsize(line_buf[0..JDF_LINE_BUF_CAP], hdr_b_len, stats.inserted_lines);
    }
    w = jdfEmitStyledLine(fmt, 2 as u8, line_buf[0..hdr_b_len], out, w);

    // Blank separator line
    w = jdfWriteByte(out, w, '\n' as u8);

    // Compute visibility mask for context folding
    let visible: [JDF_MAX_OPS]bool = [false; JDF_MAX_OPS];
    if (opts.expand) {
        let vi: usize = 0;
        while (vi < op_limit) {
            visible[vi] = true;
            vi = vi + 1;
        }
    } else {
        let ci: usize = 0;
        while (ci < op_limit) {
            if (ops[ci].tag != DIFF_EQUAL) {
                let lo: usize = if (ci > opts.context_lines) { ci - opts.context_lines } else { 0 };
                let hi: usize = if (ci + opts.context_lines + 1 < op_limit) {
                    ci + opts.context_lines + 1
                } else {
                    op_limit
                };
                let j: usize = lo;
                while (j < hi) {
                    visible[j] = true;
                    j = j + 1;
                }
            }
            ci = ci + 1;
        }
    }

    // Emit diff body lines
    let in_skipped: bool = false;
    let idx: usize = 0;
    while (idx < op_limit && w < out.len) {
        if (!visible[idx]) {
            if (!in_skipped) {
                w = jdfEmitStyledLine(fmt, 0 as u8, "@@ ... @@", out, w);
                in_skipped = true;
            }
            idx = idx + 1;
            continue;
        }
        in_skipped = false;

        let op = ops[idx];
        let lb: usize = 0;
        if (op.tag == DIFF_DELETE && op.old_idx < exp_n) {
            lb = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], lb, "- ");
            let ln = exp_lines[op.old_idx];
            lb = jdfWriteBytes(line_buf[0..JDF_LINE_BUF_CAP], lb, expected, ln.start, ln.end);
            w = jdfEmitStyledLine(fmt, 1 as u8, line_buf[0..lb], out, w);
        } else if (op.tag == DIFF_INSERT && op.new_idx < rec_n) {
            lb = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], lb, "+ ");
            let ln = rec_lines[op.new_idx];
            lb = jdfWriteBytes(line_buf[0..JDF_LINE_BUF_CAP], lb, received, ln.start, ln.end);
            w = jdfEmitStyledLine(fmt, 2 as u8, line_buf[0..lb], out, w);
        } else if (op.tag == DIFF_EQUAL && op.old_idx < exp_n) {
            lb = jdfWriteStr(line_buf[0..JDF_LINE_BUF_CAP], lb, "  ");
            let ln = exp_lines[op.old_idx];
            lb = jdfWriteBytes(line_buf[0..JDF_LINE_BUF_CAP], lb, expected, ln.start, ln.end);
            w = jdfEmitStyledLine(fmt, 0 as u8, line_buf[0..lb], out, w);
        }
        idx = idx + 1;
    }

    return w;
}

export {
    NO_DIFF_MESSAGE,
    DiffOptions,
    DiffSummary,
    diffStats,
    diffLinesUnified
};
