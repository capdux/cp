// Type definitions for CP

import { SemVer, parseSemver, compareSemver } from "../../semver/src/index.cp";

export const DIFF_NONE: u8;
export const DIFF_MAJOR: u8;
export const DIFF_MINOR: u8;
export const DIFF_PATCH: u8;
export const DIFF_PREMAJOR: u8;
export const DIFF_PREMINOR: u8;
export const DIFF_PREPATCH: u8;
export const DIFF_PRERELEASE: u8;
export const DIFF_BUILD: u8;

export enum SemverDiffKind {
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

export fn diffKindFromCode(code: u8): SemverDiffKind;
export fn diffKindToCode(kind: SemverDiffKind): u8;
export fn semverDiffName(kind: SemverDiffKind): string?;
export fn semverDiffNameOrEmpty(kind: SemverDiffKind): string;
export fn semverDiffKind(ver_a: string, ver_b: string): u8;
export fn semverDiffEnum(ver_a: string, ver_b: string): SemverDiffKind;
export fn semverDiff(ver_a: string, ver_b: string): string?;
export fn isUpgrade(ver_a: string, ver_b: string): bool;
export fn isPrereleaseDiff(kind: SemverDiffKind): bool;
export fn compareDiffSeverity(a: SemverDiffKind, b: SemverDiffKind): i32;
export fn isBreakingUpgrade(ver_a: string, ver_b: string): bool;
