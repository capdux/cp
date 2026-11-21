// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-20T11:08:36Z
// ============================================================================
// lib/url/url_.cp — Low-level URL String Search, IP Detection & Port Helpers
// ============================================================================
const std = @import("std");

pub fn findSubstr(haystack: string, needle: []const u8): usize? {
    if (needle.len == 0) return 0 as usize;
    if (haystack.len < needle.len) return null;
    let max_idx = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= max_idx) {
        let is_match = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                is_match = false;
                break;
            }
            j += 1;
        }
        if (is_match) return i;
        i += 1;
    }
    return null;
}

pub fn urlIndexOfChar(s: string, c: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == c) return i;
        i += 1;
    }
    return null;
}

pub fn urlLastIndexOfChar(s: string, c: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == c) return i;
    }
    return null;
}

pub fn isIPv6Hostname(h: string): bool {
    return h.len >= 2 and h[0] == (91 as u8) and h[h.len - 1] == (93 as u8);
}

pub fn isIPv4Hostname(h: string): bool {
    let dots: usize = 0;
    let i: usize = 0;
    let has_digits = false;
    while (i < h.len) {
        let c = h[i];
        if (c == (46 as u8)) { // '.'
            if (!has_digits) return false;
            dots += 1;
            has_digits = false;
        } else if (c >= (48 as u8) and c <= (57 as u8)) {
            has_digits = true;
        } else {
            return false;
        }
        i += 1;
    }
    return dots == 3 and has_digits;
}

pub fn isIPHostname(h: string): bool {
    return isIPv6Hostname(h) or isIPv4Hostname(h);
}

pub fn isLocalhostName(h: string): bool {
    return h.len == 0 or
        std.mem.eql(u8, h, "localhost") or
        std.mem.eql(u8, h, "127.0.0.1") or
        std.mem.eql(u8, h, "0.0.0.0") or
        std.mem.eql(u8, h, "[::1]");
}

pub fn parsePortNumber(port_str: string): u16? {
    if (port_str.len == 0) return null;
    let p: u32 = 0;
    let i: usize = 0;
    while (i < port_str.len) {
        let c = port_str[i];
        if (c < (48 as u8) or c > (57 as u8)) return null;
        p = p * 10 + ((c - (48 as u8)) as u32);
        if (p > 65535) return null;
        i += 1;
    }
    return (p as u16);
}
