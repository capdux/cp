import { test, expect_eq } from "cap:test";
import {
    parseHttpDate,
    toUnixSeconds,
    fromUnixSeconds,
    formatImfFixdate,
    formatUnixHttpDate,
    parseUnixSeconds,
} from "../../packages/httpdate/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/http-date — IMF-fixdate parsing & 29-byte formatting roundtrip", () => {
        const s = "Sun, 06 Nov 1994 08:49:37 GMT";
        const dt = parseHttpDate(s);
        expect_eq(dt.valid, true);
        expect_eq(dt.year, 1994 as u16);
        expect_eq(dt.month, 11 as u8);
        expect_eq(dt.day, 6 as u8);
        expect_eq(dt.hour, 8 as u8);
        expect_eq(dt.minute, 49 as u8);
        expect_eq(dt.second, 37 as u8);
        expect_eq(dt.weekday, 0 as u8); // Sun

        const ts = toUnixSeconds(dt);
        expect_eq(ts, 784111777 as i64);

        let buf: [32]u8 = undefined;
        const n = formatImfFixdate(dt, buf[0..32]);
        expect_eq(n, 29 as usize);
        expect_eq(testEq(buf[0..29], s), true);
    });

    test("2. @cap/http-date — Obsolete RFC 850 two-digit year parsing", () => {
        const ts94 = parseUnixSeconds("Sunday, 06-Nov-94 08:49:37 GMT");
        expect_eq(ts94, 784111777 as i64);

        const dt26 = parseHttpDate("Thursday, 26-Nov-26 12:30:00 GMT");
        expect_eq(dt26.valid, true);
        expect_eq(dt26.year, 2026 as u16);
        expect_eq(dt26.month, 11 as u8);
        expect_eq(dt26.day, 26 as u8);
    });

    test("3. @cap/http-date — ANSI C asctime() double-space day parsing", () => {
        const ts = parseUnixSeconds("Sun Nov  6 08:49:37 1994");
        expect_eq(ts, 784111777 as i64);

        const dt2 = parseHttpDate("Wed Oct 21 07:28:00 2015");
        expect_eq(dt2.valid, true);
        expect_eq(toUnixSeconds(dt2), 1445412480 as i64);
    });

    test("4. @cap/http-date — Epoch 0, 2024 leap day & post-2038 64-bit timestamps", () => {
        let buf: [32]u8 = undefined;
        const n0 = formatUnixHttpDate(0, buf[0..32]);
        expect_eq(n0, 29 as usize);
        expect_eq(testEq(buf[0..29], "Thu, 01 Jan 1970 00:00:00 GMT"), true);

        // 2024-02-29 12:00:00 UTC = 1709208000
        const leap_ts = parseUnixSeconds("Thu, 29 Feb 2024 12:00:00 GMT");
        expect_eq(leap_ts, 1709208000 as i64);
        const leap_dt = fromUnixSeconds(leap_ts);
        expect_eq(leap_dt.year, 2024 as u16);
        expect_eq(leap_dt.month, 2 as u8);
        expect_eq(leap_dt.day, 29 as u8);
        expect_eq(leap_dt.weekday, 4 as u8); // Thu

        // Post-2038: 2050-01-01 00:00:00 GMT = 2524608000 (exceeds signed 32-bit int)
        const future_ts: i64 = 2524608000;
        const nf = formatUnixHttpDate(future_ts, buf[0..32]);
        expect_eq(testEq(buf[0..nf], "Sat, 01 Jan 2050 00:00:00 GMT"), true);
        expect_eq(parseUnixSeconds(buf[0..nf]), future_ts);
    });

    test("5. @cap/http-date — Rejects invalid dates, Feb 30, non-leap Feb 29 & missing GMT", () => {
        expect_eq(parseHttpDate("").valid, false);
        expect_eq(parseHttpDate("Sun, 30 Feb 2024 00:00:00 GMT").valid, false);
        expect_eq(parseHttpDate("Wed, 29 Feb 2023 00:00:00 GMT").valid, false);
        expect_eq(parseHttpDate("Sun, 06 Nov 1994 08:49:37 UTC").valid, false);
        expect_eq(parseHttpDate("Sun, 06 Nov 1994 25:00:00 GMT").valid, false);
        expect_eq(parseUnixSeconds("invalid date"), -1 as i64);
        expect_eq(fromUnixSeconds(-1).valid, false);
    });

    test("6. @cap/http-date — Undersized buffer returns 0", () => {
        let tiny: [20]u8 = undefined;
        expect_eq(formatUnixHttpDate(784111777, tiny[0..20]), 0 as usize);
        const bad = parseHttpDate("bad");
        let ok_buf: [32]u8 = undefined;
        expect_eq(formatImfFixdate(bad, ok_buf[0..32]), 0 as usize);
    });
}
