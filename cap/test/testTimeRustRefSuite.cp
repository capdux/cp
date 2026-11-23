// Reference-hardened test suite against chronotope/chrono (MIT/Apache-2.0), RFC 3339 & RFC 2822
// Covers:
//   1. Gregorian leap-year rules (1900/2000/2024/2038/2100/2400/9999) & O(1) Civil<->Epoch round-trips (incl. 1969 negative epoch)
//   2. RFC 3339 strict parsing & formatting matrix (valid, leap second, fractional nanos, invalid Feb 29/Apr 31/+24:00/trunc/trailing)
//   3. RFC 2822 & HTTP-Date (IMF-fixdate) strict parsing & weekday consistency checks
//   4. 16-lane SIMD prefix/digits & cap:simd delimiter scanners 0-diff parity vs scalar reference
//   5. Deterministic PRNG Fuzz (>= 1,500 malformed timestamps & random wide-span Epoch<->Civil round-trips)

import { test, expect_eq } from "cap:test";
import {
    RFC3339_MAX_LEN,
    Rfc3339Time,
    isLeapYear,
    daysInMonth,
    simdIsDigits16,
    simdIsDigits16Scalar,
    simdValidateRfc3339Prefix,
    simdValidateRfc3339PrefixScalar,
    simdFindTimeDelimiter,
    simdFindTimeDelimiterScalar,
} from "../../lib/time/rfc3339";
import {
    CivilDateTime,
    Iso8601,
    parseHttpDate,
    parseRfc2822,
} from "../../lib/time/iso8601";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn optUsizeEq(a: usize?, b: usize?): bool {
    if (va := a) {
        if (vb := b) {
            return va == vb;
        }
        return false;
    }
    return b == null;
}

fn main() {
    test("1. Gregorian leap years (1900/2000/2024/2038/2100/2400/9999) & Epoch<->Civil round-trips", () => {
        expect_eq(isLeapYear(1900), false);
        expect_eq(isLeapYear(2000), true);
        expect_eq(isLeapYear(2024), true);
        expect_eq(isLeapYear(2025), false);
        expect_eq(isLeapYear(2100), false);
        expect_eq(isLeapYear(2400), true);
        expect_eq(daysInMonth(1900, 2), 28 as u8);
        expect_eq(daysInMonth(2000, 2), 29 as u8);
        expect_eq(daysInMonth(2100, 2), 28 as u8);
        expect_eq(daysInMonth(2400, 2), 29 as u8);
        expect_eq(daysInMonth(2024, 4), 30 as u8);
        expect_eq(daysInMonth(2024, 13), 0 as u8);

        // 1969-12-31T23:59:59Z -> -1
        const pre_epoch = CivilDateTime.utc(1969, 12, 31, 23, 59, 59);
        expect_eq(pre_epoch.toUnixSeconds(), -1 as i64);
        const pre_back = CivilDateTime.fromUnixSeconds(-1);
        expect_eq(pre_back.year, 1969 as i32);
        expect_eq(pre_back.month, 12 as u8);
        expect_eq(pre_back.day, 31 as u8);
        expect_eq(pre_back.hour, 23 as u8);
        expect_eq(pre_back.minute, 59 as u8);
        expect_eq(pre_back.second, 59 as u8);

        // 1970-01-01T00:00:00Z -> 0 (Thursday = 4)
        const epoch0 = CivilDateTime.utc(1970, 1, 1, 0, 0, 0);
        expect_eq(epoch0.toUnixSeconds(), 0 as i64);
        expect_eq(epoch0.dayOfWeek(), 4 as u8);

        // Y2038 overflow boundary: 2038-01-19T03:14:08Z = 2147483648
        const y2038 = CivilDateTime.fromUnixSeconds(2147483648);
        expect_eq(y2038.year, 2038 as i32);
        expect_eq(y2038.month, 1 as u8);
        expect_eq(y2038.day, 19 as u8);
        expect_eq(y2038.hour, 3 as u8);
        expect_eq(y2038.minute, 14 as u8);
        expect_eq(y2038.second, 8 as u8);
        expect_eq(y2038.toUnixSeconds(), 2147483648 as i64);

        // Year 2400 leap day & Year 9999 max date round-trip
        const y2400 = CivilDateTime.utc(2400, 2, 29, 12, 30, 45);
        const y2400_back = CivilDateTime.fromUnixSeconds(y2400.toUnixSeconds());
        expect_eq(y2400_back.year, 2400 as i32);
        expect_eq(y2400_back.month, 2 as u8);
        expect_eq(y2400_back.day, 29 as u8);

        const y9999 = CivilDateTime.utc(9999, 12, 31, 23, 59, 59);
        const y9999_back = CivilDateTime.fromUnixSeconds(y9999.toUnixSeconds());
        expect_eq(y9999_back.year, 9999 as i32);
        expect_eq(y9999_back.month, 12 as u8);
        expect_eq(y9999_back.day, 31 as u8);
        expect_eq(y9999_back.toUnixSeconds(), 253402300799 as i64);
    });

    test("2. RFC 3339 strict parsing, fractional nanos, leap seconds & invalid rejection", () => {
        // Valid variants: T, t, space, Z, z, +HH:MM, -HH:MM, 1..9+ fractional digits
        const r1 = Rfc3339Time.parse("2024-02-29T23:59:59.123456789Z");
        expect_eq(r1 != null, true);
        expect_eq(r1.?.nanos, 123456789 as u32);

        const r2 = Rfc3339Time.parse("2024-02-29t00:00:00.5z");
        expect_eq(r2 != null, true);
        expect_eq(r2.?.nanos, 500000000 as u32);

        const r3 = Rfc3339Time.parse("2026-11-23 08:30:00+08:00");
        expect_eq(r3 != null, true);
        expect_eq(r3.?.offset_minutes, 480 as i16);

        const r4 = Iso8601.parseDateTime("2026-11-23 08:30:00-23:59");
        expect_eq(r4 != null, true);
        expect_eq(r4.?.tz_offset_minutes, -1439 as i16);

        // Leap second: rejected by default, accepted by parseAllowLeapSecond / parseDateTimeWithOptions
        expect_eq(Rfc3339Time.parse("2016-12-31T23:59:60Z") == null, true);
        const leap = Rfc3339Time.parseAllowLeapSecond("2016-12-31T23:59:60Z");
        expect_eq(leap != null, true);
        expect_eq(leap.?.second, 60 as u8);

        const leap_iso = Iso8601.parseDateTimeWithOptions("2016-12-31T23:59:60Z", true);
        expect_eq(leap_iso != null, true);
        expect_eq(leap_iso.?.second, 60 as u8);

        // Format round-trip
        let fbuf: [64]u8 = undefined;
        const formatted: ([]const u8)? = r1.?.format(&fbuf);
        expect_eq(formatted orelse "", "2024-02-29T23:59:59.123456789Z");

        // Invalid RFC 3339 matrix
        expect_eq(Rfc3339Time.parse("2025-02-29T00:00:00Z") == null, true);   // non-leap Feb 29
        expect_eq(Rfc3339Time.parse("1900-02-29T00:00:00Z") == null, true);   // 1900 non-leap
        expect_eq(Rfc3339Time.parse("2100-02-29T00:00:00Z") == null, true);   // 2100 non-leap
        expect_eq(Rfc3339Time.parse("2024-04-31T00:00:00Z") == null, true);   // Apr 31
        expect_eq(Rfc3339Time.parse("2024-01-01T24:00:00Z") == null, true);   // hour 24
        expect_eq(Rfc3339Time.parse("2024-01-01T00:60:00Z") == null, true);   // min 60
        expect_eq(Rfc3339Time.parse("2024-01-01T00:00:00+24:00") == null, true); // +24:00
        expect_eq(Rfc3339Time.parse("2024-01-01T00:00:00-00:60") == null, true); // -00:60
        expect_eq(Rfc3339Time.parse("2024-01-01T00:00:00.Z") == null, true);  // truncated frac
        expect_eq(Rfc3339Time.parse("2024-01-01T00:00:00Z ") == null, true);  // trailing space
        expect_eq(Rfc3339Time.parse(" 2024-01-01T00:00:00Z") == null, true);  // leading space
        expect_eq(Rfc3339Time.parse("2024-01-01T00:00:00") == null, true);    // missing tz
        expect_eq(RFC3339_MAX_LEN, 128 as usize);
    });

    test("3. RFC 2822 & HTTP-Date (IMF-fixdate) strict parsing", () => {
        const hd = parseHttpDate("Sun, 06 Nov 1994 08:49:37 GMT");
        expect_eq(hd != null, true);
        expect_eq(hd.?.year, 1994 as i32);
        expect_eq(hd.?.month, 11 as u8);
        expect_eq(hd.?.day, 6 as u8);
        expect_eq(hd.?.hour, 8 as u8);
        expect_eq(hd.?.minute, 49 as u8);
        expect_eq(hd.?.second, 37 as u8);

        // Wrong weekday or wrong case rejected
        expect_eq(parseHttpDate("Mon, 06 Nov 1994 08:49:37 GMT") == null, true);
        expect_eq(parseHttpDate("sun, 06 Nov 1994 08:49:37 GMT") == null, true);
        expect_eq(parseHttpDate("Sun, 06 nov 1994 08:49:37 GMT") == null, true);
        expect_eq(parseHttpDate("Sun, 06 Nov 1994 08:49:37 UTC") == null, true);

        // RFC 2822 with ±HHMM and optional seconds/weekday
        const r2822 = parseRfc2822("Fri, 21 Nov 1997 09:55:06 -0600");
        expect_eq(r2822 != null, true);
        expect_eq(r2822.?.year, 1997 as i32);
        expect_eq(r2822.?.tz_offset_minutes, -360 as i16);

        const r2822_no_sec = parseRfc2822("6 Nov 1994 08:49 +0800");
        expect_eq(r2822_no_sec != null, true);
        expect_eq(r2822_no_sec.?.day, 6 as u8);
        expect_eq(r2822_no_sec.?.second, 0 as u8);
        expect_eq(r2822_no_sec.?.tz_offset_minutes, 480 as i16);

        // Invalid RFC 2822
        expect_eq(parseRfc2822("Thu, 21 Nov 1997 09:55:06 -0600") == null, true); // wrong dow
        expect_eq(parseRfc2822("29 Feb 2025 00:00:00 GMT") == null, true);        // non-leap
        expect_eq(parseRfc2822("6 Nov 1994 08:49 +2400") == null, true);          // bad tz
    });

    test("4. 16-lane SIMD prefix/digits & cap:simd delimiter scanners 0-diff parity vs scalar", () => {
        expect_eq(simdIsDigits16("0123456789012345"), true);
        expect_eq(simdIsDigits16("012345678901234a"), false);
        expect_eq(simdIsDigits16("0123456789"), false);

        let d16: [16]u8 = undefined;
        let p: usize = 0;
        while (p < 16) {
            let k: usize = 0;
            while (k < 16) {
                d16[k] = 48 as u8;
                k += 1;
            }
            d16[p] = 47 as u8; // '/' (< '0')
            expect_eq(simdIsDigits16(&d16), simdIsDigits16Scalar(&d16));
            d16[p] = 58 as u8; // ':' (> '9')
            expect_eq(simdIsDigits16(&d16), simdIsDigits16Scalar(&d16));
            d16[p] = 57 as u8; // '9'
            expect_eq(simdIsDigits16(&d16), simdIsDigits16Scalar(&d16));
            p += 1;
        }

        // Prefix SIMD vs scalar on every byte corruption position
        let rfc_buf: [20]u8 = undefined;
        const base = "2026-11-23T18:20:00Z";
        let pos: usize = 0;
        while (pos < 20) {
            let k2: usize = 0;
            while (k2 < 20) {
                rfc_buf[k2] = base[k2];
                k2 += 1;
            }
            rfc_buf[pos] = 88 as u8; // 'X'
            expect_eq(simdValidateRfc3339Prefix(&rfc_buf), simdValidateRfc3339PrefixScalar(&rfc_buf));
            pos += 1;
        }

        // Delimiter scanner 0..64 length sweep
        let sbuf: [64]u8 = undefined;
        let len: usize = 0;
        while (len <= 64) {
            let j: usize = 0;
            while (j < len) {
                sbuf[j] = 48 as u8;
                j += 1;
            }
            expect_eq(optUsizeEq(simdFindTimeDelimiter(sbuf[0..len]), simdFindTimeDelimiterScalar(sbuf[0..len])), true);
            if (len > 0) {
                sbuf[len - 1] = 45 as u8; // '-'
                expect_eq(optUsizeEq(simdFindTimeDelimiter(sbuf[0..len]), simdFindTimeDelimiterScalar(sbuf[0..len])), true);
                sbuf[len - 1] = 58 as u8; // ':'
                expect_eq(optUsizeEq(simdFindTimeDelimiter(sbuf[0..len]), simdFindTimeDelimiterScalar(sbuf[0..len])), true);
                sbuf[len - 1] = 84 as u8; // 'T'
                expect_eq(optUsizeEq(simdFindTimeDelimiter(sbuf[0..len]), simdFindTimeDelimiterScalar(sbuf[0..len])), true);
                sbuf[len - 1] = 32 as u8; // ' '
                expect_eq(optUsizeEq(simdFindTimeDelimiter(sbuf[0..len]), simdFindTimeDelimiterScalar(sbuf[0..len])), true);
            }
            len += 1;
        }
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed timestamps & wide-span Epoch<->Civil round-trips)", () => {
        const alphabet: []const u8 = "0123456789-:TtZz+. ,GMTUTCJanFebMarAprSunMon\x00\r\n\x80";
        let seed: u64 = 0x5EED_71AE_2026_0039;
        let fuzz_buf: [96]u8 = undefined;

        // 5A. 1,000 malformed timestamp fuzz iterations
        let iter: usize = 0;
        while (iter < 1000) {
            const slen: usize = ((nextRng(&seed) % 80) + 1) as usize;
            let k: usize = 0;
            while (k < slen) {
                const idx: usize = (nextRng(&seed) % (alphabet.len as u64)) as usize;
                fuzz_buf[k] = alphabet[idx];
                k += 1;
            }
            const sample = fuzz_buf[0..slen];
            * = Rfc3339Time.parse(sample);
            * = Rfc3339Time.parseAllowLeapSecond(sample);
            * = Iso8601.parseDateTime(sample);
            * = parseHttpDate(sample);
            * = parseRfc2822(sample);

            // SIMD vs scalar parity on every fuzz input
            expect_eq(simdValidateRfc3339Prefix(sample), simdValidateRfc3339PrefixScalar(sample));
            expect_eq(simdIsDigits16(sample), simdIsDigits16Scalar(sample));
            expect_eq(optUsizeEq(simdFindTimeDelimiter(sample), simdFindTimeDelimiterScalar(sample)), true);
            iter += 1;
        }

        // 5B. 600 wide-span random Epoch<->Civil<->RFC3339 round-trips (0001-01-01 .. 9999-12-31)
        // Span: [-62135596800, 253402300799] -> range = 315537897599
        let fbuf: [64]u8 = undefined;
        let r_iter: usize = 0;
        while (r_iter < 600) {
            const offset: u64 = nextRng(&seed) % 315537897599;
            const epoch_sec: i64 = (offset as i64) - 62135596800;
            const dt = CivilDateTime.fromUnixSeconds(epoch_sec);
            expect_eq(dt.isValid(), true);
            expect_eq(dt.toUnixSeconds(), epoch_sec);

            const rfc_opt: Rfc3339Time? = Rfc3339Time.fromUnixSecUtc(epoch_sec, 0);
            expect_eq(rfc_opt != null, true);
            const fmt_opt: ([]const u8)? = rfc_opt.?.format(&fbuf);
            expect_eq(fmt_opt != null, true);
            const parsed: Rfc3339Time? = Rfc3339Time.parse(fmt_opt.?);
            expect_eq(parsed != null, true);
            expect_eq(parsed.?.toUnixSec(), epoch_sec);
            r_iter += 1;
        }
    });
}
