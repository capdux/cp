import { test, expect_eq } from "cap:test";
import {
    DIFF_NONE,
    DIFF_MAJOR,
    DIFF_MINOR,
    DIFF_PATCH,
    DIFF_PREMAJOR,
    DIFF_PREMINOR,
    DIFF_PREPATCH,
    DIFF_PRERELEASE,
    DIFF_BUILD,
    semverDiffKind,
    semverDiff,
    isBreakingUpgrade,
} from "../../packages/@cap/semver-diff/src/index.cp";

fn eqStr(a: string, b: string): bool {
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
    test("1. @cap/semver-diff — classifies major, minor, and patch upgrades", () => {
        expect_eq(semverDiffKind("1.0.0", "2.0.0"), DIFF_MAJOR);
        expect_eq(eqStr(semverDiff("1.0.0", "2.0.0") ?? "", "major"), true);

        expect_eq(semverDiffKind("v1.0.0", "v1.2.0"), DIFF_MINOR);
        expect_eq(eqStr(semverDiff("v1.0.0", "v1.2.0") ?? "", "minor"), true);

        expect_eq(semverDiffKind("1.2.3", "1.2.4"), DIFF_PATCH);
        expect_eq(eqStr(semverDiff("1.2.3", "1.2.4") ?? "", "patch"), true);
    });

    test("2. @cap/semver-diff — classifies premajor, preminor, prepatch, and prerelease upgrades", () => {
        expect_eq(semverDiffKind("1.0.0", "2.0.0-alpha.1"), DIFF_PREMAJOR);
        expect_eq(eqStr(semverDiff("1.0.0", "2.0.0-alpha.1") ?? "", "premajor"), true);

        expect_eq(semverDiffKind("1.0.0", "1.1.0-beta.1"), DIFF_PREMINOR);
        expect_eq(eqStr(semverDiff("1.0.0", "1.1.0-beta.1") ?? "", "preminor"), true);

        expect_eq(semverDiffKind("1.0.0", "1.0.1-rc.1"), DIFF_PREPATCH);
        expect_eq(eqStr(semverDiff("1.0.0", "1.0.1-rc.1") ?? "", "prepatch"), true);

        expect_eq(semverDiffKind("1.0.0-alpha.1", "1.0.0-alpha.2"), DIFF_PRERELEASE);
        expect_eq(eqStr(semverDiff("1.0.0-alpha.1", "1.0.0-beta.1") ?? "", "prerelease"), true);
    });

    test("3. @cap/semver-diff — handles graduating from prerelease to release", () => {
        expect_eq(semverDiffKind("1.0.0-alpha.1", "1.0.0"), DIFF_MAJOR);
        expect_eq(eqStr(semverDiff("1.0.0-alpha.1", "1.0.0") ?? "", "major"), true);

        expect_eq(semverDiffKind("1.1.0-rc.1", "1.1.0"), DIFF_MINOR);
        expect_eq(eqStr(semverDiff("1.1.0-rc.1", "1.1.0") ?? "", "minor"), true);

        expect_eq(semverDiffKind("1.0.1-rc.1", "1.0.1"), DIFF_PATCH);
        expect_eq(eqStr(semverDiff("1.0.1-rc.1", "1.0.1") ?? "", "patch"), true);
    });

    test("4. @cap/semver-diff — detects build metadata differences on equal precedence", () => {
        expect_eq(semverDiffKind("1.0.0+build.1", "1.0.0+build.2"), DIFF_BUILD);
        expect_eq(eqStr(semverDiff("1.0.0+build.1", "1.0.0+build.2") ?? "", "build"), true);

        expect_eq(semverDiffKind("1.0.0", "1.0.0+2026"), DIFF_BUILD);
        expect_eq(semverDiffKind("1.0.0+same", "1.0.0+same"), DIFF_NONE);
        expect_eq(semverDiff("1.0.0+same", "1.0.0+same") == null, true);
    });

    test("5. @cap/semver-diff — returns DIFF_NONE and null for equal, downgrade, or invalid inputs", () => {
        expect_eq(semverDiffKind("1.2.3", "1.2.3"), DIFF_NONE);
        expect_eq(semverDiff("1.2.3", "1.2.3") == null, true);

        expect_eq(semverDiffKind("2.0.0", "1.9.9"), DIFF_NONE);
        expect_eq(semverDiff("1.0.0", "1.0.0-alpha.1") == null, true);

        expect_eq(semverDiffKind("not-semver", "1.0.0"), DIFF_NONE);
        expect_eq(semverDiff("1.0.0", "bad") == null, true);
    });

    test("6. @cap/semver-diff — isBreakingUpgrade detects major, premajor, and 0.x minor upgrades", () => {
        expect_eq(isBreakingUpgrade("1.2.3", "2.0.0"), true);
        expect_eq(isBreakingUpgrade("1.2.3", "2.0.0-alpha.1"), true);
        expect_eq(isBreakingUpgrade("0.1.0", "0.2.0"), true);
        expect_eq(isBreakingUpgrade("0.1.0", "0.2.0-rc.1"), true);

        expect_eq(isBreakingUpgrade("1.2.3", "1.3.0"), false);
        expect_eq(isBreakingUpgrade("0.1.0", "0.1.1"), false);
        expect_eq(isBreakingUpgrade("2.0.0", "1.0.0"), false);
        expect_eq(isBreakingUpgrade("invalid", "2.0.0"), false);
    });
}
