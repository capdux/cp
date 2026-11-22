// Source: Absorbed from /data/dev/bun/src/css/css_parser.cp | Synced: 2026-11-22T10:52:02Z
// lib/css/engine/_utils.cp
// Internal Shared Zero-Allocation String & Scanning Utilities for lib/css/engine/

const std = @import("std");

pub fn engAppendBytes(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

pub fn engTrimRight(s: []const u8, chars: []const u8): []const u8 {
    let end = s.len;
    while (end > 0) {
        if (std.mem.indexOfScalar(u8, chars, s[end - 1]) == null) break;
        end -= 1;
    }
    return s[0..end];
}

pub fn engFormatFloat(v: f64, out_buf: []u8): []const u8 {
    if (@abs(v) < 0.000000001) {
        return std.fmt.bufPrint(out_buf, "0", .{}) catch "0";
    }
    const rounded_i: i64 = @intFromFloat(@round(v));
    const rounded_f: f64 = @floatFromInt(rounded_i);
    if (@abs(v - rounded_f) < 0.000001) {
        return std.fmt.bufPrint(out_buf, "{d}", .{rounded_i}) catch "0";
    }
    let tmp: [64]u8 = [0; 64];
    const s = std.fmt.bufPrint(tmp[0..64], "{d:.4}", .{v}) catch return "0";
    let end = s.len;
    while (end > 0 && s[end - 1] == '0') end -= 1;
    if (end > 0 && s[end - 1] == '.') end -= 1;
    const slice = s[0..end];
    if (slice.len <= out_buf.len) {
        @memcpy(out_buf[0..slice.len], slice);
        return out_buf[0..slice.len];
    }
    return "";
}

pub fn engSplitCommaTop(raw: []const u8, out_parts: [][]const u8): usize {
    let count: usize = 0;
    let start: usize = 0;
    let paren_depth: usize = 0;
    let in_q: u8 = 0;
    let i: usize = 0;

    while (i < raw.len) {
        const c = raw[i];
        if (in_q != 0) {
            if (c == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (c == in_q) in_q = 0;
            i += 1;
            continue;
        }
        if (c == '"' || c == '\'') { in_q = c; i += 1; continue; }
        if (c == '(') paren_depth += 1;
        else if (c == ')' && paren_depth > 0) paren_depth -= 1;
        else if (c == ',' && paren_depth == 0) {
            if (count < out_parts.len) {
                out_parts[count] = std.mem.trim(u8, raw[start..i], " \t\r\n");
                count += 1;
            }
            start = i + 1;
        }
        i += 1;
    }
    const tail = std.mem.trim(u8, raw[start..], " \t\r\n");
    if (tail.len > 0 && count < out_parts.len) {
        out_parts[count] = tail;
        count += 1;
    }
    return count;
}

pub fn engSplitSpaceTop(raw: []const u8, out_parts: [][]const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    let paren_depth: usize = 0;
    let in_q: u8 = 0;
    let in_tok = false;
    let tok_start: usize = 0;

    while (i < raw.len) {
        const c = raw[i];
        if (in_q != 0) {
            if (c == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (c == in_q) in_q = 0;
            i += 1;
            continue;
        }
        if (c == '"' || c == '\'') {
            if (!in_tok) { in_tok = true; tok_start = i; }
            in_q = c;
            i += 1;
            continue;
        }
        if (c == '(') {
            if (!in_tok) { in_tok = true; tok_start = i; }
            paren_depth += 1;
            i += 1;
            continue;
        }
        if (c == ')' && paren_depth > 0) {
            paren_depth -= 1;
            i += 1;
            continue;
        }
        if ((c == ' ' || c == '\t' || c == '\r' || c == '\n') && paren_depth == 0) {
            if (in_tok) {
                if (count < out_parts.len) {
                    out_parts[count] = raw[tok_start..i];
                    count += 1;
                }
                in_tok = false;
            }
        } else if (!in_tok) {
            in_tok = true;
            tok_start = i;
        }
        i += 1;
    }
    if (in_tok && count < out_parts.len) {
        out_parts[count] = raw[tok_start..];
        count += 1;
    }
    return count;
}

pub fn engFindMatchingParen(s: []const u8, open_idx: usize): usize? {
    if (open_idx >= s.len || s[open_idx] != '(') return null;
    let depth: usize = 0;
    let in_q: u8 = 0;
    let i = open_idx;
    while (i < s.len) {
        const c = s[i];
        if (in_q != 0) {
            if (c == '\\' && i + 1 < s.len) { i += 2; continue; }
            if (c == in_q) in_q = 0;
            i += 1;
            continue;
        }
        if (c == '"' || c == '\'') { in_q = c; i += 1; continue; }
        if (c == '(') depth += 1;
        else if (c == ')') {
            if (depth > 0) depth -= 1;
            if (depth == 0) return i;
        }
        i += 1;
    }
    return null;
}
