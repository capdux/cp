// Source: cp/lib/kmp_z_algo/kmp_z_algo.cp (Pure CP KMP, Z-Algorithm & Manacher Linear String Algorithms)
const std = @import("std");

pub fn computeLps(alloc: std.mem.Allocator, pattern: []const u8): ![]usize {
    let lps = try alloc.alloc(usize, pattern.len);
    if (pattern.len == 0) { return lps; }
    lps[0] = 0;
    let len: usize = 0;
    let i: usize = 1;
    while (i < pattern.len) {
        if (pattern[i] == pattern[len]) {
            len += 1;
            lps[i] = len;
            i += 1;
        } else if (len != 0) {
            len = lps[len - 1];
        } else {
            lps[i] = 0;
            i += 1;
        }
    }
    return lps;
}

pub fn kmpFindFirst(alloc: std.mem.Allocator, text: []const u8, pattern: []const u8): !usize? {
    if (pattern.len == 0) { return 0; }
    if (pattern.len > text.len) { return null; }

    const lps = try computeLps(alloc, pattern);
    defer alloc.free(lps);

    let i: usize = 0;
    let j: usize = 0;
    while (i < text.len) {
        if (text[i] == pattern[j]) {
            i += 1;
            j += 1;
            if (j == pattern.len) {
                return i - j;
            }
        } else if (j != 0) {
            j = lps[j - 1];
        } else {
            i += 1;
        }
    }
    return null;
}

pub fn kmpFindAll(alloc: std.mem.Allocator, text: []const u8, pattern: []const u8): ![]usize {
    let out = std.ArrayList(usize).empty;
    errdefer out.deinit(alloc);
    if (pattern.len == 0 or pattern.len > text.len) {
        return out.toOwnedSlice(alloc);
    }

    const lps = try computeLps(alloc, pattern);
    defer alloc.free(lps);

    let i: usize = 0;
    let j: usize = 0;
    while (i < text.len) {
        if (text[i] == pattern[j]) {
            i += 1;
            j += 1;
            if (j == pattern.len) {
                try out.append(alloc, i - j);
                j = lps[j - 1];
            }
        } else if (j != 0) {
            j = lps[j - 1];
        } else {
            i += 1;
        }
    }
    return out.toOwnedSlice(alloc);
}

pub fn zArray(alloc: std.mem.Allocator, s: []const u8): ![]usize {
    let z = try alloc.alloc(usize, s.len);
    if (s.len == 0) { return z; }
    @memset(z, 0);
    z[0] = s.len;

    let l: usize = 0;
    let r: usize = 0;
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (i <= r) {
            z[i] = @min(r - i + 1, z[i - l]);
        }
        while (i + z[i] < s.len and s[z[i]] == s[i + z[i]]) {
            z[i] += 1;
        }
        if (i + z[i] > r + 1) {
            l = i;
            r = i + z[i] - 1;
        }
    }
    return z;
}

pub const PalindromeSpan = struct {
    start: usize,
    end: usize,
};

/// Manacher's O(N) longest palindromic substring
pub fn longestPalindrome(alloc: std.mem.Allocator, s: []const u8): !PalindromeSpan {
    if (s.len <= 1) {
        return PalindromeSpan{ .start = 0, .end = s.len };
    }
    const t_len = s.len * 2 + 1;
    let t = try alloc.alloc(u8, t_len);
    defer alloc.free(t);
    let p = try alloc.alloc(usize, t_len);
    defer alloc.free(p);
    @memset(p, 0);

    let k: usize = 0;
    while (k < s.len) : (k += 1) {
        t[k * 2] = 0x00;
        t[k * 2 + 1] = s[k];
    }
    t[t_len - 1] = 0x00;

    let center: usize = 0;
    let right: usize = 0;
    let best_center: usize = 0;
    let best_len: usize = 0;

    let i: usize = 0;
    while (i < t_len) : (i += 1) {
        if (i < right) {
            const mirror = 2 * center - i;
            p[i] = @min(right - i, p[mirror]);
        }
        while (i + p[i] + 1 < t_len and i >= p[i] + 1 and t[i + p[i] + 1] == t[i - p[i] - 1]) {
            p[i] += 1;
        }
        if (i + p[i] > right) {
            center = i;
            right = i + p[i];
        }
        if (p[i] > best_len) {
            best_len = p[i];
            best_center = i;
        }
    }

    const start = (best_center - best_len) / 2;
    return PalindromeSpan{
        .start = start,
        .end = start + best_len,
    };
}
