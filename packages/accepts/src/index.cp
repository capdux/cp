// @cap/accepts — Zero-allocation RFC 9110 HTTP content negotiation engine for CP
// Clean-room MIT implementation composing @cap/negotiator directly.

import {
    bestEncoding,
    bestMediaType,
    bestLanguage,
    parseAcceptList,
} from "../../negotiator/src/core.cp";

fn acceptsIsSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn acceptsTrimSlice(s: []const u8) -> []const u8 {
    let start: usize = 0;
    while (start < s.len && acceptsIsSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && acceptsIsSpace(s[end - (1 as usize)])) {
        end -= 1;
    }
    return s[start..end];
}

// Parse RFC 9110 q-value into integer milli-units 0..1000 (-1 on invalid)
pub fn parseQValue(s_in: []const u8) -> i32 {
    const s = acceptsTrimSlice(s_in);
    if (s.len == (0 as usize) || s.len > (5 as usize)) {
        return -1 as i32;
    }
    const first = s[0];
    if (first != '0' && first != '1') {
        return -1 as i32;
    }
    if (s.len == (1 as usize)) {
        if (first == '1') {
            return 1000 as i32;
        }
        return 0 as i32;
    }
    if (s[1] != '.') {
        return -1 as i32;
    }
    if (first == '1') {
        let i: usize = 2;
        while (i < s.len) {
            if (s[i] != '0') {
                return -1 as i32;
            }
            i += 1;
        }
        return 1000 as i32;
    }

    // first == '0', s[1] == '.'
    let val: u32 = 0;
    let weight: u32 = 100;
    let i: usize = 2;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return -1 as i32;
        }
        val += ((c - '0') as u32) * weight;
        weight = weight / (10 as u32);
        i += 1;
    }
    return val as i32;
}

/// Negotiate best Accept-Encoding candidate via `@cap/negotiator`.
pub fn negotiateEncoding(accept_header: []const u8, available: []const []const u8) -> i64 {
    return bestEncoding(accept_header, available);
}

/// Negotiate best Accept-Language candidate via `@cap/negotiator`.
pub fn negotiateLanguage(accept_header: []const u8, available: []const []const u8) -> i64 {
    return bestLanguage(accept_header, available);
}

/// Negotiate best Accept media type candidate via `@cap/negotiator`.
pub fn negotiateMediaType(accept_header: []const u8, available: []const []const u8) -> i64 {
    return bestMediaType(accept_header, available);
}

export {
    parseQValue,
    parseAcceptList,
    negotiateEncoding,
    negotiateLanguage,
    negotiateMediaType,
};
