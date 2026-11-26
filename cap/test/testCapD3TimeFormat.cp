import { test, expect_eq } from "cap:test";
import {
    utcFormat,
    utcParse,
} from "../../packages/@cap/d3-time-format/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn main() {
    test("1. @cap/d3-time-format — ISO-8601 UTC format & parse round-trip", () => {
        // 2026-11-26T15:04:05.678Z
        let buf: [64]u8 = undefined;
        const spec = "%Y-%m-%dT%H:%M:%S.%L%Z";
        const parsed = utcParse(spec, "2026-11-26T15:04:05.678+0000");
        expect_eq(parsed != null, true);

        if (ts := parsed) {
            const n = utcFormat(spec, ts, buf[0..]);
            expect_eq(strEq(buf[0..n], "2026-11-26T15:04:05.678+0000"), true);
        }
    });

    test("2. @cap/d3-time-format — Unix epoch & pre-1970 negative timestamps", () => {
        let buf: [64]u8 = undefined;

        // 1970-01-01T00:00:00.000Z (Thursday, w=4, j=001)
        const n0 = utcFormat("%Y-%m-%d %H:%M:%S %a %w %j", 0, buf[0..]);
        expect_eq(strEq(buf[0..n0], "1970-01-01 00:00:00 Thu 4 001"), true);

        // 1969-12-31T23:59:59.500Z (-500 ms)
        const n1 = utcFormat("%Y-%m-%dT%H:%M:%S.%L", -500, buf[0..]);
        expect_eq(strEq(buf[0..n1], "1969-12-31T23:59:59.500"), true);

        const p1 = utcParse("%Y-%m-%dT%H:%M:%S.%L", "1969-12-31T23:59:59.500");
        if (ts := p1) {
            expect_eq(ts, -500);
        } else {
            expect_eq(true, false);
        }
    });

    test("3. @cap/d3-time-format — Month & weekday names (%b, %B, %a, %A) and literal %%", () => {
        let buf: [64]u8 = undefined;
        // 2024-02-29T12:30:00.000Z (Leap day, Thursday, day 60)
        const leap = utcParse("%Y-%m-%d %H:%M", "2024-02-29 12:30");
        expect_eq(leap != null, true);

        if (ts := leap) {
            const n = utcFormat("%A, %B %d, %Y (%a %b) j=%j 100%%", ts, buf[0..]);
            expect_eq(strEq(buf[0..n], "Thursday, February 29, 2024 (Thu Feb) j=060 100%"), true);

            // Parse back using %a, %b and %B
            const p_short = utcParse("%a, %d %b %Y %H:%M", "Thu, 29 Feb 2024 12:30");
            if (ts2 := p_short) {
                expect_eq(ts2, ts);
            } else {
                expect_eq(true, false);
            }

            const p_full = utcParse("%A, %B %d, %Y %H:%M", "Thursday, February 29, 2024 12:30");
            if (ts3 := p_full) {
                expect_eq(ts3, ts);
            } else {
                expect_eq(true, false);
            }
        }
    });

    test("4. @cap/d3-time-format — Day-of-year (%j) parsing for leap & non-leap years", () => {
        let buf: [64]u8 = undefined;

        // 2024 is leap year -> day 366 is 2024-12-31
        const p_leap = utcParse("%Y-%j", "2024-366");
        expect_eq(p_leap != null, true);
        if (ts := p_leap) {
            const n = utcFormat("%Y-%m-%d", ts, buf[0..]);
            expect_eq(strEq(buf[0..n], "2024-12-31"), true);
        }

        // 2025 is non-leap year -> day 366 is invalid
        expect_eq(utcParse("%Y-%j", "2025-366") == null, true);
    });

    test("5. @cap/d3-time-format — Timezone offset parsing (%Z with Z / +0800 / -0500)", () => {
        const p_utc = utcParse("%Y-%m-%dT%H:%M:%S%Z", "2026-05-01T10:00:00Z");
        const p_plus8 = utcParse("%Y-%m-%dT%H:%M:%S%Z", "2026-05-01T18:00:00+0800");
        const p_minus5 = utcParse("%Y-%m-%dT%H:%M:%S%Z", "2026-05-01T05:00:00-0500");

        expect_eq(p_utc != null, true);
        expect_eq(p_plus8 != null, true);
        expect_eq(p_minus5 != null, true);

        if (a := p_utc) {
            if (b := p_plus8) {
                expect_eq(a, b);
            }
            if (c := p_minus5) {
                expect_eq(a, c);
            }
        }
    });

    test("6. @cap/d3-time-format — Invalid dates & mismatched specs return null", () => {
        // Non-leap Feb 29
        expect_eq(utcParse("%Y-%m-%d", "2025-02-29") == null, true);
        // April 31
        expect_eq(utcParse("%Y-%m-%d", "2026-04-31") == null, true);
        // Month 13 / Hour 24 / Minute 60
        expect_eq(utcParse("%Y-%m-%d", "2026-13-01") == null, true);
        expect_eq(utcParse("%H:%M", "24:00") == null, true);
        expect_eq(utcParse("%H:%M", "12:60") == null, true);
        // Trailing unconsumed chars or wrong literal
        expect_eq(utcParse("%Y-%m-%d", "2026-05-01 extra") == null, true);
        expect_eq(utcParse("%Y/%m/%d", "2026-05-01") == null, true);
    });
}
