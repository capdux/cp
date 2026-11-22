// Source: Absorbed from /data/dev/bun/src/string/immutable.cp | Synced: 2026-11-22T11:50:29Z
// lib/string/utils.cp
// High-speed ASCII SIMD case conversion, comparison, trimming, searching, splitting, join, concat, repeat & formatting
// Absorbed from /data/dev/bun/src/string/immutable.cp & /data/dev/bun/src/highway/

const std = @import("std");
import { SimdScanner } from "./simdScan.cp";
import { StringEscape } from "./escape.cp";

type StrVec16u8 = @Vector(16, u8);
type StrVec32u8 = @Vector(32, u8);

fn isAsciiWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0B || c == 0x0C;
}

fn containsByte(haystack: []const u8, b: u8): bool {
    let i: usize = 0;
    while (i < haystack.len) {
        if (haystack[i] == b) { return true; }
        i += 1;
    }
    return false;
}

/// Zero-allocation iterator produced by `split(input, delimiter)`.
pub struct SplitIterator {
    input: []const u8 = "",
    delimiter: []const u8 = "",
    pos: usize = 0,
    finished: bool = false,

    pub static fn init(input: []const u8, delimiter: []const u8): SplitIterator {
        return SplitIterator{
            input: input,
            delimiter: delimiter,
            pos: 0,
            finished: false,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.finished = false;
    }

    pub fn rest(*_): []const u8 {
        if (_.finished || _.pos >= _.input.len) {
            return "";
        }
        return _.input[_.pos.._.input.len];
    }

    pub fn next(*_): ([]const u8)? {
        if (_.finished) {
            return null;
        }
        if (_.delimiter.len == 0) {
            if (_.pos >= _.input.len) {
                _.finished = true;
                return null;
            }
            const s = _.pos;
            _.pos += 1;
            if (_.pos >= _.input.len) {
                _.finished = true;
            }
            return _.input[s .. s + 1];
        }
        if (found := indexOf(_.input[_.pos.._.input.len], _.delimiter)) {
            const start = _.pos;
            const end = start + found;
            _.pos = end + _.delimiter.len;
            return _.input[start..end];
        }
        const start = _.pos;
        const end = _.input.len;
        _.pos = end;
        _.finished = true;
        return _.input[start..end];
    }
}

/// Zero-allocation single-byte scalar split iterator produced by `splitScalar(input, delim)`.
pub struct ScalarSplitIterator {
    input: []const u8 = "",
    delimiter: u8 = 0,
    pos: usize = 0,
    finished: bool = false,

    pub static fn init(input: []const u8, delimiter: u8): ScalarSplitIterator {
        return ScalarSplitIterator{
            input: input,
            delimiter: delimiter,
            pos: 0,
            finished: false,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.finished = false;
    }

    pub fn rest(*_): []const u8 {
        if (_.finished || _.pos >= _.input.len) {
            return "";
        }
        return _.input[_.pos.._.input.len];
    }

    pub fn next(*_): ([]const u8)? {
        if (_.finished) {
            return null;
        }
        if (found := SimdScanner.indexOfByte(_.input[_.pos.._.input.len], _.delimiter)) {
            const start = _.pos;
            const end = start + found;
            _.pos = end + 1;
            return _.input[start..end];
        }
        const start = _.pos;
        const end = _.input.len;
        _.pos = end;
        _.finished = true;
        return _.input[start..end];
    }
}

/// Namespace facade grouping all Bun `strings` high-frequency helpers.
pub struct Strings {
    pub static fn isASCII(c: u8): bool {
        return c < 0x80;
    }

    pub static fn isAllAscii(input: []const u8): bool {
        let i: usize = 0;
        const n = input.len;
        const high32: StrVec32u8 = @splat(0x80);
        const zero32: StrVec32u8 = @splat(0);

        while (i + 32 <= n) {
            let chunk32: StrVec32u8 = @splat(0);
            zig {
                chunk32 = input[i..][0..32].*;
            }
            const mask32: u32 = @bitCast((chunk32 & high32) != zero32);
            if (mask32 != 0) {
                return false;
            }
            i += 32;
        }

        return SimdScanner.firstNonAsciiIndex(input[i..n]) == null;
    }

    pub static fn isAllASCII(input: []const u8): bool {
        return Strings.isAllAscii(input);
    }

    pub static fn eql(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) return false;
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, a, b);
        }
        return eq;
    }

    pub static fn eqlLong(a: []const u8, b: []const u8, check_len: bool): bool {
        if (check_len && a.len != b.len) return false;
        let eq: bool = false;
        zig {
            if (a.len == b.len and a.ptr == b.ptr) return true;
            eq = std.mem.eql(u8, a, b);
        }
        return eq;
    }

    pub static fn eqlComptime(a: []const u8, comptime b: []const u8): bool {
        if (a.len != b.len) return false;
        return Strings.eqlComptimeIgnoreLen(a, b);
    }

    pub static fn eqlComptimeIgnoreLen(a: []const u8, comptime b: []const u8): bool {
        if (a.len < b.len) return false;
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, a[0..b.len], b);
        }
        return eq;
    }

    pub static fn toLowerAscii(input: []const u8, out: []u8): []const u8 {
        const limit = if (input.len <= out.len) input.len else out.len;
        let i: usize = 0;
        const v_A: StrVec16u8 = @splat('A');
        const v_Z: StrVec16u8 = @splat('Z');
        const v_diff: StrVec16u8 = @splat(32);

        while (i + 16 <= limit) {
            zig {
                const chunk: StrVec16u8 = input[i..][0..16].*;
                const is_upper = (chunk >= v_A) & (chunk <= v_Z);
                const lowered: StrVec16u8 = @select(u8, is_upper, chunk + v_diff, chunk);
                out[i..][0..16].* = lowered;
            }
            i += 16;
        }

        while (i < limit) {
            const c = input[i];
            out[i] = if (c >= 'A' && c <= 'Z') (c + 32) else c;
            i += 1;
        }
        return out[0..limit];
    }

    pub static fn toLowerCaseASCII(input: []const u8, out: []u8): []const u8 {
        return Strings.toLowerAscii(input, out);
    }

    pub static fn toUpperAscii(input: []const u8, out: []u8): []const u8 {
        const limit = if (input.len <= out.len) input.len else out.len;
        let i: usize = 0;
        const v_a: StrVec16u8 = @splat('a');
        const v_z: StrVec16u8 = @splat('z');
        const v_diff: StrVec16u8 = @splat(32);

        while (i + 16 <= limit) {
            zig {
                const chunk: StrVec16u8 = input[i..][0..16].*;
                const is_lower = (chunk >= v_a) & (chunk <= v_z);
                const uppered: StrVec16u8 = @select(u8, is_lower, chunk - v_diff, chunk);
                out[i..][0..16].* = uppered;
            }
            i += 16;
        }

        while (i < limit) {
            const c = input[i];
            out[i] = if (c >= 'a' && c <= 'z') (c - 32) else c;
            i += 1;
        }
        return out[0..limit];
    }

    pub static fn toUpperCaseASCII(input: []const u8, out: []u8): []const u8 {
        return Strings.toUpperAscii(input, out);
    }

    pub static fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) {
            return false;
        }
        let i: usize = 0;
        const n = a.len;
        const v_A: StrVec16u8 = @splat('A');
        const v_Z: StrVec16u8 = @splat('Z');
        const v_diff: StrVec16u8 = @splat(32);

        while (i + 16 <= n) {
            let eq_mask: u16 = 0;
            zig {
                const ca: StrVec16u8 = a[i..][0..16].*;
                const cb: StrVec16u8 = b[i..][0..16].*;
                const la: StrVec16u8 = @select(u8, (ca >= v_A) & (ca <= v_Z), ca + v_diff, ca);
                const lb: StrVec16u8 = @select(u8, (cb >= v_A) & (cb <= v_Z), cb + v_diff, cb);
                eq_mask = @bitCast(la == lb);
            }
            if (eq_mask != 0xFFFF) {
                return false;
            }
            i += 16;
        }

        while (i < n) {
            let ca = a[i];
            let cb = b[i];
            if (ca >= 'A' && ca <= 'Z') { ca += 32; }
            if (cb >= 'A' && cb <= 'Z') { cb += 32; }
            if (ca != cb) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub static fn eqlCaseInsensitiveASCII(a: []const u8, b: []const u8, check_len: bool): bool {
        if (check_len && a.len != b.len) return false;
        if (a.len < b.len) return false;
        return Strings.eqlIgnoreCaseAscii(a[0..b.len], b);
    }

    pub static fn eqlCaseInsensitiveASCIIICheckLength(a: []const u8, b: []const u8): bool {
        return Strings.eqlIgnoreCaseAscii(a, b);
    }

    pub static fn eqlCaseInsensitiveASCIIIgnoreLength(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) return false;
        return Strings.eqlIgnoreCaseAscii(a, b);
    }

    pub static fn order(a: []const u8, b: []const u8): std.math.Order {
        return std.mem.order(u8, a, b);
    }

    pub static fn cmpStringsAsc(a: []const u8, b: []const u8): bool {
        return std.mem.order(u8, a, b) == .lt;
    }

    pub static fn cmpStringsDesc(a: []const u8, b: []const u8): bool {
        return std.mem.order(u8, a, b) == .gt;
    }

    pub static fn trimStart(s: []const u8): []const u8 {
        let start: usize = 0;
        while (start < s.len && isAsciiWhitespace(s[start])) {
            start += 1;
        }
        return s[start..s.len];
    }

    pub static fn trimLeft(s: []const u8): []const u8 {
        return Strings.trimStart(s);
    }

    pub static fn trimEnd(s: []const u8): []const u8 {
        let end: usize = s.len;
        while (end > 0 && isAsciiWhitespace(s[end - 1])) {
            end -= 1;
        }
        return s[0..end];
    }

    pub static fn trimRight(s: []const u8): []const u8 {
        return Strings.trimEnd(s);
    }

    pub static fn trim(s: []const u8): []const u8 {
        return Strings.trimEnd(Strings.trimStart(s));
    }

    pub static fn trimWhitespace(s: []const u8): []const u8 {
        return Strings.trim(s);
    }

    pub static fn trimChars(s: []const u8, chars: []const u8): []const u8 {
        let start: usize = 0;
        while (start < s.len && containsByte(chars, s[start])) {
            start += 1;
        }
        let end: usize = s.len;
        while (end > start && containsByte(chars, s[end - 1])) {
            end -= 1;
        }
        return s[start..end];
    }

    pub static fn trimPrefix(s: []const u8, prefix: []const u8): []const u8 {
        if (Strings.hasPrefix(s, prefix)) {
            return s[prefix.len..s.len];
        }
        return s;
    }

    pub static fn trimSuffix(s: []const u8, suffix: []const u8): []const u8 {
        if (Strings.hasSuffix(s, suffix)) {
            return s[0 .. s.len - suffix.len];
        }
        return s;
    }

    pub static fn startsWith(s: []const u8, prefix: []const u8): bool {
        if (s.len < prefix.len) {
            return false;
        }
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, s[0..prefix.len], prefix);
        }
        return eq;
    }

    pub static fn hasPrefix(s: []const u8, prefix: []const u8): bool {
        return Strings.startsWith(s, prefix);
    }

    pub static fn hasPrefixComptime(s: []const u8, comptime prefix: []const u8): bool {
        if (s.len < prefix.len) return false;
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, s[0..prefix.len], prefix);
        }
        return eq;
    }

    pub static fn endsWith(s: []const u8, suffix: []const u8): bool {
        if (s.len < suffix.len) {
            return false;
        }
        const start = s.len - suffix.len;
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, s[start..s.len], suffix);
        }
        return eq;
    }

    pub static fn hasSuffix(s: []const u8, suffix: []const u8): bool {
        return Strings.endsWith(s, suffix);
    }

    pub static fn hasSuffixComptime(s: []const u8, comptime suffix: []const u8): bool {
        if (s.len < suffix.len) return false;
        const start = s.len - suffix.len;
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, s[start..s.len], suffix);
        }
        return eq;
    }

    pub static fn startsWithCaseInsensitiveASCII(s: []const u8, prefix: []const u8): bool {
        if (s.len < prefix.len) return false;
        return Strings.eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
    }

    pub static fn endsWithCaseInsensitiveASCII(s: []const u8, suffix: []const u8): bool {
        if (s.len < suffix.len) return false;
        return Strings.eqlIgnoreCaseAscii(s[s.len - suffix.len .. s.len], suffix);
    }

    pub static fn indexOfChar(haystack: []const u8, c: u8): usize? {
        return SimdScanner.indexOfByte(haystack, c);
    }

    pub static fn indexOfCharPos(haystack: []const u8, c: u8, start: usize): usize? {
        if (start >= haystack.len) return null;
        if (hit := SimdScanner.indexOfByte(haystack[start..haystack.len], c)) {
            return start + hit;
        }
        return null;
    }

    pub static fn lastIndexOfChar(haystack: []const u8, c: u8): usize? {
        return std.mem.lastIndexOfScalar(u8, haystack, c);
    }

    pub static fn indexOfAny(haystack: []const u8, chars: []const u8): usize? {
        return std.mem.indexOfAny(u8, haystack, chars);
    }

    pub static fn indexOfNotChar(haystack: []const u8, c: u8): usize? {
        let i: usize = 0;
        while (i < haystack.len) {
            if (haystack[i] != c) return i;
            i += 1;
        }
        return null;
    }

    pub static fn containsChar(haystack: []const u8, c: u8): bool {
        return SimdScanner.indexOfByte(haystack, c) != null;
    }

    pub static fn contains(haystack: []const u8, needle: []const u8): bool {
        return Strings.indexOf(haystack, needle) != null;
    }

    pub static fn containsComptime(haystack: []const u8, comptime needle: []const u8): bool {
        if (needle.len == 0) return true;
        if (needle.len > haystack.len) return false;
        return Strings.indexOf(haystack, needle) != null;
    }

    pub static fn countChar(haystack: []const u8, c: u8): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < haystack.len) {
            if (haystack[i] == c) total += 1;
            i += 1;
        }
        return total;
    }

    pub static fn indexOf(haystack: []const u8, needle: []const u8): usize? {
        if (needle.len == 0) {
            return 0;
        }
        if (needle.len > haystack.len) {
            return null;
        }
        if (needle.len == 1) {
            return SimdScanner.indexOfByte(haystack, needle[0]);
        }
        const first = needle[0];
        const max_start = haystack.len - needle.len;
        let pos: usize = 0;
        while (pos <= max_start) {
            if (hit := SimdScanner.indexOfByte(haystack[pos .. max_start + 1], first)) {
                const candidate = pos + hit;
                let matched: bool = false;
                zig {
                    matched = std.mem.eql(u8, haystack[candidate .. candidate + needle.len], needle);
                }
                if (matched) {
                    return candidate;
                }
                pos = candidate + 1;
            } else {
                return null;
            }
        }
        return null;
    }

    pub static fn lastIndexOf(haystack: []const u8, needle: []const u8): usize? {
        if (needle.len == 0) {
            return haystack.len;
        }
        if (needle.len > haystack.len) {
            return null;
        }
        let pos = haystack.len - needle.len;
        while (true) {
            if (haystack[pos] == needle[0]) {
                let matched: bool = false;
                zig {
                    matched = std.mem.eql(u8, haystack[pos .. pos + needle.len], needle);
                }
                if (matched) {
                    return pos;
                }
            }
            if (pos == 0) {
                break;
            }
            pos -= 1;
        }
        return null;
    }

    pub static fn split(s: []const u8, delim: []const u8): SplitIterator {
        return SplitIterator.init(s, delim);
    }

    pub static fn splitScalar(s: []const u8, delim: u8): ScalarSplitIterator {
        return ScalarSplitIterator.init(s, delim);
    }

    pub static fn concatBuf(out: []u8, parts: []const []const u8): []const u8 {
        let pos: usize = 0;
        for (p of parts) {
            const avail = out.len - pos;
            const take = if (p.len <= avail) p.len else avail;
            if (take > 0) {
                @memcpy(out[pos .. pos + take], p[0..take]);
                pos += take;
            }
            if (pos >= out.len) break;
        }
        return out[0..pos];
    }

    pub static fn concat(allocator: std.mem.Allocator, parts: []const []const u8): ![]u8 {
        let total: usize = 0;
        for (p of parts) {
            total += p.len;
        }
        const out = try allocator.alloc(u8, total);
        let pos: usize = 0;
        for (p of parts) {
            if (p.len > 0) {
                @memcpy(out[pos .. pos + p.len], p);
                pos += p.len;
            }
        }
        return out;
    }

    pub static fn join(allocator: std.mem.Allocator, sep: []const u8, parts: []const []const u8): ![]u8 {
        if (parts.len == 0) {
            return allocator.dupe(u8, "");
        }
        let total: usize = sep.len * (parts.len - 1);
        for (p of parts) {
            total += p.len;
        }
        const out = try allocator.alloc(u8, total);
        let pos: usize = 0;
        let idx: usize = 0;
        for (p of parts) {
            if (idx > 0 && sep.len > 0) {
                @memcpy(out[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            if (p.len > 0) {
                @memcpy(out[pos .. pos + p.len], p);
                pos += p.len;
            }
            idx += 1;
        }
        return out;
    }

    pub static fn repeat(allocator: std.mem.Allocator, s: []const u8, n: usize): ![]u8 {
        if (n == 0 || s.len == 0) {
            return allocator.dupe(u8, "");
        }
        const total = s.len * n;
        const out = try allocator.alloc(u8, total);
        let pos: usize = 0;
        let k: usize = 0;
        while (k < n) {
            @memcpy(out[pos .. pos + s.len], s);
            pos += s.len;
            k += 1;
        }
        return out;
    }

    pub static fn repeatingAlloc(allocator: std.mem.Allocator, count: usize, ch: u8): ![]u8 {
        const buf = try allocator.alloc(u8, count);
        @memset(buf, ch);
        return buf;
    }

    pub static fn repeatingBuf(out: []u8, ch: u8) {
        @memset(out, ch);
    }

    pub static fn formatZigString(allocator: std.mem.Allocator, comptime fmt_str: []const u8, args: auto): ![]u8 {
        return std.fmt.allocPrint(allocator, fmt_str, args);
    }

    pub static fn escapeHtml(input: []const u8, out: []u8): usize {
        return StringEscape.escapeHtml(input, out);
    }

    pub static fn escapeHtmlAlloc(allocator: std.mem.Allocator, input: []const u8): ![]u8 {
        return StringEscape.escapeHtmlAlloc(allocator, input);
    }

    pub static fn escapeRegExp(input: []const u8, out: []u8): usize {
        return StringEscape.escapeRegExp(input, out);
    }

    pub static fn escapeRegExpAlloc(allocator: std.mem.Allocator, input: []const u8): ![]u8 {
        return StringEscape.escapeRegExpAlloc(allocator, input);
    }

    pub static fn replaceAll(s: []const u8, needle: []const u8, replacement: []const u8, out: []u8): []const u8 {
        if (needle.len == 0) {
            const copy_len = if (s.len <= out.len) s.len else out.len;
            zig {
                @memcpy(out[0..copy_len], s[0..copy_len]);
            }
            return out[0..copy_len];
        }
        let src_pos: usize = 0;
        let dst_pos: usize = 0;
        while (src_pos < s.len) {
            if (hit := Strings.indexOf(s[src_pos..s.len], needle)) {
                const prefix_len = hit;
                const safe_prefix = if (dst_pos + prefix_len <= out.len) prefix_len else (out.len - dst_pos);
                if (safe_prefix > 0) {
                    zig {
                        @memcpy(out[dst_pos .. dst_pos + safe_prefix], s[src_pos .. src_pos + safe_prefix]);
                    }
                    dst_pos += safe_prefix;
                }
                if (safe_prefix < prefix_len) {
                    break;
                }
                const safe_rep = if (dst_pos + replacement.len <= out.len) replacement.len else (out.len - dst_pos);
                if (safe_rep > 0) {
                    zig {
                        @memcpy(out[dst_pos .. dst_pos + safe_rep], replacement[0..safe_rep]);
                    }
                    dst_pos += safe_rep;
                }
                if (safe_rep < replacement.len) {
                    break;
                }
                src_pos += prefix_len + needle.len;
            } else {
                const rem = s.len - src_pos;
                const safe_rem = if (dst_pos + rem <= out.len) rem else (out.len - dst_pos);
                if (safe_rem > 0) {
                    zig {
                        @memcpy(out[dst_pos .. dst_pos + safe_rem], s[src_pos .. src_pos + safe_rem]);
                    }
                    dst_pos += safe_rem;
                }
                break;
            }
        }
        return out[0..dst_pos];
    }

    pub static fn replaceAllAlloc(allocator: std.mem.Allocator, s: []const u8, needle: []const u8, replacement: []const u8): ![]u8 {
        if (needle.len == 0) {
            return allocator.dupe(u8, s);
        }
        let count: usize = 0;
        let pos: usize = 0;
        while (pos <= s.len) {
            if (hit := Strings.indexOf(s[pos..s.len], needle)) {
                count += 1;
                pos += hit + needle.len;
            } else {
                break;
            }
        }
        if (count == 0) {
            return allocator.dupe(u8, s);
        }
        const out_len = s.len - count * needle.len + count * replacement.len;
        const out = try allocator.alloc(u8, out_len);
        * = Strings.replaceAll(s, needle, replacement, out);
        return out;
    }
}

pub const strings = Strings;

pub fn isASCII(c: u8): bool {
    return Strings.isASCII(c);
}

pub fn isAllAscii(input: []const u8): bool {
    return Strings.isAllAscii(input);
}

pub fn isAllASCII(input: []const u8): bool {
    return Strings.isAllAscii(input);
}

pub fn eql(a: []const u8, b: []const u8): bool {
    return Strings.eql(a, b);
}

pub fn eqlLong(a: []const u8, b: []const u8, check_len: bool): bool {
    return Strings.eqlLong(a, b, check_len);
}

pub fn eqlComptime(a: []const u8, comptime b: []const u8): bool {
    return Strings.eqlComptime(a, b);
}

pub fn eqlComptimeIgnoreLen(a: []const u8, comptime b: []const u8): bool {
    return Strings.eqlComptimeIgnoreLen(a, b);
}

pub fn toLowerAscii(input: []const u8, out: []u8): []const u8 {
    return Strings.toLowerAscii(input, out);
}

pub fn toLowerCaseASCII(input: []const u8, out: []u8): []const u8 {
    return Strings.toLowerAscii(input, out);
}

pub fn toUpperAscii(input: []const u8, out: []u8): []const u8 {
    return Strings.toUpperAscii(input, out);
}

pub fn toUpperCaseASCII(input: []const u8, out: []u8): []const u8 {
    return Strings.toUpperAscii(input, out);
}

pub fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    return Strings.eqlIgnoreCaseAscii(a, b);
}

pub fn eqlCaseInsensitiveASCII(a: []const u8, b: []const u8, check_len: bool): bool {
    return Strings.eqlCaseInsensitiveASCII(a, b, check_len);
}

pub fn eqlCaseInsensitiveASCIIICheckLength(a: []const u8, b: []const u8): bool {
    return Strings.eqlCaseInsensitiveASCIIICheckLength(a, b);
}

pub fn eqlCaseInsensitiveASCIIIgnoreLength(a: []const u8, b: []const u8): bool {
    return Strings.eqlCaseInsensitiveASCIIIgnoreLength(a, b);
}

pub fn order(a: []const u8, b: []const u8): std.math.Order {
    return Strings.order(a, b);
}

pub fn cmpStringsAsc(a: []const u8, b: []const u8): bool {
    return Strings.cmpStringsAsc(a, b);
}

pub fn cmpStringsDesc(a: []const u8, b: []const u8): bool {
    return Strings.cmpStringsDesc(a, b);
}

pub fn trimStart(s: []const u8): []const u8 {
    return Strings.trimStart(s);
}

pub fn trimLeft(s: []const u8): []const u8 {
    return Strings.trimStart(s);
}

pub fn trimEnd(s: []const u8): []const u8 {
    return Strings.trimEnd(s);
}

pub fn trimRight(s: []const u8): []const u8 {
    return Strings.trimEnd(s);
}

pub fn trim(s: []const u8): []const u8 {
    return Strings.trim(s);
}

pub fn trimWhitespace(s: []const u8): []const u8 {
    return Strings.trim(s);
}

pub fn trimChars(s: []const u8, chars: []const u8): []const u8 {
    return Strings.trimChars(s, chars);
}

pub fn trimPrefix(s: []const u8, prefix: []const u8): []const u8 {
    return Strings.trimPrefix(s, prefix);
}

pub fn trimSuffix(s: []const u8, suffix: []const u8): []const u8 {
    return Strings.trimSuffix(s, suffix);
}

pub fn startsWith(s: []const u8, prefix: []const u8): bool {
    return Strings.startsWith(s, prefix);
}

pub fn hasPrefix(s: []const u8, prefix: []const u8): bool {
    return Strings.hasPrefix(s, prefix);
}

pub fn hasPrefixComptime(s: []const u8, comptime prefix: []const u8): bool {
    return Strings.hasPrefixComptime(s, prefix);
}

pub fn endsWith(s: []const u8, suffix: []const u8): bool {
    return Strings.endsWith(s, suffix);
}

pub fn hasSuffix(s: []const u8, suffix: []const u8): bool {
    return Strings.hasSuffix(s, suffix);
}

pub fn hasSuffixComptime(s: []const u8, comptime suffix: []const u8): bool {
    return Strings.hasSuffixComptime(s, suffix);
}

pub fn startsWithCaseInsensitiveASCII(s: []const u8, prefix: []const u8): bool {
    return Strings.startsWithCaseInsensitiveASCII(s, prefix);
}

pub fn endsWithCaseInsensitiveASCII(s: []const u8, suffix: []const u8): bool {
    return Strings.endsWithCaseInsensitiveASCII(s, suffix);
}

pub fn indexOfChar(haystack: []const u8, c: u8): usize? {
    return Strings.indexOfChar(haystack, c);
}

pub fn indexOfCharPos(haystack: []const u8, c: u8, start: usize): usize? {
    return Strings.indexOfCharPos(haystack, c, start);
}

pub fn lastIndexOfChar(haystack: []const u8, c: u8): usize? {
    return Strings.lastIndexOfChar(haystack, c);
}

pub fn indexOfAny(haystack: []const u8, chars: []const u8): usize? {
    return Strings.indexOfAny(haystack, chars);
}

pub fn indexOfNotChar(haystack: []const u8, c: u8): usize? {
    return Strings.indexOfNotChar(haystack, c);
}

pub fn containsChar(haystack: []const u8, c: u8): bool {
    return Strings.containsChar(haystack, c);
}

pub fn contains(haystack: []const u8, needle: []const u8): bool {
    return Strings.contains(haystack, needle);
}

pub fn containsComptime(haystack: []const u8, comptime needle: []const u8): bool {
    return Strings.containsComptime(haystack, needle);
}

pub fn countChar(haystack: []const u8, c: u8): usize {
    return Strings.countChar(haystack, c);
}

pub fn indexOf(haystack: []const u8, needle: []const u8): usize? {
    return Strings.indexOf(haystack, needle);
}

pub fn lastIndexOf(haystack: []const u8, needle: []const u8): usize? {
    return Strings.lastIndexOf(haystack, needle);
}

pub fn split(s: []const u8, delim: []const u8): SplitIterator {
    return Strings.split(s, delim);
}

pub fn splitScalar(s: []const u8, delim: u8): ScalarSplitIterator {
    return Strings.splitScalar(s, delim);
}

pub fn concatBuf(out: []u8, parts: []const []const u8): []const u8 {
    return Strings.concatBuf(out, parts);
}

pub fn concat(allocator: std.mem.Allocator, parts: []const []const u8): ![]u8 {
    return Strings.concat(allocator, parts);
}

pub fn join(allocator: std.mem.Allocator, sep: []const u8, parts: []const []const u8): ![]u8 {
    return Strings.join(allocator, sep, parts);
}

pub fn repeat(allocator: std.mem.Allocator, s: []const u8, n: usize): ![]u8 {
    return Strings.repeat(allocator, s, n);
}

pub fn repeatingAlloc(allocator: std.mem.Allocator, count: usize, ch: u8): ![]u8 {
    return Strings.repeatingAlloc(allocator, count, ch);
}

pub fn repeatingBuf(out: []u8, ch: u8) {
    Strings.repeatingBuf(out, ch);
}

pub fn formatZigString(allocator: std.mem.Allocator, comptime fmt_str: []const u8, args: auto): ![]u8 {
    return Strings.formatZigString(allocator, fmt_str, args);
}

pub fn replaceAll(s: []const u8, needle: []const u8, replacement: []const u8, out: []u8): []const u8 {
    return Strings.replaceAll(s, needle, replacement, out);
}

pub fn replaceAllAlloc(allocator: std.mem.Allocator, s: []const u8, needle: []const u8, replacement: []const u8): ![]u8 {
    return Strings.replaceAllAlloc(allocator, s, needle, replacement);
}
