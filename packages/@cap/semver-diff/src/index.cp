// packages/@cap/semver-diff/src/index.cp
// Zero-allocation SemVer upgrade diff classifier for CP.
// Clean-room MIT pure CP implementation of npm `semver-diff` v5, reusing `@cap/semver-range`.

import {
    SemVer,
    parseSemver,
    compareSemver,
} from "../../semver-range/src/index.cp";

pub const DIFF_NONE: u8 = 0;
pub const DIFF_MAJOR: u8 = 1;
pub const DIFF_MINOR: u8 = 2;
pub const DIFF_PATCH: u8 = 3;
pub const DIFF_PREMAJOR: u8 = 4;
pub const DIFF_PREMINOR: u8 = 5;
pub const DIFF_PREPATCH: u8 = 6;
pub const DIFF_PRERELEASE: u8 = 7;
pub const DIFF_BUILD: u8 = 8;

fn svdfBuildEq(a: SemVer, b: SemVer): bool {
    if (a.build_len != b.build_len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.build_len) {
        if (a.build[i] != b.build[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns the upgrade diff kind constant (`DIFF_*`) from `ver_a` to `ver_b`.
/// Only returns a non-zero kind when `ver_b > ver_a` or when precedence is equal and build metadata differs.
/// Returns `DIFF_NONE` if either version is invalid, equal (including build), or downgraded.
pub fn semverDiffKind(ver_a: string, ver_b: string): u8 {
    let a: SemVer = parseSemver(ver_a);
    let b: SemVer = parseSemver(ver_b);
    if (!a.valid || !b.valid) {
        return DIFF_NONE;
    }

    let cmp: i32 = compareSemver(a, b);
    if (cmp > 0) {
        return DIFF_NONE;
    }
    if (cmp == 0) {
        if (!svdfBuildEq(a, b)) {
            return DIFF_BUILD;
        }
        return DIFF_NONE;
    }

    // Strict upgrade: b > a
    let b_has_pre: bool = b.pre_len > 0;
    if (b.major != a.major) {
        return if (b_has_pre) { DIFF_PREMAJOR } else { DIFF_MAJOR };
    }
    if (b.minor != a.minor) {
        return if (b_has_pre) { DIFF_PREMINOR } else { DIFF_MINOR };
    }
    if (b.patch != a.patch) {
        return if (b_has_pre) { DIFF_PREPATCH } else { DIFF_PATCH };
    }

    // Same major.minor.patch tuple with b > a
    if (b_has_pre) {
        return DIFF_PRERELEASE;
    }

    // Graduating from prerelease (a) to release (b) of the same tuple
    if (b.patch != 0) {
        return DIFF_PATCH;
    }
    if (b.minor != 0) {
        return DIFF_MINOR;
    }
    return DIFF_MAJOR;
}

/// Returns the npm `semver-diff` label (`"major"`, `"minor"`, `"patch"`, `"premajor"`,
/// `"preminor"`, `"prepatch"`, `"prerelease"`, `"build"`) or `null` when there is no upgrade.
pub fn semverDiff(ver_a: string, ver_b: string): string? {
    let kind: u8 = semverDiffKind(ver_a, ver_b);
    if (kind == DIFF_MAJOR) { return "major"; }
    if (kind == DIFF_MINOR) { return "minor"; }
    if (kind == DIFF_PATCH) { return "patch"; }
    if (kind == DIFF_PREMAJOR) { return "premajor"; }
    if (kind == DIFF_PREMINOR) { return "preminor"; }
    if (kind == DIFF_PREPATCH) { return "prepatch"; }
    if (kind == DIFF_PRERELEASE) { return "prerelease"; }
    if (kind == DIFF_BUILD) { return "build"; }
    return null;
}

/// Returns true if upgrading from `ver_a` to `ver_b` is considered breaking:
/// `major` / `premajor` upgrades, or `minor` / `preminor` upgrades within `0.x`.
pub fn isBreakingUpgrade(ver_a: string, ver_b: string): bool {
    let kind: u8 = semverDiffKind(ver_a, ver_b);
    if (kind == DIFF_MAJOR || kind == DIFF_PREMAJOR) {
        return true;
    }
    if (kind == DIFF_MINOR || kind == DIFF_PREMINOR) {
        let a: SemVer = parseSemver(ver_a);
        return a.valid && a.major == 0;
    }
    return false;
}
