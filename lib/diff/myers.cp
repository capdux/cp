// Source: cp/lib/diff/myers.cp (Myers O(ND) Line & Word Diff Engine)
// Pure CP zero-external-dependency Myers O(ND) shortest edit script engine
// with prefix/suffix trimming and 64-bit FNV-1a line hashing optimization.

const std = @import("std");

/// Errors returned by diffing, unified patch parsing/application, and 3-way merge.
pub err DiffError {
    InvalidPatch,
    HunkMismatch,
    ContextConflict,
    OutOfMemory,
}

/// Kind of a single edit operation in a diff sequence.
pub enum DiffOpKind: u8 {
    Equal = 0,
    Insert = 1,
    Delete = 2,
}

/// A single edit operation produced by `DiffEngine.diffLines` or `DiffEngine.diffWords`.
pub struct DiffEdit {
    kind: DiffOpKind = DiffOpKind.Equal,
    text: string = "",
    old_line: usize = 0,
    new_line: usize = 0,
}

/// Summary statistics for a sequence of `DiffEdit` operations.
pub struct DiffStats {
    equal_count: usize = 0,
    insert_count: usize = 0,
    delete_count: usize = 0,
}

const MYERS_MAX_D: usize = 4096;

fn hashBytes(s: string): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h = (h ^ u64(s[i])) *% 1099511628211;
        i += 1;
    }
    return h;
}

fn isWhitespaceByte(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}

fn isWordByte(b: u8): bool {
    return (b >= 'a' && b <= 'z') ||
           (b >= 'A' && b <= 'Z') ||
           (b >= '0' && b <= '9') ||
           b == '_' ||
           b >= 0x80;
}

fn trimTrailingCr(s: string): string {
    if (s.len > 0 && s[s.len - 1] == '\r') {
        return s[0..s.len - 1];
    }
    return s;
}

/// Splits `text` into lines (without trailing `\n` or `\r\n`), returning an owned slice of line slices.
pub fn splitLinesAlloc(alloc: std.mem.Allocator, text: string): ([]string) ! DiffError {
    let list: std.ArrayListUnmanaged(string) = .empty;
    errdefer list.deinit(alloc);

    if (text.len == 0) {
        return list.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
    }

    let start: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        if (text[i] == '\n') {
            const line = trimTrailingCr(text[start..i]);
            list.append(alloc, line) catch return DiffError.OutOfMemory;
            start = i + 1;
        }
        i += 1;
    }

    // If text does not end with '\n', emit the final unterminated line.
    if (start < text.len) {
        const line = trimTrailingCr(text[start..text.len]);
        list.append(alloc, line) catch return DiffError.OutOfMemory;
    }

    return list.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
}

/// Splits `text` into lossless word/whitespace/punctuation tokens.
pub fn tokenizeWordsAlloc(alloc: std.mem.Allocator, text: string): ([]string) ! DiffError {
    let list: std.ArrayListUnmanaged(string) = .empty;
    errdefer list.deinit(alloc);

    let i: usize = 0;
    while (i < text.len) {
        const start = i;
        const b = text[i];
        if (isWhitespaceByte(b)) {
            i += 1;
            while (i < text.len && isWhitespaceByte(text[i])) {
                i += 1;
            }
        } else if (isWordByte(b)) {
            i += 1;
            while (i < text.len && isWordByte(text[i])) {
                i += 1;
            }
        } else {
            i += 1;
        }
        list.append(alloc, text[start..i]) catch return DiffError.OutOfMemory;
    }

    return list.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
}

fn diffTokenSlices(alloc: std.mem.Allocator, old_tokens: []const string, new_tokens: []const string): ([]DiffEdit) ! DiffError {
    const old_len = old_tokens.len;
    const new_len = new_tokens.len;

    let prefix: usize = 0;
    while (prefix < old_len && prefix < new_len && std.mem.eql(u8, old_tokens[prefix], new_tokens[prefix])) {
        prefix += 1;
    }

    let suffix: usize = 0;
    while (prefix + suffix < old_len &&
           prefix + suffix < new_len &&
           std.mem.eql(u8, old_tokens[old_len - 1 - suffix], new_tokens[new_len - 1 - suffix]))
    {
        suffix += 1;
    }

    const mid_old_len = old_len - prefix - suffix;
    const mid_new_len = new_len - prefix - suffix;

    let out: std.ArrayListUnmanaged(DiffEdit) = .empty;
    errdefer out.deinit(alloc);

    // Emit common prefix
    let pi: usize = 0;
    while (pi < prefix) {
        out.append(alloc, DiffEdit{
            kind: DiffOpKind.Equal,
            text: old_tokens[pi],
            old_line: pi + 1,
            new_line: pi + 1,
        }) catch return DiffError.OutOfMemory;
        pi += 1;
    }

    if (mid_old_len == 0 && mid_new_len == 0) {
        // Nothing in the middle
    } else if (mid_old_len == 0) {
        let ni: usize = 0;
        while (ni < mid_new_len) {
            out.append(alloc, DiffEdit{
                kind: DiffOpKind.Insert,
                text: new_tokens[prefix + ni],
                old_line: 0,
                new_line: prefix + ni + 1,
            }) catch return DiffError.OutOfMemory;
            ni += 1;
        }
    } else if (mid_new_len == 0) {
        let oi: usize = 0;
        while (oi < mid_old_len) {
            out.append(alloc, DiffEdit{
                kind: DiffOpKind.Delete,
                text: old_tokens[prefix + oi],
                old_line: prefix + oi + 1,
                new_line: 0,
            }) catch return DiffError.OutOfMemory;
            oi += 1;
        }
    } else {
        const mid_old = old_tokens[prefix .. prefix + mid_old_len];
        const mid_new = new_tokens[prefix .. prefix + mid_new_len];

        let old_hashes = alloc.alloc(u64, mid_old_len) catch return DiffError.OutOfMemory;
        defer alloc.free(old_hashes);
        let new_hashes = alloc.alloc(u64, mid_new_len) catch return DiffError.OutOfMemory;
        defer alloc.free(new_hashes);

        let hi: usize = 0;
        while (hi < mid_old_len) {
            old_hashes[hi] = hashBytes(mid_old[hi]);
            hi += 1;
        }
        hi = 0;
        while (hi < mid_new_len) {
            new_hashes[hi] = hashBytes(mid_new[hi]);
            hi += 1;
        }

        const total_len = mid_old_len + mid_new_len;
        const max_d = if (total_len < MYERS_MAX_D) total_len else MYERS_MAX_D;
        const v_len = 2 * max_d + 1;
        const offset: isize = isize(max_d);

        let v = alloc.alloc(isize, v_len) catch return DiffError.OutOfMemory;
        defer alloc.free(v);
        @memset(v, 0);

        let trace: std.ArrayListUnmanaged(isize) = .empty;
        defer trace.deinit(alloc);
        let trace_starts: std.ArrayListUnmanaged(usize) = .empty;
        defer trace_starts.deinit(alloc);

        let found = false;
        let found_d: usize = 0;
        let d: usize = 0;

        while (d <= max_d) {
            const d_s: isize = isize(d);
            const neg_d: isize = 0 - d_s;
            trace_starts.append(alloc, trace.items.len) catch return DiffError.OutOfMemory;
            let tk: isize = neg_d;
            while (tk <= d_s) {
                trace.append(alloc, v[usize(offset + tk)]) catch return DiffError.OutOfMemory;
                tk += 2;
            }

            let k: isize = neg_d;
            while (k <= d_s) {
                const go_down = (k == neg_d) || (k != d_s && v[usize(offset + k - 1)] < v[usize(offset + k + 1)]);
                let x: isize = if (go_down) v[usize(offset + k + 1)] else v[usize(offset + k - 1)] + 1;
                let y: isize = x - k;

                while (x >= 0 && y >= 0 && usize(x) < mid_old_len && usize(y) < mid_new_len) {
                    const ux = usize(x);
                    const uy = usize(y);
                    if (old_hashes[ux] != new_hashes[uy]) {
                        break;
                    }
                    if (!std.mem.eql(u8, mid_old[ux], mid_new[uy])) {
                        break;
                    }
                    x += 1;
                    y += 1;
                }

                v[usize(offset + k)] = x;

                if (usize(x) >= mid_old_len && usize(y) >= mid_new_len) {
                    found = true;
                    found_d = d;
                    break;
                }
                k += 2;
            }

            if (found) {
                break;
            }
            d += 1;
        }

        if (!found) {
            // Fallback for extreme edit distances exceeding MYERS_MAX_D
            let oi: usize = 0;
            while (oi < mid_old_len) {
                out.append(alloc, DiffEdit{
                    kind: DiffOpKind.Delete,
                    text: mid_old[oi],
                    old_line: prefix + oi + 1,
                    new_line: 0,
                }) catch return DiffError.OutOfMemory;
                oi += 1;
            }
            let ni: usize = 0;
            while (ni < mid_new_len) {
                out.append(alloc, DiffEdit{
                    kind: DiffOpKind.Insert,
                    text: mid_new[ni],
                    old_line: 0,
                    new_line: prefix + ni + 1,
                }) catch return DiffError.OutOfMemory;
                ni += 1;
            }
        } else {
            let rev_edits: std.ArrayListUnmanaged(DiffEdit) = .empty;
            defer rev_edits.deinit(alloc);

            let cx: isize = isize(mid_old_len);
            let cy: isize = isize(mid_new_len);
            let bd: isize = isize(found_d);

            while (bd >= 0) {
                const ubd = usize(bd);
                const snap_start = trace_starts.items[ubd];

                if (bd == 0) {
                    while (cx > 0 && cy > 0) {
                        cx -= 1;
                        cy -= 1;
                        const ux = usize(cx);
                        const uy = usize(cy);
                        rev_edits.append(alloc, DiffEdit{
                            kind: DiffOpKind.Equal,
                            text: mid_old[ux],
                            old_line: prefix + ux + 1,
                            new_line: prefix + uy + 1,
                        }) catch return DiffError.OutOfMemory;
                    }
                    break;
                }

                const k = cx - cy;
                const prev_left = if (k == -bd) 0 else trace.items[snap_start + usize((k - 1 + bd) / 2)];
                const prev_right = if (k == bd) 0 else trace.items[snap_start + usize((k + 1 + bd) / 2)];
                const go_down = (k == -bd) || (k != bd && prev_left < prev_right);
                const prev_k = if (go_down) k + 1 else k - 1;
                const prev_x = trace.items[snap_start + usize((prev_k + bd) / 2)];
                const prev_y = prev_x - prev_k;

                while (cx > prev_x && cy > prev_y) {
                    cx -= 1;
                    cy -= 1;
                    const ux = usize(cx);
                    const uy = usize(cy);
                    rev_edits.append(alloc, DiffEdit{
                        kind: DiffOpKind.Equal,
                        text: mid_old[ux],
                        old_line: prefix + ux + 1,
                        new_line: prefix + uy + 1,
                    }) catch return DiffError.OutOfMemory;
                }

                if (cx == prev_x) {
                    cy -= 1;
                    const uy = usize(cy);
                    rev_edits.append(alloc, DiffEdit{
                        kind: DiffOpKind.Insert,
                        text: mid_new[uy],
                        old_line: 0,
                        new_line: prefix + uy + 1,
                    }) catch return DiffError.OutOfMemory;
                } else {
                    cx -= 1;
                    const ux = usize(cx);
                    rev_edits.append(alloc, DiffEdit{
                        kind: DiffOpKind.Delete,
                        text: mid_old[ux],
                        old_line: prefix + ux + 1,
                        new_line: 0,
                    }) catch return DiffError.OutOfMemory;
                }

                bd -= 1;
            }

            let ri = rev_edits.items.len;
            while (ri > 0) {
                ri -= 1;
                out.append(alloc, rev_edits.items[ri]) catch return DiffError.OutOfMemory;
            }
        }
    }

    // Emit common suffix
    let si: usize = 0;
    while (si < suffix) {
        const oi = old_len - suffix + si;
        const ni = new_len - suffix + si;
        out.append(alloc, DiffEdit{
            kind: DiffOpKind.Equal,
            text: old_tokens[oi],
            old_line: oi + 1,
            new_line: ni + 1,
        }) catch return DiffError.OutOfMemory;
        si += 1;
    }

    return out.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
}

/// Myers O(ND) line-level and word-level diff engine.
pub static class DiffEngine {
    /// Computes the shortest line-level edit script between `old_text` and `new_text`.
    pub fn diffLines(alloc: std.mem.Allocator, old_text: string, new_text: string): ([]DiffEdit) ! DiffError {
        const old_lines = splitLinesAlloc(alloc, old_text)?;
        defer alloc.free(old_lines);
        const new_lines = splitLinesAlloc(alloc, new_text)?;
        defer alloc.free(new_lines);
        return diffTokenSlices(alloc, old_lines, new_lines);
    }

    /// Computes a word/token-level edit script between `old_text` and `new_text`.
    pub fn diffWords(alloc: std.mem.Allocator, old_text: string, new_text: string): ([]DiffEdit) ! DiffError {
        const old_words = tokenizeWordsAlloc(alloc, old_text)?;
        defer alloc.free(old_words);
        const new_words = tokenizeWordsAlloc(alloc, new_text)?;
        defer alloc.free(new_words);
        return diffTokenSlices(alloc, old_words, new_words);
    }

    /// Computes summary counts of equal, inserted, and deleted operations.
    pub fn computeStats(edits: []const DiffEdit): DiffStats {
        let stats = DiffStats{};
        let i: usize = 0;
        while (i < edits.len) {
            switch (edits[i].kind) {
                .Equal => { stats.equal_count += 1; },
                .Insert => { stats.insert_count += 1; },
                .Delete => { stats.delete_count += 1; },
            }
            i += 1;
        }
        return stats;
    }
}
