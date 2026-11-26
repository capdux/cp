// cap/test/testCapSemverRange.cp
// Comprehensive unit tests for @cap/semver-range

import { test, expect_eq } from "cap:test";
import {
    parseVersion,
    compareVersion,
    satisfies,
    isValidRange,
    maxSatisfyingIndex,
} from "../../packages/@cap/semver-range/src/index.cp";

fn main() {
    test("1. @cap/semver-range — Caret ^1.2.3, ^0.2.3 & ^0.0.3 exact semantics", () => {
        expect_eq(satisfies("1.2.3", "^1.2.3"), true);
        expect_eq(satisfies("1.9.0", "^1.2.3"), true);
        expect_eq(satisfies("1.2.2", "^1.2.3"), false);
        expect_eq(satisfies("2.0.0", "^1.2.3"), false);

        // ^0.2.3 := >=0.2.3 <0.3.0
        expect_eq(satisfies("0.2.3", "^0.2.3"), true);
        expect_eq(satisfies("0.2.9", "^0.2.3"), true);
        expect_eq(satisfies("0.3.0", "^0.2.3"), false);

        // ^0.0.3 := >=0.0.3 <0.0.4
        expect_eq(satisfies("0.0.3", "^0.0.3"), true);
        expect_eq(satisfies("0.0.4", "^0.0.3"), false);
        expect_eq(satisfies("0.0.2", "^0.0.3"), false);
    });

    test("2. @cap/semver-range — Tilde ~1.2.3 & wildcards 1.2.x / 1.* / *", () => {
        expect_eq(satisfies("1.2.3", "~1.2.3"), true);
        expect_eq(satisfies("1.2.9", "~1.2.3"), true);
        expect_eq(satisfies("1.3.0", "~1.2.3"), false);
        expect_eq(satisfies("1.2.2", "~1.2.3"), false);

        expect_eq(satisfies("1.2.0", "1.2.x"), true);
        expect_eq(satisfies("1.2.99", "1.2.X"), true);
        expect_eq(satisfies("1.3.0", "1.2.x"), false);
        expect_eq(satisfies("1.9.5", "1.*"), true);
        expect_eq(satisfies("2.0.0", "1.*"), false);
        expect_eq(satisfies("99.0.1", "*"), true);
    });

    test("3. @cap/semver-range — Compound AND (>=1.2.0 <2.0.0) & OR (||) branches", () => {
        expect_eq(satisfies("1.2.0", ">=1.2.0 <2.0.0"), true);
        expect_eq(satisfies("1.9.9", ">=1.2.0, <2.0.0"), true);
        expect_eq(satisfies("1.1.9", ">=1.2.0 <2.0.0"), false);
        expect_eq(satisfies("2.0.0", ">=1.2.0 <2.0.0"), false);

        const multi = "^1.0.0 || >=3.1.0 <=3.2.5";
        expect_eq(satisfies("1.5.0", multi), true);
        expect_eq(satisfies("2.0.0", multi), false);
        expect_eq(satisfies("3.2.5", multi), true);
        expect_eq(satisfies("3.2.6", multi), false);
    });

    test("4. @cap/semver-range — maxSatisfyingIndex picks highest matching version", () => {
        let vers: [5]string = undefined;
        vers[0] = "1.0.0";
        vers[1] = "1.4.2";
        vers[2] = "2.0.0";
        vers[3] = "1.9.1";
        vers[4] = "1.9.0";

        expect_eq(maxSatisfyingIndex(vers[0..5], "^1.2.0"), 3 as i64);
        expect_eq(maxSatisfyingIndex(vers[0..5], ">=2.0.0"), 2 as i64);
        expect_eq(maxSatisfyingIndex(vers[0..5], "^3.0.0"), -1 as i64);
    });

    test("5. @cap/semver-range — isValidRange & malformed version/range rejection", () => {
        expect_eq(isValidRange("^1.2.3"), true);
        expect_eq(isValidRange(">=1.0.0 <2.0.0 || ~3.1.0"), true);
        expect_eq(isValidRange("1.2.x"), true);
        expect_eq(isValidRange("*"), true);

        expect_eq(isValidRange(""), false);
        expect_eq(isValidRange("^1.2.3 || "), false);
        expect_eq(isValidRange("1.0.0 | 2.0.0"), false);
        expect_eq(isValidRange(">="), false);
        expect_eq(isValidRange("abc"), false);
        expect_eq(satisfies("not-a-ver", "^1.0.0"), false);
        expect_eq(satisfies("1.0.0", "bad-range"), false);
    });

    test("6. @cap/semver-range — Leading 'v', +build metadata & compareVersion", () => {
        const a = parseVersion("v1.2.3+build.42");
        const b = parseVersion("1.2.4");
        const c = parseVersion("1.2.3");

        expect_eq(a.valid, true);
        expect_eq(a.major, 1 as u32);
        expect_eq(a.minor, 2 as u32);
        expect_eq(a.patch, 3 as u32);
        expect_eq(compareVersion(a, c), 0 as i32);
        expect_eq(compareVersion(a, b), -1 as i32);
        expect_eq(compareVersion(b, a), 1 as i32);
        expect_eq(satisfies("v1.2.3+sha.abc", "^v1.0.0"), true);
    });
}
