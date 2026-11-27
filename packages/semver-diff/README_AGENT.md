# semver-diff — AI Agent Quick Reference

Zero-allocation SemVer upgrade diff classifier (`major`/`minor`/`patch`/`pre*`/`build`) matching npm `semver-diff` v5.

## Import

```cp
import {
    DIFF_NONE, DIFF_MAJOR, DIFF_MINOR, DIFF_PATCH,
    DIFF_PREMAJOR, DIFF_PREMINOR, DIFF_PREPATCH, DIFF_PRERELEASE, DIFF_BUILD,
    SemverDiffKind,
    diffKindFromCode,
    diffKindToCode,
    semverDiffName,
    semverDiffNameOrEmpty,
    semverDiffKind,
    semverDiffEnum,
    semverDiff,
    isUpgrade,
    isPrereleaseDiff,
    compareDiffSeverity,
    isBreakingUpgrade,
} from "semver-diff";
```

## API Signatures

```cp
const DIFF_NONE: u8;       // 0
const DIFF_MAJOR: u8;      // 1
const DIFF_MINOR: u8;      // 2
const DIFF_PATCH: u8;      // 3
const DIFF_PREMAJOR: u8;   // 4
const DIFF_PREMINOR: u8;   // 5
const DIFF_PREPATCH: u8;   // 6
const DIFF_PRERELEASE: u8; // 7
const DIFF_BUILD: u8;      // 8

enum SemverDiffKind {
    None, Major, Premajor, Minor, Preminor, Patch, Prepatch, Prerelease, Build,
}

fn diffKindFromCode(code: u8): SemverDiffKind;
fn diffKindToCode(kind: SemverDiffKind): u8;
fn semverDiffName(kind: SemverDiffKind): string?;
fn semverDiffNameOrEmpty(kind: SemverDiffKind): string;
fn semverDiffKind(ver_a: string, ver_b: string): u8;
fn semverDiffEnum(ver_a: string, ver_b: string): SemverDiffKind;
fn semverDiff(ver_a: string, ver_b: string): string?;
fn isUpgrade(ver_a: string, ver_b: string): bool;
fn isPrereleaseDiff(kind: SemverDiffKind): bool;
fn compareDiffSeverity(a: SemverDiffKind, b: SemverDiffKind): i32;
fn isBreakingUpgrade(ver_a: string, ver_b: string): bool;
```

## Minimal Example

```cp
import { semverDiff, isBreakingUpgrade } from "semver-diff";

const label = semverDiff("1.2.3", "2.0.0") ?? "none"; // "major"
const breaking = isBreakingUpgrade("0.2.0", "0.3.0");  // true
```

## Gotchas & Borrow Rules

- Upgrade-only semantics: equal versions, downgrades (`ver_b < ver_a`), and invalid SemVer strings all return `null` / `.None` / `DIFF_NONE`.
- Build metadata difference (`1.0.0+a` -> `1.0.0+b`) classifies as `.Build` (`"build"`).
- All parameters are read-only `string` borrows; returned `string?` / `string` are static literals (zero heap allocation).
