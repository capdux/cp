// @cap/diff — Zero-allocation Myers O(ND) Diff & Unified Patch Engine
// Clean-room pure-CP implementation (inspired by npm `diff` & Rust `similar` crate, MIT License).
// Features:
// - FNV-1a 64-bit line hash pre-filtering for O(1) fast rejection before byte comparison
// - Common prefix/suffix stripping + standard Myers O(ND) shortest edit script (SES)
// - Compact triangular stack trace buffer (`undefined` initialized, zero heap allocation)
// - Line-level diff, byte/character-level diff, GNU Unified Diff formatter, and lossless patch apply

import { findByteInBytes, bytesEq } from "cap:bin";

pub const MAX_LINES: usize = 256;
pub const MAX_EDITS: usize = 512;

const FNV_OFFSET_BASIS: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
const FNV_PRIME: u64 = 0x100000001b3;

// Compact triangular trace capacity for d in 0..=MAX_EDITS:
// sum_{d=0}^{512} (d + 1) = 513 * 514 / 2 = 131841
const TRACE_CAP: usize = (MAX_EDITS + 1) * (MAX_EDITS + 2) / 2;
const V_OFFSET: usize = MAX_EDITS + 1;
const V_LEN: usize = MAX_EDITS * 2 + 3;

pub err DiffError {
    TooManyLines,
    BufferTooSmall,
    PatchFailed,
    InvalidPatch,
}

/// Edit operation kind in a shortest edit script.
pub enum DiffOp: u8 {
    Equal = 0,
    Delete = 1,
    Insert = 2,
}

/// A single edit operation referencing line or byte indices in the old/new inputs.
pub struct DiffEdit {
    op: DiffOp = DiffOp.Equal,
    old_index: usize = 0,
    new_index: usize = 0,
    text: []const u8 = "",
}

/// Aggregate statistics of a diff result.
pub struct DiffStats {
    equal: usize = 0,
    inserted: usize = 0,
    deleted: usize = 0,

    /// Computes Ratcliff-Obershelp / `similar`-compatible similarity ratio in [0.0, 1.0].
    /// Returns 1.0 when both inputs are empty.
    pub fn similarity(): f64 {
        const total = _.equal * 2 + _.inserted + _.deleted;
        if (total == 0) return 1.0;
        return (2.0 * (_.equal as f64)) / (total as f64);
    }
}

/// Stack-allocated diff result holding up to `MAX_EDITS` edit operations.
pub struct LineDiffResult {
    edits: [MAX_EDITS]DiffEdit = undefined,
    len: usize = 0,

    /// Returns aggregate equal/inserted/deleted counts and similarity ratio.
    pub fn stats(): DiffStats {
        let s = DiffStats{};
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const op = _.edits[i].op;
            if (op == DiffOp.Equal) {
                s.equal += 1;
            } else if (op == DiffOp.Insert) {
                s.inserted += 1;
            } else {
                s.deleted += 1;
            }
        }
        return s;
    }

    /// Returns true if and only if there are no insertions or deletions.
    pub fn isIdentical(): bool {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.edits[i].op != DiffOp.Equal) return false;
        }
        return true;
    }

    /// Returns the valid slice of edit operations.
    pub fn slice(): []const DiffEdit {
        return _.edits[0.._.len];
    }

    /// Safe indexed access to an edit operation.
    pub fn get(idx: usize): DiffEdit? {
        if (idx >= _.len) return null;
        return _.edits[idx];
    }
}

struct LineEntry {
    text: []const u8 = "",
    hash: u64 = 0,
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = (h ^ (data[i] as u64)) *% FNV_PRIME;
    }
    return h;
}

fn linesEqual(a: LineEntry, b: LineEntry): bool {
    if (a.hash != b.hash) return false;
    if (a.text.len != b.text.len) return false;
    return bytesEq(a.text, b.text);
}

/// Splits `text` into lines preserving trailing `\n` characters so reconstruction is lossless.
fn splitLines(text: []const u8, out: []LineEntry): usize ! DiffError {
    if (text.len == 0) return 0;
    let count: usize = 0;
    let start: usize = 0;
    while (start < text.len) {
        if (count >= out.len) return DiffError.TooManyLines;
        if (nl := findByteInBytes(text, '\n', start)) {
            const seg = text[start..(nl + 1)];
            out[count] = LineEntry{
                text: seg,
                hash: fnv1a64(seg),
            };
            count += 1;
            start = nl + 1;
        } else {
            const seg = text[start..text.len];
            out[count] = LineEntry{
                text: seg,
                hash: fnv1a64(seg),
            };
            count += 1;
            break;
        }
    }
    return count;
}

fn traceSlot(d: usize, k: i32): usize {
    const base = (d * (d + 1)) / 2;
    const j = ((k + (d as i32)) >> 1) as usize;
    return base + j;
}

/// Core Myers O(ND) shortest edit script over pre-hashed line entries.
fn myersDiffEntries(
    old_entries: []const LineEntry,
    new_entries: []const LineEntry,
): LineDiffResult ! DiffError {
    let res = LineDiffResult{};
    const old_len = old_entries.len;
    const new_len = new_entries.len;

    // 1. Common prefix stripping (O(min(N, M)) fast path)
    let prefix: usize = 0;
    while (prefix < old_len && prefix < new_len && linesEqual(old_entries[prefix], new_entries[prefix])) {
        prefix += 1;
    }

    // 2. Common suffix stripping
    let suffix: usize = 0;
    while (
        suffix < (old_len - prefix)
        && suffix < (new_len - prefix)
        && linesEqual(old_entries[old_len - 1 - suffix], new_entries[new_len - 1 - suffix])
    ) {
        suffix += 1;
    }

    // Emit common prefix
    let p: usize = 0;
    while (p < prefix) : (p += 1) {
        res.edits[res.len] = DiffEdit{
            op: DiffOp.Equal,
            old_index: p,
            new_index: p,
            text: old_entries[p].text,
        };
        res.len += 1;
    }

    const n = old_len - prefix - suffix;
    const m = new_len - prefix - suffix;

    // 3. Run Myers O(ND) on the differing middle window [prefix .. prefix + n/m)
    if (n == 0 && m > 0) {
        let j: usize = 0;
        while (j < m) : (j += 1) {
            res.edits[res.len] = DiffEdit{
                op: DiffOp.Insert,
                old_index: prefix,
                new_index: prefix + j,
                text: new_entries[prefix + j].text,
            };
            res.len += 1;
        }
    } else if (m == 0 && n > 0) {
        let i: usize = 0;
        while (i < n) : (i += 1) {
            res.edits[res.len] = DiffEdit{
                op: DiffOp.Delete,
                old_index: prefix + i,
                new_index: prefix,
                text: old_entries[prefix + i].text,
            };
            res.len += 1;
        }
    } else if (n > 0 && m > 0) {
        const old_mid = old_entries[prefix..(prefix + n)];
        const new_mid = new_entries[prefix..(prefix + m)];
        const max_d = n + m;
        const n_i = n as i32;
        const m_i = m as i32;

        let v: [V_LEN]i32 = undefined;
        let trace: [TRACE_CAP]u16 = undefined;
        v[V_OFFSET + 1] = 0;

        let found_d: usize = max_d;
        let d: usize = 0;
        let done = false;

        while (d <= max_d && !done) : (d += 1) {
            const d_i = d as i32;
            let k: i32 = -d_i;
            while (k <= d_i) : (k += 2) {
                const k_idx = ((V_OFFSET as i32) + k) as usize;
                let x: i32 = 0;
                if (k == -d_i || (k != d_i && v[k_idx - 1] < v[k_idx + 1])) {
                    x = v[k_idx + 1];
                } else {
                    x = v[k_idx - 1] + 1;
                }
                let y: i32 = x - k;

                while (x < n_i && y < m_i && linesEqual(old_mid[x as usize], new_mid[y as usize])) {
                    x += 1;
                    y += 1;
                }

                v[k_idx] = x;
                trace[traceSlot(d, k)] = x as u16;

                if (x >= n_i && y >= m_i) {
                    found_d = d;
                    done = true;
                    break;
                }
            }
        }

        // Backtrack from (n, m) at step found_d down to (0, 0)
        let rev: [MAX_EDITS]DiffEdit = undefined;
        let rev_len: usize = 0;
        let cur_x: i32 = n_i;
        let cur_y: i32 = m_i;
        let step: usize = found_d;

        while (step > 0) : (step -= 1) {
            const step_i = step as i32;
            const k = cur_x - cur_y;
            let prev_k: i32 = 0;
            if (k == -step_i) {
                prev_k = k + 1;
            } else if (k == step_i) {
                prev_k = k - 1;
            } else {
                const left_x = trace[traceSlot(step - 1, k - 1)];
                const up_x = trace[traceSlot(step - 1, k + 1)];
                if (left_x < up_x) {
                    prev_k = k + 1;
                } else {
                    prev_k = k - 1;
                }
            }

            const prev_x = trace[traceSlot(step - 1, prev_k)] as i32;
            const prev_y = prev_x - prev_k;

            while (cur_x > prev_x && cur_y > prev_y) {
                cur_x -= 1;
                cur_y -= 1;
                rev[rev_len] = DiffEdit{
                    op: DiffOp.Equal,
                    old_index: prefix + (cur_x as usize),
                    new_index: prefix + (cur_y as usize),
                    text: old_mid[cur_x as usize].text,
                };
                rev_len += 1;
            }

            if (cur_x == prev_x) {
                rev[rev_len] = DiffEdit{
                    op: DiffOp.Insert,
                    old_index: prefix + (prev_x as usize),
                    new_index: prefix + (prev_y as usize),
                    text: new_mid[prev_y as usize].text,
                };
                rev_len += 1;
            } else {
                rev[rev_len] = DiffEdit{
                    op: DiffOp.Delete,
                    old_index: prefix + (prev_x as usize),
                    new_index: prefix + (prev_y as usize),
                    text: old_mid[prev_x as usize].text,
                };
                rev_len += 1;
            }

            cur_x = prev_x;
            cur_y = prev_y;
        }

        // Remaining initial diagonal snake at d == 0
        while (cur_x > 0 && cur_y > 0) {
            cur_x -= 1;
            cur_y -= 1;
            rev[rev_len] = DiffEdit{
                op: DiffOp.Equal,
                old_index: prefix + (cur_x as usize),
                new_index: prefix + (cur_y as usize),
                text: old_mid[cur_x as usize].text,
            };
            rev_len += 1;
        }

        // Reverse middle edits into result
        let r: usize = rev_len;
        while (r > 0) {
            r -= 1;
            res.edits[res.len] = rev[r];
            res.len += 1;
        }
    }

    // 4. Emit common suffix
    let s: usize = 0;
    while (s < suffix) : (s += 1) {
        const oi = old_len - suffix + s;
        const ni = new_len - suffix + s;
        res.edits[res.len] = DiffEdit{
            op: DiffOp.Equal,
            old_index: oi,
            new_index: ni,
            text: old_entries[oi].text,
        };
        res.len += 1;
    }

    return res;
}

/// Computes the line-level shortest edit script (Myers O(ND)) between `old_text` and `new_text`.
/// Uses FNV-1a 64-bit hash pre-filtering and zero heap allocation (up to `MAX_LINES = 256` lines each).
pub fn diffLines(old_text: []const u8, new_text: []const u8): LineDiffResult ! DiffError {
    let old_buf: [MAX_LINES]LineEntry = undefined;
    const old_count = splitLines(old_text, old_buf[0..])?;
    if (bytesEq(old_text, new_text)) {
        let res = LineDiffResult{};
        let i: usize = 0;
        while (i < old_count) : (i += 1) {
            res.edits[i] = DiffEdit{
                op: DiffOp.Equal,
                old_index: i,
                new_index: i,
                text: old_buf[i].text,
            };
        }
        res.len = old_count;
        return res;
    }
    let new_buf: [MAX_LINES]LineEntry = undefined;
    const new_count = splitLines(new_text, new_buf[0..])?;
    return myersDiffEntries(old_buf[0..old_count], new_buf[0..new_count]);
}

/// Computes the byte/character-level shortest edit script (Myers O(ND)) for short strings
/// up to `MAX_LINES` (256) bytes each, with zero heap allocation.
pub fn diffChars(old_text: []const u8, new_text: []const u8): LineDiffResult ! DiffError {
    if (old_text.len > MAX_LINES || new_text.len > MAX_LINES) {
        return DiffError.TooManyLines;
    }
    if (bytesEq(old_text, new_text)) {
        let res = LineDiffResult{};
        let i: usize = 0;
        while (i < old_text.len) : (i += 1) {
            res.edits[i] = DiffEdit{
                op: DiffOp.Equal,
                old_index: i,
                new_index: i,
                text: old_text[i..(i + 1)],
            };
        }
        res.len = old_text.len;
        return res;
    }

    let old_buf: [MAX_LINES]LineEntry = undefined;
    let new_buf: [MAX_LINES]LineEntry = undefined;

    let i: usize = 0;
    while (i < old_text.len) : (i += 1) {
        old_buf[i] = LineEntry{
            text: old_text[i..(i + 1)],
            hash: old_text[i] as u64,
        };
    }

    let j: usize = 0;
    while (j < new_text.len) : (j += 1) {
        new_buf[j] = LineEntry{
            text: new_text[j..(j + 1)],
            hash: new_text[j] as u64,
        };
    }

    return myersDiffEntries(old_buf[0..old_text.len], new_buf[0..new_text.len]);
}

struct BufWriter {
    buf: []u8,
    pos: usize = 0,

    fn writeByte(*_, b: u8): void ! DiffError {
        if (_.pos >= _.buf.len) return DiffError.BufferTooSmall;
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn writeSlice(*_, s: []const u8): void ! DiffError {
        if (s.len > _.buf.len - _.pos) return DiffError.BufferTooSmall;
        if (s.len > 0) {
            @memcpy(_.buf[_.pos..(_.pos + s.len)], s);
        }
        _.pos += s.len;
    }

    fn writeUsize(*_, val: usize): void ! DiffError {
        let tmp: [24]u8 = undefined;
        let n: usize = 0;
        let v = val;
        if (v == 0) {
            _.writeByte('0')?;
            return;
        }
        while (v > 0) {
            tmp[n] = ('0' as u8) + ((v % 10) as u8);
            n += 1;
            v = v / 10;
        }
        while (n > 0) {
            n -= 1;
            _.writeByte(tmp[n])?;
        }
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }
}

/// Formats a standard GNU Unified Diff (`--- old_name\n+++ new_name\n@@ -a,b +c,d @@\n...`)
/// with merged context hunks into caller-provided stack buffer `out`.
/// Returns an empty slice when `old_text` and `new_text` are identical.
pub fn formatUnifiedDiff(
    old_name: []const u8,
    new_name: []const u8,
    old_text: []const u8,
    new_text: []const u8,
    context_lines: usize,
    out: []u8,
): ([]const u8) ! DiffError {
    const res = diffLines(old_text, new_text)?;
    if (res.isIdentical()) {
        return out[0..0];
    }

    let w = BufWriter{ buf: out, pos: 0 };
    w.writeSlice("--- ")?;
    w.writeSlice(old_name)?;
    w.writeByte('\n')?;
    w.writeSlice("+++ ")?;
    w.writeSlice(new_name)?;
    w.writeByte('\n')?;

    const total = res.len;
    let idx: usize = 0;

    while (idx < total) {
        // Find next change edit
        while (idx < total && res.edits[idx].op == DiffOp.Equal) {
            idx += 1;
        }
        if (idx >= total) break;

        // Expand hunk start backwards by `context_lines`
        let h_start: usize = 0;
        if (idx > context_lines) {
            h_start = idx - context_lines;
        }

        // Expand hunk end forwards, merging overlapping/adjacent hunks
        let h_end: usize = idx + 1;
        while (h_end < total) {
            // Consume contiguous changes
            while (h_end < total && res.edits[h_end].op != DiffOp.Equal) {
                h_end += 1;
            }
            // Count following equal lines
            let eq_start = h_end;
            let eq_end = eq_start;
            while (eq_end < total && res.edits[eq_end].op == DiffOp.Equal) {
                eq_end += 1;
            }
            const eq_count = eq_end - eq_start;

            // If there is another change after these equal lines and the gap <= 2 * context_lines, merge
            if (eq_end < total && eq_count <= context_lines * 2) {
                h_end = eq_end;
            } else {
                const tail = if (eq_count < context_lines) eq_count else context_lines;
                h_end = eq_start + tail;
                break;
            }
        }

        // Compute hunk header coordinates: @@ -old_start,old_count +new_start,new_count @@
        let old_count: usize = 0;
        let new_count: usize = 0;
        let has_old = false;
        let has_new = false;
        let old_start: usize = 0;
        let new_start: usize = 0;

        let k: usize = h_start;
        while (k < h_end) : (k += 1) {
            const e = res.edits[k];
            if (e.op == DiffOp.Equal) {
                if (!has_old) {
                    old_start = e.old_index + 1;
                    has_old = true;
                }
                if (!has_new) {
                    new_start = e.new_index + 1;
                    has_new = true;
                }
                old_count += 1;
                new_count += 1;
            } else if (e.op == DiffOp.Delete) {
                if (!has_old) {
                    old_start = e.old_index + 1;
                    has_old = true;
                }
                old_count += 1;
            } else {
                if (!has_new) {
                    new_start = e.new_index + 1;
                    has_new = true;
                }
                new_count += 1;
            }
        }

        if (!has_old) {
            old_start = res.edits[h_start].old_index;
        }
        if (!has_new) {
            new_start = res.edits[h_start].new_index;
        }

        w.writeSlice("@@ -")?;
        w.writeUsize(old_start)?;
        w.writeByte(',')?;
        w.writeUsize(old_count)?;
        w.writeSlice(" +")?;
        w.writeUsize(new_start)?;
        w.writeByte(',')?;
        w.writeUsize(new_count)?;
        w.writeSlice(" @@\n")?;

        k = h_start;
        while (k < h_end) : (k += 1) {
            const e = res.edits[k];
            const prefix_ch: u8 = if (e.op == DiffOp.Equal) ' ' else if (e.op == DiffOp.Delete) '-' else '+';
            w.writeByte(prefix_ch)?;
            w.writeSlice(e.text)?;
            if (e.text.len == 0 || e.text[e.text.len - 1] != '\n') {
                w.writeSlice("\n\\ No newline at end of file\n")?;
            }
        }

        idx = h_end;
    }

    return w.written();
}

/// Applies a `LineDiffResult` to `old_text`, writing the reconstructed new text into `out`.
/// Verifies every `Equal` and `Delete` line matches `old_text` in sequence and that no trailing
/// lines remain unconsumed; returns `DiffError.PatchFailed` on mismatch or `DiffError.InvalidPatch`
/// if `diff_res.len > MAX_EDITS`.
pub fn applyLineDiff(
    old_text: []const u8,
    diff_res: LineDiffResult,
    out: []u8,
): ([]const u8) ! DiffError {
    if (diff_res.len > MAX_EDITS) {
        return DiffError.InvalidPatch;
    }

    let old_buf: [MAX_LINES]LineEntry = undefined;
    const old_count = splitLines(old_text, old_buf[0..])?;

    let w = BufWriter{ buf: out, pos: 0 };
    let old_cursor: usize = 0;
    let i: usize = 0;

    while (i < diff_res.len) : (i += 1) {
        const e = diff_res.edits[i];
        if (e.op == DiffOp.Equal) {
            if (old_cursor >= old_count) return DiffError.PatchFailed;
            if (!bytesEq(old_buf[old_cursor].text, e.text)) {
                return DiffError.PatchFailed;
            }
            w.writeSlice(e.text)?;
            old_cursor += 1;
        } else if (e.op == DiffOp.Delete) {
            if (old_cursor >= old_count) return DiffError.PatchFailed;
            if (!bytesEq(old_buf[old_cursor].text, e.text)) {
                return DiffError.PatchFailed;
            }
            old_cursor += 1;
        } else {
            w.writeSlice(e.text)?;
        }
    }

    if (old_cursor != old_count) {
        return DiffError.PatchFailed;
    }

    return w.written();
}
