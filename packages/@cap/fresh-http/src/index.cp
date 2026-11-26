// @cap/fresh-http — Zero-heap RFC 7232/9110 HTTP conditional request freshness evaluator in pure CP
// Clean-room MIT implementation inspired by npm `fresh` v0.5.
// Reuses:
//   - `containsIgnoreCase` from `@cap/unicase` for case-insensitive `Cache-Control: no-cache` detection.

import { containsIgnoreCase } from "../../unicase/src/index.cp";

// ============================================================================
// Private helpers (all prefixed with `frsh` to avoid module flattening clashes)
// ============================================================================

fn frshIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn frshIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn frshToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn frshStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn frshEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (frshToLower(a[i]) != frshToLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn frshTrim(s: string): string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && frshIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && frshIsSpace(s[b - 1])) {
        b = b - 1;
    }
    return s[a..b];
}

fn frshIsValidWeekday(s: string): bool {
    return frshStrEq(s, "Mon") || frshStrEq(s, "Tue") || frshStrEq(s, "Wed") ||
        frshStrEq(s, "Thu") || frshStrEq(s, "Fri") || frshStrEq(s, "Sat") ||
        frshStrEq(s, "Sun");
}

fn frshParseMonth(s: string): u64 {
    if (frshStrEq(s, "Jan")) { return 1; }
    if (frshStrEq(s, "Feb")) { return 2; }
    if (frshStrEq(s, "Mar")) { return 3; }
    if (frshStrEq(s, "Apr")) { return 4; }
    if (frshStrEq(s, "May")) { return 5; }
    if (frshStrEq(s, "Jun")) { return 6; }
    if (frshStrEq(s, "Jul")) { return 7; }
    if (frshStrEq(s, "Aug")) { return 8; }
    if (frshStrEq(s, "Sep")) { return 9; }
    if (frshStrEq(s, "Oct")) { return 10; }
    if (frshStrEq(s, "Nov")) { return 11; }
    if (frshStrEq(s, "Dec")) { return 12; }
    return 0;
}

fn frshIsLeapYear(y: u64): bool {
    if (y % 400 == 0) {
        return true;
    }
    if (y % 100 == 0) {
        return false;
    }
    return y % 4 == 0;
}

fn frshDaysInMonth(y: u64, m: u64): u64 {
    if (m == 1 || m == 3 || m == 5 || m == 7 || m == 8 || m == 10 || m == 12) {
        return 31;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) {
        return 30;
    }
    if (m == 2) {
        if (frshIsLeapYear(y)) {
            return 29;
        }
        return 28;
    }
    return 0;
}

fn frshParse2Digits(s: string, pos: usize, out_val: *u64): bool {
    if (pos + 2 > s.len) {
        return false;
    }
    if (!frshIsDigit(s[pos]) || !frshIsDigit(s[pos + 1])) {
        return false;
    }
    out_val.* = ((s[pos] - 48) as u64) * 10 + ((s[pos + 1] - 48) as u64);
    return true;
}

fn frshParse4Digits(s: string, pos: usize, out_val: *u64): bool {
    if (pos + 4 > s.len) {
        return false;
    }
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 4) {
        if (!frshIsDigit(s[pos + i])) {
            return false;
        }
        v = v * 10 + ((s[pos + i] - 48) as u64);
        i = i + 1;
    }
    out_val.* = v;
    return true;
}

// Parse an entity-tag (`"opaque"` or `W/"opaque"`).
// Sets `is_weak` and returns the quoted opaque-tag slice, or empty string if invalid.
fn frshParseEtag(raw: string, is_weak: *bool): string {
    const s: string = frshTrim(raw);
    if (s.len < 2) {
        is_weak.* = false;
        return "";
    }
    let start: usize = 0;
    if (s.len >= 4 && s[0] == 87 && s[1] == 47) {
        // Weak validator prefix `W/` (case-sensitive per RFC 7232 §2.3)
        is_weak.* = true;
        start = 2;
    } else {
        is_weak.* = false;
    }
    const quoted: string = s[start..s.len];
    if (quoted.len < 2 || quoted[0] != 34 || quoted[quoted.len - 1] != 34) {
        return "";
    }
    return quoted;
}

fn frshIsDirectiveBoundary(c: u8): bool {
    return frshIsSpace(c) || c == 44 || c == 61;
}

// ============================================================================
// Public API (RFC 7232 / RFC 9110 & npm `fresh` parity)
// ============================================================================

/// Return true if `cache_control` contains the `no-cache` directive (case-insensitive,
/// respecting HTTP Cache-Control token boundaries).
pub fn hasNoCacheDirective(cache_control: string): bool {
    if (!containsIgnoreCase(cache_control, "no-cache")) {
        return false;
    }
    const needle_len: usize = 8;
    let i: usize = 0;
    while (i + needle_len <= cache_control.len) {
        if (frshEqIgnoreCase(cache_control[i..(i + needle_len)], "no-cache")) {
            const left_ok: bool = if (i == 0) { true } else { frshIsSpace(cache_control[i - 1]) || cache_control[i - 1] == 44 };
            const right_ok: bool = if (i + needle_len == cache_control.len) {
                true
            } else {
                frshIsDirectiveBoundary(cache_control[i + needle_len])
            };
            if (left_ok && right_ok) {
                return true;
            }
        }
        i = i + 1;
    }
    return false;
}

/// RFC 7232 §2.3.2 weak ETag comparison: two entity-tags are equivalent if their
/// opaque-tags match character-by-character, regardless of `W/` weakness indicator.
pub fn etagWeakMatch(a: string, b: string): bool {
    let wa: bool = false;
    let wb: bool = false;
    const qa: string = frshParseEtag(a, &wa);
    const qb: string = frshParseEtag(b, &wb);
    if (qa.len == 0 || qb.len == 0) {
        return false;
    }
    return frshStrEq(qa, qb);
}

/// RFC 7232 §2.3.2 strong ETag comparison: both entity-tags must be strong (no `W/`)
/// and their opaque-tags must match character-by-character.
pub fn etagStrongMatch(a: string, b: string): bool {
    let wa: bool = false;
    let wb: bool = false;
    const qa: string = frshParseEtag(a, &wa);
    const qb: string = frshParseEtag(b, &wb);
    if (qa.len == 0 || qb.len == 0 || wa || wb) {
        return false;
    }
    return frshStrEq(qa, qb);
}

/// Parse an IMF-fixdate / RFC 1123 HTTP date string
/// (e.g. `"Wed, 21 Oct 2015 07:28:00 GMT"`) into a Unix timestamp in seconds.
/// Returns `null` on malformed dates or out-of-range calendar values.
pub fn parseHttpDate(date_str: string): i64? {
    const s: string = frshTrim(date_str);
    // Exact IMF-fixdate length is 29: "Wed, 21 Oct 2015 07:28:00 GMT"
    if (s.len != 29) {
        return null;
    }
    if (!frshIsValidWeekday(s[0..3])) {
        return null;
    }
    if (s[3] != 44 || s[4] != 32 || s[7] != 32 || s[11] != 32 ||
        s[16] != 32 || s[19] != 58 || s[22] != 58 || s[25] != 32) {
        return null;
    }
    if (!frshStrEq(s[26..29], "GMT")) {
        return null;
    }

    let day: u64 = 0;
    let year: u64 = 0;
    let hour: u64 = 0;
    let min: u64 = 0;
    let sec: u64 = 0;

    if (!frshParse2Digits(s, 5, &day)) { return null; }
    const month: u64 = frshParseMonth(s[8..11]);
    if (month == 0) { return null; }
    if (!frshParse4Digits(s, 12, &year)) { return null; }
    if (!frshParse2Digits(s, 17, &hour)) { return null; }
    if (!frshParse2Digits(s, 20, &min)) { return null; }
    if (!frshParse2Digits(s, 23, &sec)) { return null; }

    if (year < 1970 || hour > 23 || min > 59 || sec > 59) {
        return null;
    }
    const max_day: u64 = frshDaysInMonth(year, month);
    if (day < 1 || day > max_day) {
        return null;
    }

    // Pure unsigned u64 days since 1970-01-01
    const y_diff: u64 = year - 1970;
    const leap_days: u64 = (year - 1969) / 4 - (year - 1901) / 100 + (year - 1601) / 400;
    let total_days: u64 = y_diff * 365 + leap_days;
    let m: u64 = 1;
    while (m < month) {
        total_days = total_days + frshDaysInMonth(year, m);
        m = m + 1;
    }
    total_days = total_days + (day - 1);

    const ts: u64 = total_days * 86400 + hour * 3600 + min * 60 + sec;
    return ts as i64;
}

/// Evaluate HTTP response freshness according to RFC 7232 / RFC 9110:
/// - Returns `false` if neither `if_none_match` nor `if_modified_since` is present.
/// - Returns `false` if `cache_control` contains `no-cache`.
/// - Evaluates `If-None-Match` (`*` or comma-separated ETag list using weak comparison).
/// - Evaluates `If-Modified-Since` (`last_modified <= if_modified_since`).
/// - When both conditional headers are present, both must be satisfied.
pub fn isFresh(
    if_none_match: string,
    if_modified_since: string,
    cache_control: string,
    etag: string,
    last_modified: string,
): bool {
    const inm: string = frshTrim(if_none_match);
    const ims: string = frshTrim(if_modified_since);

    // Unconditional request
    if (inm.len == 0 && ims.len == 0) {
        return false;
    }

    // Cache-Control: no-cache forces revalidation
    if (cache_control.len > 0 && hasNoCacheDirective(cache_control)) {
        return false;
    }

    // 1. If-None-Match check
    if (inm.len > 0) {
        if (!frshStrEq(inm, "*")) {
            if (etag.len == 0) {
                return false;
            }
            let matched: bool = false;
            let start: usize = 0;
            let i: usize = 0;
            let in_quotes: bool = false;
            while (i <= inm.len) {
                const at_end: bool = i == inm.len;
                if (!at_end && inm[i] == 34) {
                    in_quotes = !in_quotes;
                }
                if (at_end || (!in_quotes && inm[i] == 44)) {
                    const candidate: string = frshTrim(inm[start..i]);
                    if (candidate.len > 0 && etagWeakMatch(candidate, etag)) {
                        matched = true;
                        break;
                    }
                    start = i + 1;
                }
                i = i + 1;
            }
            if (!matched) {
                return false;
            }
        }
    }

    // 2. If-Modified-Since check
    if (ims.len > 0) {
        if (last_modified.len == 0) {
            return false;
        }
        const ims_ts = parseHttpDate(ims);
        const lm_ts = parseHttpDate(last_modified);
        if (ims_ts == null || lm_ts == null) {
            return false;
        }
        const ims_val: i64 = ims_ts ?? 0;
        const lm_val: i64 = lm_ts ?? 0;
        if (lm_val > ims_val) {
            return false;
        }
    }

    return true;
}
