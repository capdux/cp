// packages/semver-diff/src/index.cp
// Zero-allocation SemVer upgrade diff classifier for CP.
// Clean-room MIT pure CP implementation of npm `semver-diff` v5, reusing `semver`.

import {
    SemVer,
    parseSemver,
    compareSemver,
} from "../../semver/src/index.cp";

pub const DIFF_NONE: u8 = 0;
pub const DIFF_MAJOR: u8 = 1;
pub const DIFF_MINOR: u8 = 2;
pub const DIFF_PATCH: u8 = 3;
pub const DIFF_PREMAJOR: u8 = 4;
pub const DIFF_PREMINOR: u8 = 5;
pub const DIFF_PREPATCH: u8 = 6;
pub const DIFF_PRERELEASE: u8 = 7;
pub const DIFF_BUILD: u8 = 8;

/// Strongly-typed SemVer upgrade diff classification enum.
pub enum SemverDiffKind {
    None,
    Major,
    Premajor,
    Minor,
    Preminor,
    Patch,
    Prepatch,
    Prerelease,
    Build,
}

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

/// Converts a `DIFF_*` numeric code into a `SemverDiffKind` enum value.
pub fn diffKindFromCode(code: u8): SemverDiffKind {
    if (code == DIFF_MAJOR) { return .Major; }
    if (code == DIFF_PREMAJOR) { return .Premajor; }
    if (code == DIFF_MINOR) { return .Minor; }
    if (code == DIFF_PREMINOR) { return .Preminor; }
    if (code == DIFF_PATCH) { return .Patch; }
    if (code == DIFF_PREPATCH) { return .Prepatch; }
    if (code == DIFF_PRERELEASE) { return .Prerelease; }
    if (code == DIFF_BUILD) { return .Build; }
    return .None;
}

/// Converts a `SemverDiffKind` enum value into its `DIFF_*` numeric code.
pub fn diffKindToCode(kind: SemverDiffKind): u8 {
    return switch (kind) {
        .None => DIFF_NONE,
        .Major => DIFF_MAJOR,
        .Premajor => DIFF_PREMAJOR,
        .Minor => DIFF_MINOR,
        .Preminor => DIFF_PREMINOR,
        .Patch => DIFF_PATCH,
        .Prepatch => DIFF_PREPATCH,
        .Prerelease => DIFF_PRERELEASE,
        .Build => DIFF_BUILD,
    };
}

/// Returns the canonical npm `semver-diff` string name for `kind`, or `null` for `.None`.
pub fn semverDiffName(kind: SemverDiffKind): string? {
    return switch (kind) {
        .None => null,
        .Major => "major",
        .Premajor => "premajor",
        .Minor => "minor",
        .Preminor => "preminor",
        .Patch => "patch",
        .Prepatch => "prepatch",
        .Prerelease => "prerelease",
        .Build => "build",
    };
}

/// Returns the canonical string name for `kind`, or `""` for `.None`.
pub fn semverDiffNameOrEmpty(kind: SemverDiffKind): string {
    return semverDiffName(kind) ?? "";
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

/// Returns the strongly-typed `SemverDiffKind` enum for upgrading from `ver_a` to `ver_b`.
pub fn semverDiffEnum(ver_a: string, ver_b: string): SemverDiffKind {
    return diffKindFromCode(semverDiffKind(ver_a, ver_b));
}

/// Returns the npm `semver-diff` label (`"major"`, `"minor"`, `"patch"`, `"premajor"`,
/// `"preminor"`, `"prepatch"`, `"prerelease"`, `"build"`) or `null` when there is no upgrade.
pub fn semverDiff(ver_a: string, ver_b: string): string? {
    return semverDiffName(semverDiffEnum(ver_a, ver_b));
}

/// Returns true if `ver_b` is a valid upgrade (including build-only change) over `ver_a`.
pub fn isUpgrade(ver_a: string, ver_b: string): bool {
    return semverDiffKind(ver_a, ver_b) != DIFF_NONE;
}

/// Returns true if `kind` represents a prerelease transition (`Premajor`, `Preminor`, `Prepatch`, `Prerelease`).
pub fn isPrereleaseDiff(kind: SemverDiffKind): bool {
    return kind == .Premajor || kind == .Preminor || kind == .Prepatch || kind == .Prerelease;
}

fn diffSeverityRank(kind: SemverDiffKind): u8 {
    return switch (kind) {
        .None => 0,
        .Build => 1,
        .Prerelease => 2,
        .Prepatch => 3,
        .Patch => 4,
        .Preminor => 5,
        .Minor => 6,
        .Premajor => 7,
        .Major => 8,
    };
}

/// Compares upgrade severity of two `SemverDiffKind` values (-1, 0, or 1).
pub fn compareDiffSeverity(a: SemverDiffKind, b: SemverDiffKind): i32 {
    const ra = diffSeverityRank(a);
    const rb = diffSeverityRank(b);
    if (ra < rb) { return -1; }
    if (ra > rb) { return 1; }
    return 0;
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
