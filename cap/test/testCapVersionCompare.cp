// cap/test/testCapVersionCompare.cp
// Comprehensive unit tests for @cap/version-compare

import { test, expect_eq } from "cap:test";
import {
    VersionCmp,
    compareVersions,
    isLess,
    isEqual,
    isGreater,
    satisfies,
} from "../../packages/version-compare/src/index.cp";

fn main() {
    test("1. @cap/version-compare — Basic numeric & multi-segment comparison", () => {
        expect_eq(compareVersions("1.0.0", "1.0.0") == VersionCmp.Eq, true);
        expect_eq(compareVersions("1.2.3", "1.2.4") == VersionCmp.Lt, true);
        expect_eq(compareVersions("1.10.0", "1.2.0") == VersionCmp.Gt, true);
        expect_eq(compareVersions("2.0.0.1", "2.0.0.0") == VersionCmp.Gt, true);
        expect_eq(isEqual("1.0", "1.0.0"), true);
        expect_eq(isEqual("1.0.0.0", "1"), true);
        expect_eq(isLess("1.0", "1.0.0.1"), true);
    });

    test("2. @cap/version-compare — v/V prefix, separators & +build metadata", () => {
        expect_eq(isEqual("v1.2.3", "1.2.3"), true);
        expect_eq(isEqual("V2.4.0", "2.4"), true);
        expect_eq(isEqual("1.2_3", "1.2.3"), true);
        expect_eq(isEqual("1.2.3+build.456", "1.2.3+other"), true);
        expect_eq(isGreater("v1.12.0", "v1.9.5"), true);
    });

    test("3. @cap/version-compare — Pre-release ordering (dev < alpha < beta < rc < release)", () => {
        expect_eq(isLess("1.0.0-dev", "1.0.0-alpha"), true);
        expect_eq(isLess("1.0.0-alpha", "1.0.0-alpha.1"), true);
        expect_eq(isLess("1.0.0-alpha.1", "1.0.0-beta"), true);
        expect_eq(isLess("1.0.0-beta.2", "1.0.0-rc.1"), true);
        expect_eq(isLess("1.0.0-rc.1", "1.0.0"), true);
        expect_eq(isEqual("1.0.0-release", "1.0.0"), true);
        expect_eq(isGreater("1.0.0-post.1", "1.0.0"), true);
    });

    test("4. @cap/version-compare — Inline alpha-numeric transitions (1.0rc1, 2.0a3)", () => {
        expect_eq(isLess("1.0a1", "1.0b1"), true);
        expect_eq(isLess("1.0b2", "1.0rc1"), true);
        expect_eq(isLess("1.0rc1", "1.0rc2"), true);
        expect_eq(isLess("1.0rc2", "1.0"), true);
        expect_eq(isEqual("1.0-rc.2", "1.0rc2"), true);
    });

    test("5. @cap/version-compare — satisfies() operator evaluation", () => {
        expect_eq(satisfies("1.2.3", "<", "1.2.4"), true);
        expect_eq(satisfies("1.2.3", "<=", "1.2.3"), true);
        expect_eq(satisfies("1.2.3", "==", "v1.2.3"), true);
        expect_eq(satisfies("1.2.3", ">=", "1.2.0"), true);
        expect_eq(satisfies("1.2.3", ">", "1.2.3-rc.1"), true);
        expect_eq(satisfies("1.2.3", "!=", "1.2.4"), true);
        expect_eq(satisfies("1.2.3", "!=", "1.2.3"), false);
        expect_eq(satisfies("1.2.3", "~=", "1.2.3"), false);
    });

    test("6. @cap/version-compare — Invalid version strings return VersionCmp.Invalid", () => {
        expect_eq(compareVersions("", "1.0.0") == VersionCmp.Invalid, true);
        expect_eq(compareVersions("   ", "1.0.0") == VersionCmp.Invalid, true);
        expect_eq(compareVersions("v", "1.0.0") == VersionCmp.Invalid, true);
        expect_eq(compareVersions("alpha", "1.0.0") == VersionCmp.Invalid, true);
        expect_eq(compareVersions("1.0 @bad", "1.0.0") == VersionCmp.Invalid, true);
        expect_eq(satisfies("not-a-ver", "==", "1.0.0"), false);
    });
}
