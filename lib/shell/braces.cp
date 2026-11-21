// Source: /data/dev/bun/src/shell_parser/braces.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell_parser/braces.cp
// Upstream Sync: Refer to /data/dev/bun/src/shell_parser/braces.cp for future upstream updates.
// ============================================================================
// lib/shell/braces.cp
// Industrial-grade Shell Brace Expansion Engine (cap:shell)
// Supports comma lists {a,b,c}, nested braces {a,{b,c},d}, Cartesian products {a,b}{1,2},
// numeric ranges {1..5}, {5..1}, zero-padded ranges {01..05}, stepped ranges {1..10..2},
// and alphabetic ranges {a..e}, {Z..A}.

const std = @import("std");
import { StringJoiner } from "cap:string";

pub const MAX_BRACE_RESULTS: usize = 512;

fn hasUnquotedBrace(raw: []const u8): bool {
    let in_single = false;
    let in_double = false;
    let i: usize = 0;
    while (i < raw.len) {
        const ch = raw[i];
        if (in_single) {
            if (ch == '\'') in_single = false;
            i += 1;
        } else if (in_double) {
            if (ch == '\\' && i + 1 < raw.len) {
                i += 2;
                continue;
            }
            if (ch == '"') in_double = false;
            i += 1;
        } else {
            if (ch == '\\' && i + 1 < raw.len) {
                i += 2;
                continue;
            }
            if (ch == '\'') {
                in_single = true;
            } else if (ch == '"') {
                in_double = true;
            } else if (ch == '$' && i + 1 < raw.len && (raw[i + 1] == '{' || raw[i + 1] == '(')) {
                // Skip ${...} / $(...) / $((...)) so ${VAR:-def} isn't mistaken for brace expansion
                if (raw[i + 1] == '{') {
                    i += 2;
                    let d: usize = 1;
                    while (i < raw.len && d > 0) {
                        if (raw[i] == '\\' && i + 1 < raw.len) { i += 2; continue; }
                        if (raw[i] == '{') d += 1;
                        else if (raw[i] == '}') d -= 1;
                        i += 1;
                    }
                    continue;
                } else {
                    i += 2;
                    let d: usize = 1;
                    while (i < raw.len && d > 0) {
                        if (raw[i] == '\\' && i + 1 < raw.len) { i += 2; continue; }
                        if (raw[i] == '(') d += 1;
                        else if (raw[i] == ')') d -= 1;
                        i += 1;
                    }
                    continue;
                }
            } else if (ch == '{') {
                return true;
            }
            i += 1;
        }
    }
    return false;
}

fn findMatchingBrace(raw: []const u8, open_pos: usize): usize? {
    let depth: usize = 0;
    let in_single = false;
    let in_double = false;
    let i = open_pos;
    while (i < raw.len) {
        const ch = raw[i];
        if (in_single) {
            if (ch == '\'') in_single = false;
            i += 1;
        } else if (in_double) {
            if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (ch == '"') in_double = false;
            i += 1;
        } else {
            if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (ch == '\'') in_single = true;
            else if (ch == '"') in_double = true;
            else if (ch == '$' && i + 1 < raw.len && raw[i + 1] == '{') {
                i += 2;
                let d: usize = 1;
                while (i < raw.len && d > 0) {
                    if (raw[i] == '\\' && i + 1 < raw.len) { i += 2; continue; }
                    if (raw[i] == '{') d += 1;
                    else if (raw[i] == '}') d -= 1;
                    i += 1;
                }
                continue;
            } else if (ch == '{') {
                depth += 1;
            } else if (ch == '}') {
                if (depth > 0) depth -= 1;
                if (depth == 0) return i;
            }
            i += 1;
        }
    }
    return null;
}

struct BraceSpan {
    open: usize = 0,
    close: usize = 0,
}

fn findFirstExpandableOpen(raw: []const u8): BraceSpan? {
    let in_single = false;
    let in_double = false;
    let i: usize = 0;
    while (i < raw.len) {
        const ch = raw[i];
        if (in_single) {
            if (ch == '\'') in_single = false;
            i += 1;
        } else if (in_double) {
            if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (ch == '"') in_double = false;
            i += 1;
        } else {
            if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (ch == '\'') in_single = true;
            else if (ch == '"') in_double = true;
            else if (ch == '$' && i + 1 < raw.len && (raw[i + 1] == '{' || raw[i + 1] == '(')) {
                if (raw[i + 1] == '{') {
                    i += 2;
                    let d: usize = 1;
                    while (i < raw.len && d > 0) {
                        if (raw[i] == '\\' && i + 1 < raw.len) { i += 2; continue; }
                        if (raw[i] == '{') d += 1;
                        else if (raw[i] == '}') d -= 1;
                        i += 1;
                    }
                    continue;
                } else {
                    i += 2;
                    let d: usize = 1;
                    while (i < raw.len && d > 0) {
                        if (raw[i] == '\\' && i + 1 < raw.len) { i += 2; continue; }
                        if (raw[i] == '(') d += 1;
                        else if (raw[i] == ')') d -= 1;
                        i += 1;
                    }
                    continue;
                }
            } else if (ch == '{') {
                if (close_pos := findMatchingBrace(raw, i)) {
                    const inner = raw[i + 1 .. close_pos];
                    if (isCommaList(inner) || isRangeSpec(inner)) {
                        return BraceSpan{ open: i, close: close_pos };
                    }
                }
            }
            i += 1;
        }
    }
    return null;
}

fn isCommaList(inner: []const u8): bool {
    let depth: usize = 0;
    let in_single = false;
    let in_double = false;
    let i: usize = 0;
    while (i < inner.len) {
        const ch = inner[i];
        if (in_single) {
            if (ch == '\'') in_single = false;
        } else if (in_double) {
            if (ch == '\\' && i + 1 < inner.len) { i += 2; continue; }
            if (ch == '"') in_double = false;
        } else {
            if (ch == '\\' && i + 1 < inner.len) { i += 2; continue; }
            if (ch == '\'') in_single = true;
            else if (ch == '"') in_double = true;
            else if (ch == '{') depth += 1;
            else if (ch == '}') { if (depth > 0) depth -= 1; }
            else if (ch == ',' && depth == 0) return true;
        }
        i += 1;
    }
    return false;
}

fn parseIntStrict(s: []const u8): i64? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(i64, s, 10) catch null;
}

fn isRangeSpec(inner: []const u8): bool {
    const first_dots = std.mem.indexOf(u8, inner, "..") ?? return false;
    const p1 = inner[0..first_dots];
    const rest = inner[first_dots + 2 ..];
    let p2 = rest;
    let step_opt: ([]const u8)? = null;
    if (second_dots := std.mem.indexOf(u8, rest, "..")) {
        p2 = rest[0..second_dots];
        step_opt = rest[second_dots + 2 ..];
    }
    if (parseIntStrict(p1) != null && parseIntStrict(p2) != null) {
        if (st := step_opt) {
            return parseIntStrict(st) != null;
        }
        return true;
    }
    if (p1.len == 1 && p2.len == 1 && std.ascii.isAlphabetic(p1[0]) && std.ascii.isAlphabetic(p2[0])) {
        if (st := step_opt) {
            return parseIntStrict(st) != null;
        }
        return true;
    }
    return false;
}

fn expandRangeSpec(
    allocator: std.mem.Allocator,
    prefix: []const u8,
    inner: []const u8,
    suffix: []const u8,
    out: *std.ArrayListUnmanaged(string),
    depth: usize,
) {
    const first_dots = std.mem.indexOf(u8, inner, "..") orelse return;
    const p1 = inner[0..first_dots];
    const rest = inner[first_dots + 2 ..];
    let p2 = rest;
    let step_val: i64 = 1;
    if (second_dots := std.mem.indexOf(u8, rest, "..")) {
        p2 = rest[0..second_dots];
        const st_str = rest[second_dots + 2 ..];
        if (sv := parseIntStrict(st_str)) {
            step_val = sv;
        }
        if (step_val == 0) step_val = 1;
        if (step_val < 0) step_val = -step_val;
    }

    if (n1 := parseIntStrict(p1)) {
        if (n2 := parseIntStrict(p2)) {
            const p1_digits = if (p1.len > 0 && (p1[0] == '-' || p1[0] == '+')) p1[1..] else p1;
            const p2_digits = if (p2.len > 0 && (p2[0] == '-' || p2[0] == '+')) p2[1..] else p2;
            const zero_pad = (p1_digits.len > 1 && p1_digits[0] == '0') || (p2_digits.len > 1 && p2_digits[0] == '0');
            const pad_width = if (p1_digits.len > p2_digits.len) p1_digits.len else p2_digits.len;

            const dir: i64 = if (n1 <= n2) 1 else -1;
            const delta = dir * step_val;
            let cur = n1;
            let guard: usize = 0;
            while (guard < MAX_BRACE_RESULTS) {
                if (dir > 0 && cur > n2) break;
                if (dir < 0 && cur < n2) break;
                if (out.items.len >= MAX_BRACE_RESULTS) break;

                let num_buf: [64]u8 = [0; 64];
                let num_str: []const u8 = "";
                if (zero_pad && cur >= 0) {
                    const raw_n = std.fmt.bufPrint(num_buf[0..32], "{d}", .{cur}) catch "0";
                    let padded = StringJoiner.init();
                    defer padded.~init();
                    if (raw_n.len < pad_width) {
                        let k: usize = 0;
                        while (k < pad_width - raw_n.len) {
                            padded.push("0");
                            k += 1;
                        }
                    }
                    padded.push(raw_n);
                    let comb = StringJoiner.init();
                    defer comb.~init();
                    comb.push(prefix);
                    comb.push(padded.slice());
                    comb.push(suffix);
                    expandRecursive(allocator, comb.slice(), out, depth + 1);
                } else {
                    num_str = std.fmt.bufPrint(num_buf[0..64], "{d}", .{cur}) catch "0";
                    let comb = StringJoiner.init();
                    defer comb.~init();
                    comb.push(prefix);
                    comb.push(num_str);
                    comb.push(suffix);
                    expandRecursive(allocator, comb.slice(), out, depth + 1);
                }
                cur += delta;
                guard += 1;
            }
            return;
        }
    }

    if (p1.len == 1 && p2.len == 1) {
        const c1 = p1[0] as i64;
        const c2 = p2[0] as i64;
        const dir: i64 = if (c1 <= c2) 1 else -1;
        const delta = dir * step_val;
        let cur = c1;
        let guard: usize = 0;
        while (guard < MAX_BRACE_RESULTS) {
            if (dir > 0 && cur > c2) break;
            if (dir < 0 && cur < c2) break;
            if (out.items.len >= MAX_BRACE_RESULTS) break;
            let ch_arr: [1]u8 = [cur as u8];
            let comb = StringJoiner.init();
            defer comb.~init();
            comb.push(prefix);
            comb.push(ch_arr[0..1]);
            comb.push(suffix);
            expandRecursive(allocator, comb.slice(), out, depth + 1);
            cur += delta;
            guard += 1;
        }
    }
}

fn expandRecursive(
    allocator: std.mem.Allocator,
    raw: []const u8,
    out: *std.ArrayListUnmanaged(string),
    depth: usize,
) {
    if (out.items.len >= MAX_BRACE_RESULTS || depth > 16) {
        const dup = allocator.dupe(u8, raw) catch return;
        out.append(allocator, dup) catch {};
        return;
    }
    if (!hasUnquotedBrace(raw)) {
        const dup = allocator.dupe(u8, raw) catch return;
        out.append(allocator, dup) catch {};
        return;
    }

    const found_opt = findFirstExpandableOpen(raw);
    if (found_opt == null) {
        const dup = allocator.dupe(u8, raw) catch return;
        out.append(allocator, dup) catch {};
        return;
    }
    const found = found_opt.?;
    const prefix = raw[0..found.open];
    const inner = raw[found.open + 1 .. found.close];
    const suffix = raw[found.close + 1 ..];

    if (isCommaList(inner)) {
        let seg_start: usize = 0;
        let bdepth: usize = 0;
        let in_single = false;
        let in_double = false;
        let i: usize = 0;
        while (i <= inner.len) {
            let at_split = (i == inner.len);
            if (!at_split) {
                const ch = inner[i];
                if (in_single) {
                    if (ch == '\'') in_single = false;
                } else if (in_double) {
                    if (ch == '\\' && i + 1 < inner.len) { i += 2; continue; }
                    if (ch == '"') in_double = false;
                } else {
                    if (ch == '\\' && i + 1 < inner.len) { i += 2; continue; }
                    if (ch == '\'') in_single = true;
                    else if (ch == '"') in_double = true;
                    else if (ch == '{') bdepth += 1;
                    else if (ch == '}') { if (bdepth > 0) bdepth -= 1; }
                    else if (ch == ',' && bdepth == 0) at_split = true;
                }
            }
            if (at_split) {
                const item = inner[seg_start..i];
                let comb = StringJoiner.init();
                defer comb.~init();
                comb.push(prefix);
                comb.push(item);
                comb.push(suffix);
                expandRecursive(allocator, comb.slice(), out, depth + 1);
                seg_start = i + 1;
            }
            i += 1;
        }
        return;
    }

    if (isRangeSpec(inner)) {
        expandRangeSpec(allocator, prefix, inner, suffix, out, depth);
        return;
    }

    const dup = allocator.dupe(u8, raw) catch return;
    out.append(allocator, dup) catch {};
}

pub struct BraceExpander {
    pub static fn expand(allocator: std.mem.Allocator, raw: []const u8): [][]const u8 {
        let out: std.ArrayListUnmanaged(string) = .empty;
        expandRecursive(allocator, raw, &out, 0);
        if (out.items.len == 0) {
            const dup = allocator.dupe(u8, raw) catch return &.{};
            out.append(allocator, dup) catch {};
        }
        return out.toOwnedSlice(allocator) catch &.{};
    }

    pub static fn freeResults(allocator: std.mem.Allocator, items: [][]const u8) {
        for (item of items) {
            allocator.free(item);
        }
        allocator.free(items);
    }
}
