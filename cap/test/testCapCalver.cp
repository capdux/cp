// cap/test/testCapCalver.cp
// Comprehensive unit tests for @cap/calver

import { test, expect_eq } from "cap:test";
import {
    CalverFormat,
    Calver,
    parseCalver,
    parseCalverWith,
    compareCalver,
    bumpCalver,
    formatCalver,
} from "../../packages/@cap/calver/src/index.cp";

fn main() {
    test("1. @cap/calver — Parsing 4-digit, 2-digit, day-level & pre-release modifiers", () => {
        const v1 = parseCalver("2026.11.0");
        expect_eq(v1.valid, true);
        expect_eq(v1.year, 2026 as u16);
        expect_eq(v1.month, 11 as u8);
        expect_eq(v1.day, 0 as u8);
        expect_eq(v1.micro, 0 as u32);

        const v2 = parseCalver("26.04.1");
        expect_eq(v2.valid, true);
        expect_eq(v2.year, 2026 as u16);
        expect_eq(v2.month, 4 as u8);
        expect_eq(v2.micro, 1 as u32);

        const v3 = parseCalver("2026.11.26.2");
        expect_eq(v3.valid, true);
        expect_eq(v3.day, 26 as u8);
        expect_eq(v3.micro, 2 as u32);

        const v4 = parseCalverWith("2026.11.26", CalverFormat.YyyyMmDd);
        expect_eq(v4.valid, true);
        expect_eq(v4.day, 26 as u8);
        expect_eq(v4.micro, 0 as u32);
    });

    test("2. @cap/calver — Pre-release modifiers (-alpha.1, b2, -rc.1)", () => {
        const a1 = parseCalver("2026.11.0-alpha.1");
        expect_eq(a1.valid, true);
        expect_eq(a1.modifier, "alpha");
        expect_eq(a1.modifier_num, 1 as u32);

        const b2 = parseCalver("2026.11.0b2");
        expect_eq(b2.valid, true);
        expect_eq(b2.modifier, "b");
        expect_eq(b2.modifier_num, 2 as u32);

        const rc1 = parseCalver("2026.11.0-rc.1");
        expect_eq(rc1.valid, true);
        expect_eq(rc1.modifier, "rc");
        expect_eq(rc1.modifier_num, 1 as u32);
    });

    test("3. @cap/calver — compareCalver ordering (final > rc > beta > alpha > dev)", () => {
        const dev = parseCalver("2026.11.0-dev.1");
        const alpha1 = parseCalver("2026.11.0-alpha.1");
        const beta2 = parseCalver("2026.11.0b2");
        const rc1 = parseCalver("2026.11.0-rc.1");
        const final_v = parseCalver("2026.11.0");
        const next_v = parseCalver("2026.11.1");

        expect_eq(compareCalver(dev, alpha1), -1);
        expect_eq(compareCalver(alpha1, beta2), -1);
        expect_eq(compareCalver(beta2, rc1), -1);
        expect_eq(compareCalver(rc1, final_v), -1);
        expect_eq(compareCalver(final_v, rc1), 1);
        expect_eq(compareCalver(final_v, next_v), -1);
        expect_eq(compareCalver(final_v, parseCalver("26.11.0")), 0);
    });

    test("4. @cap/calver — bumpCalver (same period micro+1 vs new period reset to 0)", () => {
        const cur = parseCalver("2026.11.2");
        const same = bumpCalver(cur, 2026, 11, 0, false);
        expect_eq(same.year, 2026 as u16);
        expect_eq(same.month, 11 as u8);
        expect_eq(same.micro, 3 as u32);

        const next_m = bumpCalver(cur, 2026, 12, 0, false);
        expect_eq(next_m.year, 2026 as u16);
        expect_eq(next_m.month, 12 as u8);
        expect_eq(next_m.micro, 0 as u32);

        // Day-level bump
        const daily = parseCalver("2026.11.26.1");
        const same_day = bumpCalver(daily, 2026, 11, 26, true);
        expect_eq(same_day.day, 26 as u8);
        expect_eq(same_day.micro, 2 as u32);

        const next_day = bumpCalver(daily, 2026, 11, 27, true);
        expect_eq(next_day.day, 27 as u8);
        expect_eq(next_day.micro, 0 as u32);
    });

    test("5. @cap/calver — formatCalver across all 5 presets", () => {
        let out: [64]u8 = undefined;
        const v = parseCalver("2026.04.1");
        expect_eq(formatCalver(v, CalverFormat.YyyyMmMicro, out[0..64]), "2026.4.1");
        expect_eq(formatCalver(v, CalverFormat.YyMmMicro, out[0..64]), "26.4.1");
        expect_eq(formatCalver(v, CalverFormat.Yyyy0mMicro, out[0..64]), "2026.04.1");

        const vd = parseCalver("2026.11.26.2");
        expect_eq(formatCalver(vd, CalverFormat.YyyyMmDd, out[0..64]), "2026.11.26");
        expect_eq(formatCalver(vd, CalverFormat.YyyyMmDdMicro, out[0..64]), "2026.11.26.2");

        const vpre = parseCalver("2026.11.0-rc.2");
        expect_eq(formatCalver(vpre, CalverFormat.YyyyMmMicro, out[0..64]), "2026.11.0-rc.2");
    });

    test("6. @cap/calver — Calendar validation & invalid inputs rejected (leap years, bounds)", () => {
        // Month out of 1..12
        expect_eq(parseCalver("2026.0.0").valid, false);
        expect_eq(parseCalver("2026.13.0").valid, false);

        // Day out of bounds (Feb 29 on non-leap year 2026 vs leap year 2024)
        expect_eq(parseCalver("2026.02.29.0").valid, false);
        expect_eq(parseCalver("2024.02.29.0").valid, true);
        expect_eq(parseCalver("2026.04.31.0").valid, false);

        // Malformed strings
        expect_eq(parseCalver("").valid, false);
        expect_eq(parseCalver("2026").valid, false);
        expect_eq(parseCalver("abc.11.0").valid, false);
    });
}
