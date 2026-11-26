import { test, expect_eq } from "cap:test";
import { IsoDuration, parseIsoDuration, formatIsoDuration, toApproxMillis } from "../../packages/@cap/duration-iso/src/index.cp";

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
    test("1. @cap/duration-iso — Full P1Y2M3DT4H5M6S parsing", () => {
        const d = parseIsoDuration("P1Y2M3DT4H5M6S");
        expect_eq(d.valid, true);
        expect_eq(d.negative, false);
        expect_eq(d.years, 1 as u32);
        expect_eq(d.months, 2 as u32);
        expect_eq(d.weeks, 0 as u32);
        expect_eq(d.days, 3 as u32);
        expect_eq(d.hours, 4 as u32);
        expect_eq(d.minutes, 5 as u32);
        expect_eq(d.seconds, 6 as u32);
        expect_eq(d.millis, 0 as u32);
    });

    test("2. @cap/duration-iso — Week format P3W", () => {
        const d = parseIsoDuration("P3W");
        expect_eq(d.valid, true);
        expect_eq(d.weeks, 3 as u32);
        expect_eq(toApproxMillis(d), (21 * 86400 * 1000) as i64);
    });

    test("3. @cap/duration-iso — Fractional seconds PT1.25S & negative -PT30M", () => {
        const frac = parseIsoDuration("PT1.25S");
        expect_eq(frac.valid, true);
        expect_eq(frac.seconds, 1 as u32);
        expect_eq(frac.millis, 250 as u32);
        expect_eq(toApproxMillis(frac), 1250 as i64);

        const neg = parseIsoDuration("-PT30M");
        expect_eq(neg.valid, true);
        expect_eq(neg.negative, true);
        expect_eq(neg.minutes, 30 as u32);
        expect_eq(toApproxMillis(neg), -1800000 as i64);
    });

    test("4. @cap/duration-iso — formatIsoDuration roundtrip & zero PT0S", () => {
        let buf: [64]u8 = undefined;
        const orig = parseIsoDuration("-P1Y2DT3H4.5S");
        expect_eq(orig.valid, true);
        const n1 = formatIsoDuration(orig, buf[0..64]);
        expect_eq(testEq(buf[0..n1 as usize], "-P1Y2DT3H4.5S"), true);

        const zero = IsoDuration {
            negative: false,
            years: 0,
            months: 0,
            weeks: 0,
            days: 0,
            hours: 0,
            minutes: 0,
            seconds: 0,
            millis: 0,
            valid: true,
        };
        const n0 = formatIsoDuration(zero, buf[0..64]);
        expect_eq(n0, 4 as i64);
        expect_eq(testEq(buf[0..4], "PT0S"), true);
    });

    test("5. @cap/duration-iso — toApproxMillis exact conversion", () => {
        // 1Y(365d) + 1M(30d) + 1D(1d) = 396 days; + 1H(3600s) + 2M(120s) + 3.005S
        const d = parseIsoDuration("P1Y1M1DT1H2M3.005S");
        expect_eq(d.valid, true);
        const expected: i64 = ((396 * 86400 + 3600 + 120 + 3) * 1000 + 5) as i64;
        expect_eq(toApproxMillis(d), expected);
    });

    test("6. @cap/duration-iso — Invalid inputs & undersized buffer -1", () => {
        expect_eq(parseIsoDuration("").valid, false);
        expect_eq(parseIsoDuration("P").valid, false);
        expect_eq(parseIsoDuration("PT").valid, false);
        expect_eq(parseIsoDuration("1H").valid, false);
        expect_eq(parseIsoDuration("P1DT").valid, false);
        expect_eq(parseIsoDuration("PT5S10M").valid, false);
        expect_eq(parseIsoDuration("P1Y2Y").valid, false);

        const d = parseIsoDuration("P1Y2M3DT4H5M6S");
        let tiny: [3]u8 = undefined;
        expect_eq(formatIsoDuration(d, tiny[0..3]), -1 as i64);
    });
}
