// @cap/retry-after — Zero-heap RFC 9110 HTTP Retry-After Header Parser & Formatter
// Clean-room MIT implementation in pure CP (supports delay-seconds & IMF-fixdate HTTP-date).

import { trim } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { parseU64, writeStr, writeU64 } from "cap:fmt";

pub const KIND_INVALID: u8 = 0;
pub const KIND_DELAY_SECONDS: u8 = 1;
pub const KIND_HTTP_DATE: u8 = 2;

pub struct RetryAfter {
    kind: u8,
    delay_seconds: u64,
    epoch_seconds: i64,
    valid: bool,
}

fn parseUint(s: []const u8): i64 {
    if (s.len == 0 || s.len > 15 || s[0] == '+') {
        return -1;
    }
    if (v := parseU64(s)) {
        return v as i64;
    }
    return -1;
}

fn parseMonth(s: []const u8): i32 {
    if (bytesEq(s, "Jan")) { return 1; }
    if (bytesEq(s, "Feb")) { return 2; }
    if (bytesEq(s, "Mar")) { return 3; }
    if (bytesEq(s, "Apr")) { return 4; }
    if (bytesEq(s, "May")) { return 5; }
    if (bytesEq(s, "Jun")) { return 6; }
    if (bytesEq(s, "Jul")) { return 7; }
    if (bytesEq(s, "Aug")) { return 8; }
    if (bytesEq(s, "Sep")) { return 9; }
    if (bytesEq(s, "Oct")) { return 10; }
    if (bytesEq(s, "Nov")) { return 11; }
    if (bytesEq(s, "Dec")) { return 12; }
    return -1;
}

fn isLeapYear(y: i64): bool {
    return (@rem(y, 4) == 0 && @rem(y, 100) != 0) || @rem(y, 400) == 0;
}

fn daysInMonth(y: i64, m: i32): i32 {
    if (m == 1 || m == 3 || m == 5 || m == 7 || m == 8 || m == 10 || m == 12) { return 31; }
    if (m == 4 || m == 6 || m == 9 || m == 11) { return 30; }
    if (m == 2) {
        if (isLeapYear(y)) { return 29; }
        return 28;
    }
    return 0;
}

// Howard Hinnant civil-from-days inverse: days since 1970-01-01
fn daysFromCivil(y_in: i64, m_in: i64, d: i64): i64 {
    let y: i64 = y_in;
    if (m_in <= 2) {
        y -= 1;
    }
    let era: i64 = @divTrunc(y, 400);
    if (y < 0) {
        era = @divTrunc(y - 399, 400);
    }
    let yoe: i64 = y - era * 400;
    let mp: i64 = m_in + 9;
    if (m_in > 2) {
        mp = m_in - 3;
    }
    let doy: i64 = @divTrunc(153 * mp + 2, 5) + d - 1;
    let doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

// Parse IMF-fixdate: "Wed, 21 Oct 2015 07:28:00 GMT" (exact 29 bytes)
pub fn parseHttpDate(s: []const u8): i64 {
    if (s.len != 29) {
        return -1;
    }
    if (s[3] != ',' || s[4] != ' ' || s[7] != ' ' || s[11] != ' ' || s[16] != ' ' || s[19] != ':' || s[22] != ':' || s[25] != ' ') {
        return -1;
    }
    if (!bytesEq(s[26..29], "GMT")) {
        return -1;
    }
    let day: i64 = parseUint(s[5..7]);
    let mon: i32 = parseMonth(s[8..11]);
    let year: i64 = parseUint(s[12..16]);
    let hour: i64 = parseUint(s[17..19]);
    let min: i64 = parseUint(s[20..22]);
    let sec: i64 = parseUint(s[23..25]);
    if (day < 1 || mon < 1 || year < 1970 || hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 60) {
        return -1;
    }
    if (day > (daysInMonth(year, mon) as i64)) {
        return -1;
    }
    let days: i64 = daysFromCivil(year, mon as i64, day);
    return days * 86400 + hour * 3600 + min * 60 + sec;
}

pub fn parseRetryAfter(header: []const u8): RetryAfter {
    let s: []const u8 = trim(header);
    if (s.len == 0) {
        return RetryAfter { kind: KIND_INVALID, delay_seconds: 0, epoch_seconds: 0, valid: false };
    }
    let secs: i64 = parseUint(s);
    if (secs >= 0) {
        return RetryAfter {
            kind: KIND_DELAY_SECONDS,
            delay_seconds: secs as u64,
            epoch_seconds: 0,
            valid: true,
        };
    }
    let epoch: i64 = parseHttpDate(s);
    if (epoch >= 0) {
        return RetryAfter {
            kind: KIND_HTTP_DATE,
            delay_seconds: 0,
            epoch_seconds: epoch,
            valid: true,
        };
    }
    return RetryAfter { kind: KIND_INVALID, delay_seconds: 0, epoch_seconds: 0, valid: false };
}

// Compute wait duration in seconds relative to `now_epoch_sec` (clamped to >= 0; -1 if invalid)
pub fn waitSeconds(ra: RetryAfter, now_epoch_sec: i64): i64 {
    if (!ra.valid) {
        return -1;
    }
    if (ra.kind == KIND_DELAY_SECONDS) {
        return ra.delay_seconds as i64;
    }
    if (ra.epoch_seconds <= now_epoch_sec) {
        return 0;
    }
    return ra.epoch_seconds - now_epoch_sec;
}

pub fn formatDelaySeconds(seconds: u64, dst: []u8): i64 {
    let tmp: [24]u8 = undefined;
    const n = writeU64(seconds, tmp[0..24], 0);
    if (dst.len < n) {
        return -1;
    }
    return writeStr(tmp[0..n], dst, 0) as i64;
}
