// @cap/fresh — Zero-allocation RFC 9110 HTTP Conditional Request Freshness Engine
// Clean-room MIT implementation in pure CP.
// Supports If-None-Match (strong/weak ETag list & `*`), If-Modified-Since vs Last-Modified,
// IMF-fixdate parsing, and Cache-Control: no-cache invalidation.

pub struct RequestHeaders {
    pub if_none_match: []const u8,
    pub if_modified_since: []const u8,
    pub cache_control: []const u8,
}

pub struct ResponseHeaders {
    pub etag: []const u8,
    pub last_modified: []const u8,
}

struct TrimRange {
    start: usize,
    end: usize,
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn trimRange(s: []const u8, start: usize, end: usize) -> TrimRange {
    let a = start;
    let b = end;
    while (a < b && isSpace(s[a])) {
        a += 1;
    }
    while (b > a && isSpace(s[b - 1])) {
        b -= 1;
    }
    return TrimRange { start: a, end: b };
}

fn eqIgnoreCase(s: []const u8, start: usize, end: usize, target_lower: []const u8) -> bool {
    if (end - start != target_lower.len) {
        return false;
    }
    let i: usize = 0;
    while (i < target_lower.len) {
        if (toLowerAscii(s[start + i]) != target_lower[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Checks if a `Cache-Control` header contains the `no-cache` directive (case-insensitive).
/// Avoids false positives on extension tokens such as `x-no-cache`.
pub fn hasNoCache(cache_control: []const u8) -> bool {
    const n = cache_control.len;
    let pos: usize = 0;
    while (pos < n) {
        const seg_start = pos;
        let in_quote = false;
        while (pos < n) {
            const ch = cache_control[pos];
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const seg_end = pos;

        const tr = trimRange(cache_control, seg_start, seg_end);
        // Directive name ends at '=', whitespace, or end of token
        let name_end = tr.start;
        while (name_end < tr.end && cache_control[name_end] != 61 && !isSpace(cache_control[name_end])) {
            name_end += 1;
        }
        if (eqIgnoreCase(cache_control, tr.start, name_end, "no-cache")) {
            return true;
        }

        if (pos < n && cache_control[pos] == 44) {
            pos += 1;
        }
    }
    return false;
}

struct ParsedEtag {
    start: usize,
    end: usize,
    weak: bool,
    valid: bool,
}

fn parseEtagSlice(s: []const u8, raw_start: usize, raw_end: usize) -> ParsedEtag {
    const tr = trimRange(s, raw_start, raw_end);
    let a = tr.start;
    const b = tr.end;
    let weak = false;
    if (b - a >= 2 && s[a] == 87 && s[a + 1] == 47) {
        // "W/" prefix
        weak = true;
        a += 2;
    }
    if (b - a < 2 || s[a] != 34 || s[b - 1] != 34) {
        return ParsedEtag { start: 0, end: 0, weak: false, valid: false };
    }
    return ParsedEtag { start: a, end: b, weak: weak, valid: true };
}

fn slicesEqual(a: []const u8, a_start: usize, a_end: usize, b: []const u8, b_start: usize, b_end: usize) -> bool {
    const alen = a_end - a_start;
    if (alen != b_end - b_start) {
        return false;
    }
    let i: usize = 0;
    while (i < alen) {
        if (a[a_start + i] != b[b_start + i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// RFC 9110 weak ETag comparison: strips optional `W/` prefix and compares quoted opaque-tags.
pub fn etagWeakMatch(a: []const u8, b: []const u8) -> bool {
    const pa = parseEtagSlice(a, 0, a.len);
    const pb = parseEtagSlice(b, 0, b.len);
    if (!pa.valid || !pb.valid) {
        return false;
    }
    return slicesEqual(a, pa.start, pa.end, b, pb.start, pb.end);
}

/// RFC 9110 strong ETag comparison: returns false if either ETag is weak (`W/`).
pub fn etagStrongMatch(a: []const u8, b: []const u8) -> bool {
    const pa = parseEtagSlice(a, 0, a.len);
    const pb = parseEtagSlice(b, 0, b.len);
    if (!pa.valid || !pb.valid || pa.weak || pb.weak) {
        return false;
    }
    return slicesEqual(a, pa.start, pa.end, b, pb.start, pb.end);
}

fn parse2Digits(s: []const u8, pos: usize) -> i32 {
    const c0 = s[pos];
    const c1 = s[pos + 1];
    if (c0 < 48 || c0 > 57 || c1 < 48 || c1 > 57) {
        return -1;
    }
    return ((c0 - 48) as i32) * 10 + ((c1 - 48) as i32);
}

fn parse4Digits(s: []const u8, pos: usize) -> i32 {
    const hi = parse2Digits(s, pos);
    const lo = parse2Digits(s, pos + 2);
    if (hi < 0 || lo < 0) {
        return -1;
    }
    return hi * 100 + lo;
}

fn parseMonth(s: []const u8, pos: usize) -> i32 {
    const c0 = s[pos];
    const c1 = s[pos + 1];
    const c2 = s[pos + 2];
    if (c0 == 74 && c1 == 97 && c2 == 110) { return 1; }  // Jan
    if (c0 == 70 && c1 == 101 && c2 == 98) { return 2; }  // Feb
    if (c0 == 77 && c1 == 97 && c2 == 114) { return 3; }  // Mar
    if (c0 == 65 && c1 == 112 && c2 == 114) { return 4; } // Apr
    if (c0 == 77 && c1 == 97 && c2 == 121) { return 5; }  // May
    if (c0 == 74 && c1 == 117 && c2 == 110) { return 6; } // Jun
    if (c0 == 74 && c1 == 117 && c2 == 108) { return 7; } // Jul
    if (c0 == 65 && c1 == 117 && c2 == 103) { return 8; } // Aug
    if (c0 == 83 && c1 == 101 && c2 == 112) { return 9; } // Sep
    if (c0 == 79 && c1 == 99 && c2 == 116) { return 10; } // Oct
    if (c0 == 78 && c1 == 111 && c2 == 118) { return 11; }// Nov
    if (c0 == 68 && c1 == 101 && c2 == 99) { return 12; } // Dec
    return -1;
}

fn isValidWeekday(s: []const u8, pos: usize) -> bool {
    const c0 = s[pos];
    const c1 = s[pos + 1];
    const c2 = s[pos + 2];
    if (c0 == 77 && c1 == 111 && c2 == 110) { return true; } // Mon
    if (c0 == 84 && c1 == 117 && c2 == 101) { return true; } // Tue
    if (c0 == 87 && c1 == 101 && c2 == 100) { return true; } // Wed
    if (c0 == 84 && c1 == 104 && c2 == 117) { return true; } // Thu
    if (c0 == 70 && c1 == 114 && c2 == 105) { return true; } // Fri
    if (c0 == 83 && c1 == 97 && c2 == 116) { return true; }  // Sat
    if (c0 == 83 && c1 == 117 && c2 == 110) { return true; } // Sun
    return false;
}

fn isLeapYear(y: u64) -> bool {
    return (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
}

fn daysInMonth(month: i32, leap: bool) -> i32 {
    if (month == 2) {
        if (leap) {
            return 29;
        }
        return 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

/// Parses an RFC 9110 IMF-fixdate string (e.g. `"Wed, 21 Oct 2015 07:28:00 GMT"`)
/// into Unix epoch seconds. Returns `-1` on invalid format or out-of-range date.
pub fn parseHttpDate(date_str: []const u8) -> i64 {
    const tr = trimRange(date_str, 0, date_str.len);
    if (tr.end - tr.start != 29) {
        return -1;
    }
    const p = tr.start;
    if (!isValidWeekday(date_str, p)) {
        return -1;
    }
    // Check fixed separators: ", " at +3..+5, ' ' at +7, +11, +16, ':' at +19, +22, " GMT" at +25..+29
    if (date_str[p + 3] != 44
        || date_str[p + 4] != 32
        || date_str[p + 7] != 32
        || date_str[p + 11] != 32
        || date_str[p + 16] != 32
        || date_str[p + 19] != 58
        || date_str[p + 22] != 58
        || date_str[p + 25] != 32
        || date_str[p + 26] != 71
        || date_str[p + 27] != 77
        || date_str[p + 28] != 84)
    {
        return -1;
    }

    const day = parse2Digits(date_str, p + 5);
    const month = parseMonth(date_str, p + 8);
    const year = parse4Digits(date_str, p + 12);
    const hour = parse2Digits(date_str, p + 17);
    const minute = parse2Digits(date_str, p + 20);
    const second = parse2Digits(date_str, p + 23);

    if (year < 1970 || month < 1 || month > 12 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return -1;
    }

    const y = year as u64;
    const leap = isLeapYear(y);
    const max_day = daysInMonth(month, leap);
    if (day < 1 || day > max_day) {
        return -1;
    }

    const ym1 = y - 1;
    const leaps_before_y = (ym1 / 4) - (ym1 / 100) + (ym1 / 400);
    const leaps_before_1970: u64 = (1969 / 4) - (1969 / 100) + (1969 / 400);
    const year_days = (y - 1970) * 365 + (leaps_before_y - leaps_before_1970);

    const cum_days: [12]u64 = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334];
    let month_days = cum_days[(month - 1) as usize];
    if (leap && month > 2) {
        month_days += 1;
    }

    const total_days = year_days + month_days + ((day - 1) as u64);
    const secs = total_days * 86400 + (hour as u64) * 3600 + (minute as u64) * 60 + (second as u64);
    return secs as i64;
}

fn matchIfNoneMatch(inm: []const u8, res_etag: []const u8) -> bool {
    const tr = trimRange(inm, 0, inm.len);
    if (tr.end - tr.start == 1 && inm[tr.start] == 42) {
        return true; // "*" wildcard
    }
    if (res_etag.len == 0) {
        return false;
    }

    const res_parsed = parseEtagSlice(res_etag, 0, res_etag.len);
    if (!res_parsed.valid) {
        return false;
    }

    let pos = tr.start;
    while (pos < tr.end) {
        const item_start = pos;
        let in_quote = false;
        while (pos < tr.end) {
            const ch = inm[pos];
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const item_end = pos;
        const item_parsed = parseEtagSlice(inm, item_start, item_end);
        if (item_parsed.valid && slicesEqual(inm, item_parsed.start, item_parsed.end, res_etag, res_parsed.start, res_parsed.end)) {
            return true;
        }
        if (pos < tr.end && inm[pos] == 44) {
            pos += 1;
        }
    }
    return false;
}

/// Evaluates RFC 9110 §13.2.2 HTTP conditional request freshness.
/// Returns true if the client's cached representation is fresh (server may reply `304 Not Modified`).
pub fn isFresh(req: RequestHeaders, res: ResponseHeaders) -> bool {
    const inm_tr = trimRange(req.if_none_match, 0, req.if_none_match.len);
    const ims_tr = trimRange(req.if_modified_since, 0, req.if_modified_since.len);
    const has_inm = inm_tr.start < inm_tr.end;
    const has_ims = ims_tr.start < ims_tr.end;

    if (!has_inm && !has_ims) {
        return false;
    }

    if (hasNoCache(req.cache_control)) {
        return false;
    }

    if (has_inm) {
        if (!matchIfNoneMatch(req.if_none_match, res.etag)) {
            return false;
        }
    }

    if (has_ims) {
        const ims_ts = parseHttpDate(req.if_modified_since);
        const lm_ts = parseHttpDate(res.last_modified);
        if (ims_ts < 0 || lm_ts < 0) {
            return false;
        }
        if (lm_ts > ims_ts) {
            return false;
        }
    }

    return true;
}
