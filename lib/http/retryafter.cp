// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:49:17Z (RFC 9110 §10.2.3 Retry-After Header Parser & Formatter)
// Pure-CP zero-allocation RFC 9110 Retry-After parser (delay-seconds and IMF-fixdate) and delay calculator.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn parseU64Strict(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = @as(u64, c - '0');
        if (val > 1844674407370955161) return null;
        if (val == 1844674407370955161 && d > 5) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

fn parseTwoDigits(s: []const u8): u32? {
    if (s.len != 2) return null;
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') return null;
    return @as(u32, s[0] - '0') * 10 + @as(u32, s[1] - '0');
}

fn parseFourDigits(s: []const u8): u32? {
    if (s.len != 4) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < 4) {
        if (s[i] < '0' || s[i] > '9') return null;
        val = val * 10 + @as(u32, s[i] - '0');
        i += 1;
    }
    return val;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn parseMonth3(m: []const u8): u32? {
    if (strEq(m, "Jan")) return 1;
    if (strEq(m, "Feb")) return 2;
    if (strEq(m, "Mar")) return 3;
    if (strEq(m, "Apr")) return 4;
    if (strEq(m, "May")) return 5;
    if (strEq(m, "Jun")) return 6;
    if (strEq(m, "Jul")) return 7;
    if (strEq(m, "Aug")) return 8;
    if (strEq(m, "Sep")) return 9;
    if (strEq(m, "Oct")) return 10;
    if (strEq(m, "Nov")) return 11;
    if (strEq(m, "Dec")) return 12;
    return null;
}

/// Howard Hinnant days-from-civil algorithm -> days since 1970-01-01.
fn daysFromCivil(year_in: i64, month_in: u32, day: u32): i64 {
    const y = if (month_in <= 2) year_in - 1 else year_in;
    const era = if (y >= 0) y / 400 else (y - 399) / 400;
    const yoe = y - era * 400;
    const m: i64 = @as(i64, month_in);
    const mp = if (m > 2) m - 3 else m + 9;
    const doy = (153 * mp + 2) / 5 + @as(i64, day) - 1;
    const doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

/// Parse IMF-fixdate (e.g. `"Wed, 21 Oct 2015 07:28:00 GMT"`) into Unix epoch seconds.
fn parseImfFixdate(s: []const u8): i64? {
    // Exact length: 29 bytes ("Day, DD Mon YYYY HH:MM:SS GMT")
    if (s.len != 29) return null;
    if (s[3] != ',' || s[4] != ' ' || s[7] != ' ' || s[11] != ' ' || s[16] != ' ' ||
        s[19] != ':' || s[22] != ':' || s[25] != ' ')
    {
        return null;
    }
    if (!strEq(s[26..29], "GMT")) return null;
    const day = parseTwoDigits(s[5..7]) ?? return null;
    const month = parseMonth3(s[8..11]) ?? return null;
    const year = parseFourDigits(s[12..16]) ?? return null;
    const hour = parseTwoDigits(s[17..19]) ?? return null;
    const min = parseTwoDigits(s[20..22]) ?? return null;
    const sec = parseTwoDigits(s[23..25]) ?? return null;

    if (day < 1 || day > 31 || hour > 23 || min > 59 || sec > 60) return null;
    const days = daysFromCivil(@as(i64, year), month, day);
    return days * 86400 + @as(i64, hour) * 3600 + @as(i64, min) * 60 + @as(i64, sec);
}

export enum RetryAfterKind {
    delay_seconds,
    http_date,
}

export struct RetryAfter {
    kind: RetryAfterKind,
    delay_sec: u64,
    target_unix_sec: i64,

    /// Parse an RFC 9110 §10.2.3 `Retry-After` header (either non-negative integer seconds or IMF-fixdate).
    pub fn parse(header: []const u8): RetryAfter? {
        const s = trimWs(header);
        if (s.len == 0) return null;
        if (secs := parseU64Strict(s)) {
            return RetryAfter {
                kind: RetryAfterKind.delay_seconds,
                delay_sec: secs,
                target_unix_sec: 0,
            };
        }
        if (ts := parseImfFixdate(s)) {
            return RetryAfter {
                kind: RetryAfterKind.http_date,
                delay_sec: 0,
                target_unix_sec: ts,
            };
        }
        return null;
    }

    /// Compute the wait delay in seconds relative to `now_unix_sec` (returns 0 if target date is in the past).
    pub fn waitSeconds(now_unix_sec: i64): u64 {
        if (_.kind == RetryAfterKind.delay_seconds) {
            return _.delay_sec;
        }
        if (_.target_unix_sec <= now_unix_sec) return 0;
        return @as(u64, _.target_unix_sec - now_unix_sec);
    }

    /// Compute wait delay clamped to `max_wait_sec`.
    pub fn waitSecondsCapped(now_unix_sec: i64, max_wait_sec: u64): u64 {
        const w = _.waitSeconds(now_unix_sec);
        return if (w > max_wait_sec) max_wait_sec else w;
    }
}

/// Format a delta-seconds `Retry-After` header value into `out`.
export fn formatRetryAfterSeconds(seconds: u64, out: []u8): ([]const u8)? {
    if (seconds == 0) {
        if (out.len < 1) return null;
        out[0] = '0';
        return out[0..1];
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = seconds;
    while (v > 0) {
        tmp[n] = @as(u8, '0') + @as(u8, v % 10);
        v /= 10;
        n += 1;
    }
    if (n > out.len) return null;
    let i: usize = 0;
    while (i < n) {
        out[i] = tmp[n - 1 - i];
        i += 1;
    }
    return out[0..n];
}
