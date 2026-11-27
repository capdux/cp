// packages/@cap/similar-diff/src/index.cp
// Zero-allocation line-level LCS diff, unified diff formatter & similarity ratio engine.
// Clean-room MIT implementation inspired by Rust `similar` v2.

// ---------------------------------------------------------------------------
// Public Constants & Types
// ---------------------------------------------------------------------------

pub const DIFF_EQUAL: u8 = 0;
pub const DIFF_DELETE: u8 = 1;
pub const DIFF_INSERT: u8 = 2;

/// Single edit operation in a line-level diff script.
pub struct DiffOp {
    pub tag: u8,
    pub old_idx: usize,
    pub new_idx: usize,
}

/// Summary statistics returned by `diffLines`.
pub struct DiffStats {
    pub equal_lines: usize,
    pub deleted_lines: usize,
    pub inserted_lines: usize,
    pub total_ops: usize,
}

// ---------------------------------------------------------------------------
// Private Line Splitter, LCS Engine & Buffer Helpers (prefixed with smd)
// ---------------------------------------------------------------------------

const SMD_MAX_LINES: usize = 128;
const SMD_DP_STRIDE: usize = 129;
const SMD_DP_CELLS: usize = 16641; // 129 * 129

struct SmdLine {
    start: usize,
    end: usize,
}

fn smdSplitLines(text: string, out_lines: []SmdLine): usize {
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
                line_end -= 1;
            }
            if (count < out_lines.len) {
                out_lines[count] = SmdLine{ start: line_start, end: line_end };
            }
            count += 1;
            line_start = i + 1;
        }
        i += 1;
    }
    if (line_start < text.len) {
        let line_end: usize = text.len;
        if (line_end > line_start && text[line_end - 1] == ('\r' as u8)) {
            line_end -= 1;
        }
        if (count < out_lines.len) {
            out_lines[count] = SmdLine{ start: line_start, end: line_end };
        }
        count += 1;
    }
    return count;
}

fn smdLineEq(a: string, la: SmdLine, b: string, lb: SmdLine): bool {
    const len_a: usize = la.end - la.start;
    const len_b: usize = lb.end - lb.start;
    if (len_a != len_b) {
        return false;
    }
    let i: usize = 0;
    while (i < len_a) {
        if (a[la.start + i] != b[lb.start + i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn smdEmitOp(ops_out: []DiffOp, stats: *DiffStats, tag: u8, old_idx: usize, new_idx: usize) {
    if (stats.*.total_ops < ops_out.len) {
        ops_out[stats.*.total_ops] = DiffOp{ tag: tag, old_idx: old_idx, new_idx: new_idx };
    }
    stats.*.total_ops += 1;
    if (tag == DIFF_EQUAL) {
        stats.*.equal_lines += 1;
    } else if (tag == DIFF_DELETE) {
        stats.*.deleted_lines += 1;
    } else {
        stats.*.inserted_lines += 1;
    }
}

fn smdWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize {
    let w: usize = pos;
    let i: usize = start;
    while (i < end && w < out.len) {
        out[w] = src[i];
        w += 1;
        i += 1;
    }
    return w;
}

fn smdWriteStr(out: []u8, pos: usize, s: string): usize {
    return smdWriteBytes(out, pos, s, 0, s.len);
}

fn smdWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn smdWriteUsize(out: []u8, pos: usize, val: usize): usize {
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    let v: u64 = val as u64;
    if (v == (0 as u64)) {
        return smdWriteByte(out, pos, '0' as u8);
    }
    while (v > (0 as u64)) {
        const digit: u8 = ((v % (10 as u64)) as u8) + ('0' as u8);
        tmp[len] = digit;
        len += 1;
        v = v / (10 as u64);
    }
    let w: usize = pos;
    let k: usize = len;
    while (k > 0 && w < out.len) {
        k -= 1;
        out[w] = tmp[k];
        w += 1;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Public Diff API
// ---------------------------------------------------------------------------

/// Computes a minimal line-level edit script (LCS) between `old_text` and `new_text`.
/// Supports up to 128 lines per side with exact LCS and gracefully degrades for larger inputs.
pub fn diffLines(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats {
    let old_lines: [SMD_MAX_LINES]SmdLine = undefined;
    let new_lines: [SMD_MAX_LINES]SmdLine = undefined;

    const raw_old_n: usize = smdSplitLines(old_text, old_lines[0..SMD_MAX_LINES]);
    const raw_new_n: usize = smdSplitLines(new_text, new_lines[0..SMD_MAX_LINES]);

    const old_n: usize = if (raw_old_n > SMD_MAX_LINES) { SMD_MAX_LINES } else { raw_old_n };
    const new_n: usize = if (raw_new_n > SMD_MAX_LINES) { SMD_MAX_LINES } else { raw_new_n };

    let stats = DiffStats{
        equal_lines: 0,
        deleted_lines: 0,
        inserted_lines: 0,
        total_ops: 0,
    };

    // Trim common prefix
    let prefix: usize = 0;
    while (prefix < old_n && prefix < new_n
        && smdLineEq(old_text, old_lines[prefix], new_text, new_lines[prefix]))
    {
        smdEmitOp(ops_out, &stats, DIFF_EQUAL, prefix, prefix);
        prefix += 1;
    }

    // Trim common suffix
    let old_end: usize = old_n;
    let new_end: usize = new_n;
    while (old_end > prefix && new_end > prefix
        && smdLineEq(old_text, old_lines[old_end - 1], new_text, new_lines[new_end - 1]))
    {
        old_end -= 1;
        new_end -= 1;
    }

    const m: usize = old_end - prefix;
    const n: usize = new_end - prefix;

    if (m == 0) {
        let j: usize = 0;
        while (j < n) {
            smdEmitOp(ops_out, &stats, DIFF_INSERT, old_end, prefix + j);
            j += 1;
        }
    } else if (n == 0) {
        let i: usize = 0;
        while (i < m) {
            smdEmitOp(ops_out, &stats, DIFF_DELETE, prefix + i, new_end);
            i += 1;
        }
    } else {
        // Exact bottom-up LCS DP table on [0..m] x [0..n]
        let dp: [SMD_DP_CELLS]u16 = undefined;
        const stride: usize = n + 1;

        // Initialize last row and last column to 0
        let col: usize = 0;
        while (col <= n) {
            dp[m * stride + col] = 0 as u16;
            col += 1;
        }
        let row: usize = 0;
        while (row <= m) {
            dp[row * stride + n] = 0 as u16;
            row += 1;
        }

        let r: usize = m;
        while (r > 0) {
            r -= 1;
            let c: usize = n;
            while (c > 0) {
                c -= 1;
                if (smdLineEq(old_text, old_lines[prefix + r], new_text, new_lines[prefix + c])) {
                    dp[r * stride + c] = dp[(r + 1) * stride + (c + 1)] + (1 as u16);
                } else {
                    const down: u16 = dp[(r + 1) * stride + c];
                    const right: u16 = dp[r * stride + (c + 1)];
                    dp[r * stride + c] = if (down >= right) { down } else { right };
                }
            }
        }

        // Forward traceback from (0, 0) to (m, n)
        let i: usize = 0;
        let j: usize = 0;
        while (i < m || j < n) {
            if (i < m && j < n
                && smdLineEq(old_text, old_lines[prefix + i], new_text, new_lines[prefix + j]))
            {
                smdEmitOp(ops_out, &stats, DIFF_EQUAL, prefix + i, prefix + j);
                i += 1;
                j += 1;
            } else if (i < m && (j == n || dp[(i + 1) * stride + j] >= dp[i * stride + (j + 1)])) {
                smdEmitOp(ops_out, &stats, DIFF_DELETE, prefix + i, prefix + j);
                i += 1;
            } else {
                smdEmitOp(ops_out, &stats, DIFF_INSERT, prefix + i, prefix + j);
                j += 1;
            }
        }
    }

    // Emit trimmed common suffix
    let suf_old: usize = old_end;
    let suf_new: usize = new_end;
    while (suf_old < old_n && suf_new < new_n) {
        smdEmitOp(ops_out, &stats, DIFF_EQUAL, suf_old, suf_new);
        suf_old += 1;
        suf_new += 1;
    }

    // Graceful degradation for truncated tail beyond SMD_MAX_LINES
    if (raw_old_n > SMD_MAX_LINES) {
        let extra_old: usize = SMD_MAX_LINES;
        while (extra_old < raw_old_n) {
            smdEmitOp(ops_out, &stats, DIFF_DELETE, extra_old, new_n);
            extra_old += 1;
        }
    }
    if (raw_new_n > SMD_MAX_LINES) {
        let extra_new: usize = SMD_MAX_LINES;
        while (extra_new < raw_new_n) {
            smdEmitOp(ops_out, &stats, DIFF_INSERT, old_n, extra_new);
            extra_new += 1;
        }
    }

    return stats;
}

/// Returns `true` if `old_text` and `new_text` have any line-level differences.
pub fn hasChanges(old_text: string, new_text: string): bool {
    if (old_text.len == new_text.len) {
        let same: bool = true;
        let i: usize = 0;
        while (i < old_text.len) {
            if (old_text[i] != new_text[i]) {
                same = false;
                break;
            }
            i += 1;
        }
        if (same) {
            return false;
        }
    }
    let dummy: [0]DiffOp = undefined;
    const stats = diffLines(old_text, new_text, dummy[0..0]);
    return stats.deleted_lines > 0 || stats.inserted_lines > 0;
}

/// Computes line similarity ratio in basis points `0..10000` (`2 * matches * 10000 / (old + new)`).
/// Returns `10000` when both inputs are empty.
pub fn similarityRatio(old_text: string, new_text: string): u32 {
    let dummy: [0]DiffOp = undefined;
    const stats = diffLines(old_text, new_text, dummy[0..0]);
    const total_lines: u64 = ((stats.equal_lines * 2) + stats.deleted_lines + stats.inserted_lines) as u64;
    if (total_lines == (0 as u64)) {
        return 10000 as u32;
    }
    const numer: u64 = (stats.equal_lines as u64) * (20000 as u64);
    return (numer / total_lines) as u32;
}

/// Formats a standard unified diff (`--- old\n+++ new\n@@ ... @@\n` + `-`/`+`/` ` lines) into `out`.
/// Returns `0` if there are no changes, otherwise returns total bytes written.
pub fn formatUnifiedDiff(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize {
    if (!hasChanges(old_text, new_text) || out.len == 0) {
        return 0;
    }

    let old_lines: [SMD_MAX_LINES]SmdLine = undefined;
    let new_lines: [SMD_MAX_LINES]SmdLine = undefined;
    const raw_old_n: usize = smdSplitLines(old_text, old_lines[0..SMD_MAX_LINES]);
    const raw_new_n: usize = smdSplitLines(new_text, new_lines[0..SMD_MAX_LINES]);
    const old_n: usize = if (raw_old_n > SMD_MAX_LINES) { SMD_MAX_LINES } else { raw_old_n };
    const new_n: usize = if (raw_new_n > SMD_MAX_LINES) { SMD_MAX_LINES } else { raw_new_n };

    let ops: [256]DiffOp = undefined;
    const stats = diffLines(old_text, new_text, ops[0..256]);

    let w: usize = 0;
    w = smdWriteStr(out, w, "--- ");
    w = smdWriteStr(out, w, old_name);
    w = smdWriteByte(out, w, '\n' as u8);
    w = smdWriteStr(out, w, "+++ ");
    w = smdWriteStr(out, w, new_name);
    w = smdWriteByte(out, w, '\n' as u8);

    // Hunk header
    w = smdWriteStr(out, w, "@@ -");
    const old_start_num: usize = if (old_n == 0) { 0 as usize } else { 1 as usize };
    const new_start_num: usize = if (new_n == 0) { 0 as usize } else { 1 as usize };
    w = smdWriteUsize(out, w, old_start_num);
    w = smdWriteByte(out, w, ',' as u8);
    w = smdWriteUsize(out, w, old_n);
    w = smdWriteStr(out, w, " +");
    w = smdWriteUsize(out, w, new_start_num);
    w = smdWriteByte(out, w, ',' as u8);
    w = smdWriteUsize(out, w, new_n);
    w = smdWriteStr(out, w, " @@\n");

    const op_limit: usize = if (stats.total_ops > 256) { 256 as usize } else { stats.total_ops };
    let i: usize = 0;
    while (i < op_limit && w < out.len) {
        const op = ops[i];
        if (op.tag == DIFF_EQUAL && op.old_idx < old_n) {
            w = smdWriteByte(out, w, ' ' as u8);
            const ln = old_lines[op.old_idx];
            w = smdWriteBytes(out, w, old_text, ln.start, ln.end);
            w = smdWriteByte(out, w, '\n' as u8);
        } else if (op.tag == DIFF_DELETE && op.old_idx < old_n) {
            w = smdWriteByte(out, w, '-' as u8);
            const ln = old_lines[op.old_idx];
            w = smdWriteBytes(out, w, old_text, ln.start, ln.end);
            w = smdWriteByte(out, w, '\n' as u8);
        } else if (op.tag == DIFF_INSERT && op.new_idx < new_n) {
            w = smdWriteByte(out, w, '+' as u8);
            const ln = new_lines[op.new_idx];
            w = smdWriteBytes(out, w, new_text, ln.start, ln.end);
            w = smdWriteByte(out, w, '\n' as u8);
        }
        i += 1;
    }

    return w;
}
