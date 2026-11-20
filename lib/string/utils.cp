// Source: /data/dev/bun/src/string/immutable.cp
// lib/string/utils.cp
// High-speed ASCII SIMD case conversion, trimming, searching, splitting, and replacement utilities
// Absorbed from /data/dev/bun/src/string/immutable.cp & /data/dev/bun/src/highway/

const std = @import("std");
import { SimdScanner } from "./simd_scan.cp";

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

/// Namespace facade grouping all string search/trim/case/split/replace helpers.
pub struct Strings {
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

    pub static fn trimStart(s: []const u8): []const u8 {
        let start: usize = 0;
        while (start < s.len && isAsciiWhitespace(s[start])) {
            start += 1;
        }
        return s[start..s.len];
    }

    pub static fn trimEnd(s: []const u8): []const u8 {
        let end: usize = s.len;
        while (end > 0 && isAsciiWhitespace(s[end - 1])) {
            end -= 1;
        }
        return s[0..end];
    }

    pub static fn trim(s: []const u8): []const u8 {
        return Strings.trimEnd(Strings.trimStart(s));
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

pub fn isAllAscii(input: []const u8): bool {
    return Strings.isAllAscii(input);
}

pub fn toLowerAscii(input: []const u8, out: []u8): []const u8 {
    return Strings.toLowerAscii(input, out);
}

pub fn toUpperAscii(input: []const u8, out: []u8): []const u8 {
    return Strings.toUpperAscii(input, out);
}

pub fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    return Strings.eqlIgnoreCaseAscii(a, b);
}

pub fn trimStart(s: []const u8): []const u8 {
    return Strings.trimStart(s);
}

pub fn trimEnd(s: []const u8): []const u8 {
    return Strings.trimEnd(s);
}

pub fn trim(s: []const u8): []const u8 {
    return Strings.trim(s);
}

pub fn trimChars(s: []const u8, chars: []const u8): []const u8 {
    return Strings.trimChars(s, chars);
}

pub fn startsWith(s: []const u8, prefix: []const u8): bool {
    return Strings.startsWith(s, prefix);
}

pub fn endsWith(s: []const u8, suffix: []const u8): bool {
    return Strings.endsWith(s, suffix);
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

pub fn replaceAll(s: []const u8, needle: []const u8, replacement: []const u8, out: []u8): []const u8 {
    return Strings.replaceAll(s, needle, replacement, out);
}

pub fn replaceAllAlloc(allocator: std.mem.Allocator, s: []const u8, needle: []const u8, replacement: []const u8): ![]u8 {
    return Strings.replaceAllAlloc(allocator, s, needle, replacement);
}
