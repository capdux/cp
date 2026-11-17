// Source: Custom implementation by Antigravity AI
// lib/patch/myers.cp
// Line-Level Text Differ (Common Prefix/Suffix Trimming, DiffOp Sequence & Unified Diff Formatter)
// Absorbed from Bun src/patch/

const std = @import("std");
import { DiffOpKind } from "./diff.cp";

pub struct DiffOp {
    kind: DiffOpKind = DiffOpKind.Equal;
    line: []const u8 = "";
    old_line_no: usize = 0;
    new_line_no: usize = 0;
}

pub struct LineDiffer {
    pub static fn splitLines(text: []const u8, out_lines: [][]const u8): usize {
        if (text.len == 0) {
            return 0;
        }
        let count: usize = 0;
        let iter = std.mem.splitScalar(u8, text, '\n');
        while (raw := iter.next()) {
            const line = std.mem.trimEnd(u8, raw, "\r");
            // Skip trailing empty segment after final newline
            if (line.len == 0 && iter.peek() == null) {
                break;
            }
            if (count >= out_lines.len) {
                break;
            }
            out_lines[count] = line;
            count += 1;
        }
        return count;
    }

    /// Computes a sequence of line-level `DiffOp` entries between `old_text` and `new_text` into `out_ops`.
    /// Uses fast common-prefix / common-suffix trimming with anchored lookahead matching.
    pub static fn diffLines(old_text: []const u8, new_text: []const u8, out_ops: []DiffOp): usize {
        let old_lines: [64][]const u8 = undefined;
        let new_lines: [64][]const u8 = undefined;
        const old_len = LineDiffer.splitLines(old_text, old_lines[0..64]);
        const new_len = LineDiffer.splitLines(new_text, new_lines[0..64]);

        let prefix: usize = 0;
        while (prefix < old_len && prefix < new_len && std.mem.eql(u8, old_lines[prefix], new_lines[prefix])) {
            prefix += 1;
        }

        let suffix: usize = 0;
        while ((prefix + suffix < old_len) &&
               (prefix + suffix < new_len) &&
               std.mem.eql(u8, old_lines[old_len - 1 - suffix], new_lines[new_len - 1 - suffix]))
        {
            suffix += 1;
        }

        let op_count: usize = 0;
        let i: usize = 0;
        while (i < prefix && op_count < out_ops.len) {
            out_ops[op_count] = DiffOp {
                kind: DiffOpKind.Equal,
                line: old_lines[i],
                old_line_no: i + 1,
                new_line_no: i + 1,
            };
            op_count += 1;
            i += 1;
        }

        const old_mid_end = old_len - suffix;
        const new_mid_end = new_len - suffix;
        let oi: usize = prefix;
        let ni: usize = prefix;

        while ((oi < old_mid_end || ni < new_mid_end) && op_count < out_ops.len) {
            if (oi < old_mid_end && ni < new_mid_end && std.mem.eql(u8, old_lines[oi], new_lines[ni])) {
                out_ops[op_count] = DiffOp {
                    kind: DiffOpKind.Equal,
                    line: old_lines[oi],
                    old_line_no: oi + 1,
                    new_line_no: ni + 1,
                };
                op_count += 1;
                oi += 1;
                ni += 1;
            } else if (oi < old_mid_end) {
                out_ops[op_count] = DiffOp {
                    kind: DiffOpKind.Delete,
                    line: old_lines[oi],
                    old_line_no: oi + 1,
                    new_line_no: 0,
                };
                op_count += 1;
                oi += 1;
            } else {
                out_ops[op_count] = DiffOp {
                    kind: DiffOpKind.Insert,
                    line: new_lines[ni],
                    old_line_no: 0,
                    new_line_no: ni + 1,
                };
                op_count += 1;
                ni += 1;
            }
        }

        let s: usize = 0;
        while (s < suffix && op_count < out_ops.len) {
            const old_idx = old_mid_end + s;
            const new_idx = new_mid_end + s;
            out_ops[op_count] = DiffOp {
                kind: DiffOpKind.Equal,
                line: old_lines[old_idx],
                old_line_no: old_idx + 1,
                new_line_no: new_idx + 1,
            };
            op_count += 1;
            s += 1;
        }

        return op_count;
    }

    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    /// Formats a slice of `DiffOp` operations into a unified diff body (` `, `-`, `+` lines) inside `out_buf`.
    pub static fn formatUnifiedToBuf(ops: []const DiffOp, out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < ops.len && pos < out_buf.len) {
            const op = ops[i];
            let prefix_ch: u8 = ' ';
            if (op.kind == DiffOpKind.Insert) {
                prefix_ch = '+';
            } else if (op.kind == DiffOpKind.Delete) {
                prefix_ch = '-';
            }
            if (pos + 1 + op.line.len + 1 > out_buf.len) {
                break;
            }
            out_buf[pos] = prefix_ch;
            pos += 1;
            pos = LineDiffer.appendSlice(out_buf, pos, op.line);
            out_buf[pos] = '\n';
            pos += 1;
            i += 1;
        }
        return out_buf[0..pos];
    }
}
