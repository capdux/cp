[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# semver-diff

Zero-allocation SemVer upgrade diff classifier (`major`, `minor`, `patch`, `premajor`, `preminor`, `prepatch`, `prerelease`, `build`) and breaking-upgrade detector for CP, matching npm [`semver-diff`](https://github.com/sindresorhus/semver-diff) v5.

## Source Alignment

- **Upstream**: npm `semver-diff` v5 (`sindresorhus/semver-diff`, MIT)
- **CP Package**: `semver-diff` v1.0.0 (clean-room MIT in pure CP, built on top of `semver`)
- **Design**: Zero heap allocations; returns either a strongly-typed `SemverDiffKind` enum, a `u8` `DIFF_*` code, or an optional static `string?` (`null` on non-upgrade or invalid input).

## Installation & Import

```bash
cap add semver-diff
```

```cp
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

## Core API

### Constants & Enum

| Symbol | Values | Description |
| :--- | :--- | :--- |
| `DIFF_NONE` .. `DIFF_BUILD` | `0` (`NONE`), `1` (`MAJOR`), `2` (`MINOR`), `3` (`PATCH`), `4` (`PREMAJOR`), `5` (`PREMINOR`), `6` (`PREPATCH`), `7` (`PRERELEASE`), `8` (`BUILD`) | Compact `u8` diff kind codes |
| `SemverDiffKind` | `None \| Major \| Premajor \| Minor \| Preminor \| Patch \| Prepatch \| Prerelease \| Build` | Strongly-typed SemVer upgrade diff enum |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `semverDiff` | `fn semverDiff(ver_a: string, ver_b: string): string?` | Returns canonical npm diff name (`"major"`, `"preminor"`, `"build"`, etc.) or `null` if not an upgrade / invalid |
| `semverDiffEnum` | `fn semverDiffEnum(ver_a: string, ver_b: string): SemverDiffKind` | Returns strongly-typed `SemverDiffKind` (`.None` if equal, downgrade, or invalid) |
| `semverDiffKind` | `fn semverDiffKind(ver_a: string, ver_b: string): u8` | Returns compact `DIFF_*` code (`DIFF_NONE` when not an upgrade) |
| `semverDiffName` | `fn semverDiffName(kind: SemverDiffKind): string?` | Converts `SemverDiffKind` to static string name, or `null` for `.None` |
| `semverDiffNameOrEmpty` | `fn semverDiffNameOrEmpty(kind: SemverDiffKind): string` | Converts `SemverDiffKind` to static string name, or `""` for `.None` |
| `diffKindFromCode` | `fn diffKindFromCode(code: u8): SemverDiffKind` | Converts `DIFF_*` `u8` code to `SemverDiffKind` |
| `diffKindToCode` | `fn diffKindToCode(kind: SemverDiffKind): u8` | Converts `SemverDiffKind` to `DIFF_*` `u8` code |
| `isUpgrade` | `fn isUpgrade(ver_a: string, ver_b: string): bool` | Returns `true` iff `semverDiffKind(ver_a, ver_b) != DIFF_NONE` |
| `isBreakingUpgrade` | `fn isBreakingUpgrade(ver_a: string, ver_b: string): bool` | Returns `true` for breaking upgrades (major/premajor, or 0.x minor/preminor bump) |
| `isPrereleaseDiff` | `fn isPrereleaseDiff(kind: SemverDiffKind): bool` | Returns `true` for `.Premajor`, `.Preminor`, `.Prepatch`, or `.Prerelease` |
| `compareDiffSeverity` | `fn compareDiffSeverity(a: SemverDiffKind, b: SemverDiffKind): i32` | Compares upgrade severity (`-1`, `0`, `1`; `Major` highest, `None` lowest) |

## Examples

### 1. Basic Upgrade Classification

```cp
import { semverDiff } from "semver-diff";
import { print } from "cap:io";

pub fn main() {
    if (const diff = semverDiff("1.2.3", "2.0.0")) {
        print(diff); // "major"
    }
    if (const pre = semverDiff("1.2.3", "1.3.0-alpha.1")) {
        print(pre);  // "preminor"
    }
    const down = semverDiff("2.0.0", "1.9.9"); // null (downgrade)
}
```

### 2. Exhaustive Enum Matching & Breaking-Change Gate

```cp
import { SemverDiffKind, semverDiffEnum, isBreakingUpgrade, isPrereleaseDiff } from "semver-diff";

pub fn shouldAutoUpdate(current: string, latest: string): bool {
    if (isBreakingUpgrade(current, latest)) {
        return false;
    }
    const kind = semverDiffEnum(current, latest);
    if (isPrereleaseDiff(kind)) {
        return false;
    }
    return kind == .Minor || kind == .Patch;
}
```

### 3. Comparing Severity Across Dependencies

```cp
import { semverDiffEnum, compareDiffSeverity } from "semver-diff";

pub fn isMoreSevere(a1: string, b1: string, a2: string, b2: string): bool {
    const k1 = semverDiffEnum(a1, b1);
    const k2 = semverDiffEnum(a2, b2);
    return compareDiffSeverity(k1, k2) > 0;
}
```

## Differences from npm `semver-diff` & Safety Highlights

- **No Exceptions on Invalid Input**: Invalid version strings return `.None` / `DIFF_NONE` / `null` instead of throwing.
- **Zero-Allocation Static Strings**: `semverDiff` and `semverDiffName` return static string literals with `'static` lifetime — no heap allocation or buffer management needed.
- **Extended Helpers**: Adds `SemverDiffKind`, `isUpgrade`, `isBreakingUpgrade`, `isPrereleaseDiff`, and `compareDiffSeverity` for CLI updaters and dependency auditors.
