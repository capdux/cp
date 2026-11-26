import { test, expect_eq } from "cap:test";
import {
    parseSemver,
    compareSemver,
    satisfies,
    maxSatisfying,
    minSatisfying,
    incMajor,
    incMinor,
    incPatch,
    formatSemver,
} from "../../packages/@cap/semver-range/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/semver-range — parseSemver & SemVer 2.0.0 §11 pre-release ordering", () => {
        const v1 = parseSemver("1.0.0-alpha");
        const v2 = parseSemver("1.0.0-alpha.1");
        const v3 = parseSemver("1.0.0-beta");
        const v4 = parseSemver("1.0.0-rc.1+build.42");
        const v5 = parseSemver("v1.0.0+20261127");
        const v6 = parseSemver("=1.0.0");

        expect_eq(v1.valid && v2.valid && v3.valid && v4.valid && v5.valid && v6.valid, true);
        expect_eq(compareSemver(v1, v2) < 0, true);
        expect_eq(compareSemver(v2, v3) < 0, true);
        expect_eq(compareSemver(v3, v4) < 0, true);
        expect_eq(compareSemver(v4, v5) < 0, true);
        // Build metadata must be ignored in precedence
        expect_eq(compareSemver(v5, v6), 0);

        let buf: [64]u8 = undefined;
        const n = formatSemver(v4, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n, "1.0.0-rc.1+build.42"), true);
    });

    test("2. @cap/semver-range — Caret ranges (^1.2.3, ^0.2.3, ^0.0.3)", () => {
        expect_eq(satisfies("1.2.3", "^1.2.3"), true);
        expect_eq(satisfies("1.9.9", "^1.2.3"), true);
        expect_eq(satisfies("2.0.0", "^1.2.3"), false);
        expect_eq(satisfies("1.2.2", "^1.2.3"), false);

        expect_eq(satisfies("0.2.3", "^0.2.3"), true);
        expect_eq(satisfies("0.2.9", "^0.2.3"), true);
        expect_eq(satisfies("0.3.0", "^0.2.3"), false);

        expect_eq(satisfies("0.0.3", "^0.0.3"), true);
        expect_eq(satisfies("0.0.4", "^0.0.3"), false);
        expect_eq(satisfies("0.0.2", "^0.0.3"), false);
    });

    test("3. @cap/semver-range — Tilde (~1.2.3, ~1.2) & wildcards (1.x, 1.2.x, *)", () => {
        expect_eq(satisfies("1.2.3", "~1.2.3"), true);
        expect_eq(satisfies("1.2.9", "~1.2.3"), true);
        expect_eq(satisfies("1.3.0", "~1.2.3"), false);
        expect_eq(satisfies("1.2.0", "~1.2"), true);
        expect_eq(satisfies("1.3.0", "~1.2"), false);

        expect_eq(satisfies("1.0.0", "1.x"), true);
        expect_eq(satisfies("1.9.9", "1.x"), true);
        expect_eq(satisfies("2.0.0", "1.x"), false);
        expect_eq(satisfies("1.2.7", "1.2.x"), true);
        expect_eq(satisfies("1.3.0", "1.2.x"), false);
        expect_eq(satisfies("9.8.7", "*"), true);
    });

    test("4. @cap/semver-range — Hyphen ranges, space intersection & || union", () => {
        expect_eq(satisfies("1.0.0", "1.0.0 - 2.0.0"), true);
        expect_eq(satisfies("1.5.3", "1.0.0 - 2.0.0"), true);
        expect_eq(satisfies("2.0.0", "1.0.0 - 2.0.0"), true);
        expect_eq(satisfies("2.0.1", "1.0.0 - 2.0.0"), false);

        expect_eq(satisfies("1.5.0", ">=1.0.0 <2.0.0"), true);
        expect_eq(satisfies("2.0.0", ">=1.0.0 <2.0.0"), false);

        expect_eq(satisfies("1.4.2", "^1.0.0 || ^3.0.0"), true);
        expect_eq(satisfies("2.1.0", "^1.0.0 || ^3.0.0"), false);
        expect_eq(satisfies("3.2.1", "^1.0.0 || ^3.0.0"), true);
    });

    test("5. @cap/semver-range — maxSatisfying, minSatisfying & incMajor/incMinor/incPatch", () => {
        const list: [5]string = ["0.9.0", "1.2.0", "1.5.3", "2.0.0", "1.8.1"];
        expect_eq(maxSatisfying(list[0..5], "^1.0.0"), 4); // "1.8.1"
        expect_eq(minSatisfying(list[0..5], "^1.0.0"), 1); // "1.2.0"
        expect_eq(maxSatisfying(list[0..5], "^5.0.0"), -1);
        expect_eq(minSatisfying(list[0..5], "^5.0.0"), -1);

        const base = parseSemver("1.2.3");
        let buf: [32]u8 = undefined;
        const n1 = formatSemver(incMajor(base), buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n1, "2.0.0"), true);
        const n2 = formatSemver(incMinor(base), buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n2, "1.3.0"), true);
        const n3 = formatSemver(incPatch(base), buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n3, "1.2.4"), true);
    });

    test("6. @cap/semver-range — Invalid version strings safely rejected", () => {
        expect_eq(parseSemver("01.2.3").valid, false);
        expect_eq(parseSemver("1.02.3").valid, false);
        expect_eq(parseSemver("1.2").valid, false);
        expect_eq(parseSemver("").valid, false);
        expect_eq(parseSemver("1.2.3-01").valid, false);
        expect_eq(parseSemver("1.2.3+").valid, false);
        expect_eq(satisfies("01.2.3", "*"), false);
        expect_eq(satisfies("1.2.3", "not-a-range"), false);
    });
}
