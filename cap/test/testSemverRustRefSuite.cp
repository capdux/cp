// Reference-hardened test suite against dtolnay/semver (MIT/Apache-2.0) & SemVer 2.0.0
// Covers:
//   1. dtolnay/semver classic valid & invalid version strings (leading zeros, u64 overflow, empty pre/build, 01a, whitespace)
//   2. SemVer 2.0.0 §11 full precedence ladder & build metadata equality
//   3. Caret (^0.0.3 / ^0.2.3 / ^1.2.3), Tilde (~1.2.3 / ~1.2 / ~1), wildcards (* / 1.x / 1.2.*),
//      comparators (>= / <= / > / < / =), prerelease tuple isolation & u64::MAX range overflow guards
//   4. cap:simd delimiter scanner 0-diff parity against scalar reference
//   5. Deterministic PRNG Fuzz (>= 1,500 malformed version strings & range expressions)

import { test, expect_eq } from "cap:test";
import {
    Version,
    SEMVER_MAX_LEN,
    isValidSemverStrict,
    compareSemverPrecedence,
    simdFindSemverDelimiter,
    simdFindSemverDelimiterScalar,
    simdIsValidSemverAscii,
} from "../../lib/semver/version";
import { SemverRange } from "../../lib/semver/range";
import { Query, SEMVER_RANGE_MAX_LEN } from "../../lib/semver/query";
const std = @import("std");

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
    test("1. dtolnay/semver strict valid & invalid version strings (leading zeros, u64 overflow, 01a)", () => {
        // Valid SemVer 2.0.0 versions (from dtolnay/semver/tests/test_version.rs)
        expect_eq(isValidSemverStrict("0.0.0"), true);
        expect_eq(isValidSemverStrict("1.2.3"), true);
        expect_eq(isValidSemverStrict("1.2.3-alpha1"), true);
        expect_eq(isValidSemverStrict("1.2.3+build5"), true);
        expect_eq(isValidSemverStrict("1.2.3+5build"), true);
        expect_eq(isValidSemverStrict("1.2.3-alpha1+build5"), true);
        expect_eq(isValidSemverStrict("1.2.3-1.alpha1.9+build5.7.3aedf"), true);
        expect_eq(isValidSemverStrict("1.2.3-0a.alpha1.9+05build.7.3aedf"), true);
        expect_eq(isValidSemverStrict("0.4.0-beta.1+0851523"), true);
        expect_eq(isValidSemverStrict("1.1.0-beta-10"), true);
        expect_eq(isValidSemverStrict("1.0.0-0"), true);
        expect_eq(isValidSemverStrict("1.0.0-01a"), true);
        expect_eq(isValidSemverStrict("18446744073709551615.0.0"), true);

        let v_max: Version = Version.parseStrict("18446744073709551615.42.99-01a+0851523") catch unreachable;
        expect_eq(v_max.major, std.math.maxInt(u64));
        expect_eq(v_max.minor, 42 as u64);
        expect_eq(v_max.patch, 99 as u64);
        expect_eq(v_max.prerelease, "01a");
        expect_eq(v_max.build, "0851523");

        // Invalid SemVer 2.0.0 versions
        expect_eq(isValidSemverStrict(""), false);
        expect_eq(isValidSemverStrict("  "), false);
        expect_eq(isValidSemverStrict(" 1.2.3"), false);
        expect_eq(isValidSemverStrict("1.2.3 "), false);
        expect_eq(isValidSemverStrict("1"), false);
        expect_eq(isValidSemverStrict("1.2"), false);
        expect_eq(isValidSemverStrict("1.2.3.4"), false);
        expect_eq(isValidSemverStrict("v1.2.3"), false);
        expect_eq(isValidSemverStrict("01.2.3"), false);
        expect_eq(isValidSemverStrict("1.02.3"), false);
        expect_eq(isValidSemverStrict("1.2.03"), false);
        expect_eq(isValidSemverStrict("1.0.0-01"), false);
        expect_eq(isValidSemverStrict("1.0.0-alpha.01"), false);
        expect_eq(isValidSemverStrict("1.0.0-"), false);
        expect_eq(isValidSemverStrict("1.0.0+"), false);
        expect_eq(isValidSemverStrict("1.2.3++"), false);
        expect_eq(isValidSemverStrict("1.0.0-a..b"), false);
        expect_eq(isValidSemverStrict("1.0.0+a..b"), false);
        expect_eq(isValidSemverStrict("1.2.3 abc"), false);
        expect_eq(isValidSemverStrict("111111111111111111111.0.0"), false); // > u64::MAX
        expect_eq(isValidSemverStrict("18446744073709551616.0.0"), false);  // u64::MAX + 1
        expect_eq(isValidSemverStrict("1.18446744073709551616.0"), false);
        expect_eq(isValidSemverStrict("1.0.18446744073709551616"), false);
        expect_eq(isValidSemverStrict("8\x00"), false);
        expect_eq(isValidSemverStrict("1.2.3-\x80"), false);
        expect_eq(SEMVER_MAX_LEN, 256 as usize);
        expect_eq(SEMVER_RANGE_MAX_LEN, 1024 as usize);
    });

    test("2. SemVer 2.0.0 Section 11 full precedence chain & build metadata", () => {
        const ladder: [8][]const u8 = [
            "1.0.0-alpha",
            "1.0.0-alpha.1",
            "1.0.0-alpha.beta",
            "1.0.0-beta",
            "1.0.0-beta.2",
            "1.0.0-beta.11",
            "1.0.0-rc.1",
            "1.0.0",
        ];

        let i: usize = 0;
        while (i + 1 < 8) {
            const ord_lt = compareSemverPrecedence(ladder[i], ladder[i + 1]);
            expect_eq(ord_lt != null, true);
            expect_eq(ord_lt.? == .lt, true);
            const ord_gt = compareSemverPrecedence(ladder[i + 1], ladder[i]);
            expect_eq(ord_gt != null, true);
            expect_eq(ord_gt.? == .gt, true);
            i += 1;
        }

        // Numeric prerelease < alphanumeric prerelease
        expect_eq((compareSemverPrecedence("1.0.0-999", "1.0.0-0a") orelse .eq) == .lt, true);
        // Shorter prerelease < longer prerelease when prefix equal
        expect_eq((compareSemverPrecedence("1.0.0-alpha", "1.0.0-alpha.0") orelse .eq) == .lt, true);
        // Build metadata MUST NOT affect precedence
        expect_eq((compareSemverPrecedence("1.0.0+build.1", "1.0.0+build.999") orelse .lt) == .eq, true);
        expect_eq((compareSemverPrecedence("1.0.0-rc.1+sha.aaa", "1.0.0-rc.1+sha.zzz") orelse .lt) == .eq, true);
        expect_eq((compareSemverPrecedence("1.0.0+20130313144700", "1.0.0") orelse .lt) == .eq, true);
    });

    test("3. Caret, Tilde, Wildcard, Comparator & prerelease tuple isolation", () => {
        // Caret ^0.0.3 / ^0.2.3 / ^1.2.3
        expect_eq(SemverRange.satisfiesStr("0.0.3", "^0.0.3"), true);
        expect_eq(SemverRange.satisfiesStr("0.0.4", "^0.0.3"), false);
        expect_eq(SemverRange.satisfiesStr("0.2.3", "^0.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("0.2.99", "^0.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("0.3.0", "^0.2.3"), false);
        expect_eq(SemverRange.satisfiesStr("1.2.3", "^1.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("1.99.0", "^1.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("2.0.0", "^1.2.3"), false);

        // Tilde ~1.2.3 / ~1.2 / ~1
        expect_eq(SemverRange.satisfiesStr("1.2.3", "~1.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("1.2.9", "~1.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("1.3.0", "~1.2.3"), false);
        expect_eq(SemverRange.satisfiesStr("1.2.0", "~1.2"), true);
        expect_eq(SemverRange.satisfiesStr("1.3.0", "~1.2"), false);
        expect_eq(SemverRange.satisfiesStr("1.9.9", "~1"), true);
        expect_eq(SemverRange.satisfiesStr("2.0.0", "~1"), false);

        // Comparators with spaces (ensures no dangling stack buffer slices)
        expect_eq(SemverRange.satisfiesStr("1.2.3-beta.4", ">= 1.2.3-beta.2 < 2.0.0"), true);
        expect_eq(SemverRange.satisfiesStr("1.2.3-beta.1", ">= 1.2.3-beta.2 < 2.0.0"), false);
        expect_eq(SemverRange.satisfiesStr("1.2.3", "= 1.2.3"), true);
        expect_eq(SemverRange.satisfiesStr("1.2.4", "> 1.2.3 <= 1.2.4"), true);

        // Wildcards: *, 1.x, 1.2.*
        expect_eq(SemverRange.satisfiesStr("5.0.0", "*"), true);
        expect_eq(SemverRange.satisfiesStr("1.7.2", "1.x"), true);
        expect_eq(SemverRange.satisfiesStr("2.0.0", "1.x"), false);
        expect_eq(SemverRange.satisfiesStr("1.2.9", "1.2.*"), true);
        expect_eq(SemverRange.satisfiesStr("1.3.0", "1.2.*"), false);

        // Prerelease tuple isolation
        expect_eq(SemverRange.satisfiesStr("1.2.4-alpha.1", "^1.2.3-alpha.1"), false);
        expect_eq(SemverRange.satisfiesStr("1.2.3-alpha.2", "^1.2.3-alpha.1"), true);
        expect_eq(SemverRange.satisfiesStr("1.2.3-alpha.1", "^1.0.0"), false);

        // u64::MAX upper-bound overflow guard (must not panic, must reject cleanly)
        expect_eq(SemverRange.isValidRange("^18446744073709551615.0.0"), false);
        expect_eq(SemverRange.isValidRange("~0.18446744073709551615.0"), false);
        expect_eq(SemverRange.isValidRange("^0.0.18446744073709551615"), false);
    });

    test("4. cap:simd delimiter scanner 0-diff parity vs scalar", () => {
        let buf: [96]u8 = undefined;
        let len: usize = 0;
        while (len <= 96) {
            let j: usize = 0;
            while (j < len) {
                buf[j] = 48 as u8; // '0'
                j += 1;
            }
            expect_eq(optUsizeEq(simdFindSemverDelimiter(buf[0..len]), simdFindSemverDelimiterScalar(buf[0..len])), true);
            if (len > 0) {
                const pos = len - 1;
                buf[pos] = 46 as u8; // '.'
                expect_eq(optUsizeEq(simdFindSemverDelimiter(buf[0..len]), simdFindSemverDelimiterScalar(buf[0..len])), true);
                buf[pos] = 45 as u8; // '-'
                expect_eq(optUsizeEq(simdFindSemverDelimiter(buf[0..len]), simdFindSemverDelimiterScalar(buf[0..len])), true);
                buf[pos] = 43 as u8; // '+'
                expect_eq(optUsizeEq(simdFindSemverDelimiter(buf[0..len]), simdFindSemverDelimiterScalar(buf[0..len])), true);
                buf[pos] = 32 as u8; // ' '
                expect_eq(optUsizeEq(simdFindSemverDelimiter(buf[0..len]), simdFindSemverDelimiterScalar(buf[0..len])), true);
            }
            len += 1;
        }
        expect_eq(simdIsValidSemverAscii("1.2.3-alpha.1+build.456"), true);
        expect_eq(simdIsValidSemverAscii("1.2.3\x80"), false);
    });

    test("5. Deterministic PRNG Fuzz (>= 1,500 malformed version & range expressions)", () => {
        const alphabet: []const u8 = "0123456789.-+^~<>=|*xXvV alpha-beta.01\t\r\n\x00\x80";
        let seed: u64 = 0x5EED_5E11_2026_0037;
        let fuzz_buf: [128]u8 = undefined;
        let out_buf: [256]u8 = undefined;

        let iter: usize = 0;
        let strict_valid: usize = 0;
        let strict_invalid: usize = 0;
        while (iter < 1500) {
            const slen: usize = ((nextRng(&seed) % 120) + 1) as usize;
            let k: usize = 0;
            while (k < slen) {
                const idx: usize = (nextRng(&seed) % (alphabet.len as u64)) as usize;
                fuzz_buf[k] = alphabet[idx];
                k += 1;
            }
            const sample = fuzz_buf[0..slen];

            // Must never panic, OOB, or infinite-loop
            if (isValidSemverStrict(sample)) {
                strict_valid += 1;
            } else {
                strict_invalid += 1;
            }
            * = Version.parse(sample) catch Version{};
            * = Version.parsePartial(sample) catch {};
            * = SemverRange.isValidRange(sample);
            * = SemverRange.satisfiesStr("1.2.3", sample);
            * = SemverRange.satisfiesStr(sample, "^1.0.0");
            * = SemverRange.validRangeBuf(sample, &out_buf);
            * = Query.parseClause(sample) catch {};

            // SIMD vs scalar differential verification on every fuzz sample
            expect_eq(optUsizeEq(simdFindSemverDelimiter(sample), simdFindSemverDelimiterScalar(sample)), true);

            iter += 1;
        }
        expect_eq(strict_valid + strict_invalid, 1500 as usize);
        expect_eq(strict_invalid > 1400, true);
    });
}
