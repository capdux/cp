# semver@1.0.0 — AI Agent Quick Ref

- **Import**: `import { SemVer, Version, VersionReq, parseSemver, compareSemver, satisfies, gt, inc, maxSatisfying } from "semver";`
- **Provenance**: npm `semver` (v7) + crates.io `semver` (v1) unified superset | Zero-heap | License: MIT
- **Perf (ReleaseFast)**: `parseSemver` + `compareSemver` = **6.39M ops/s** (156.4 ns/op, 16-lane SIMD)
- **Types**: `src/index.d.cp` (`Version` is a type alias for `SemVer`)

## Core Types
```cp
class SemVer { // aliased as Version
    major: u32, minor: u32, patch: u32,
    pre: [32]u8, pre_len: usize, build: [32]u8, build_len: usize, valid: bool,
    fn invalid(): SemVer;
    fn make(maj: u32, min: u32, pat: u32): SemVer;
    fn parse(s: string): SemVer;
    fn preStr(): &[]u8;
    fn buildStr(): &[]u8;
    fn isPrerelease(): bool;
    fn writeTo(out: []u8): usize;
}
class VersionReq {
    raw: string, valid: bool,
    fn parse(req: string): VersionReq;
    fn matches(v: Version): bool;
    fn matchesStr(version_str: string): bool;
}
```

## Key Functions
- **Parse / Compare**: `parseSemver(s: string): SemVer`, `parseVersion(s: string): Version`, `compareSemver(a: SemVer, b: SemVer): i32`, `compareVersions(a: Version, b: Version): i32`, `compare(a: string, b: string): i32`
- **Predicates**: `valid(s: string): bool`, `isPrerelease(v: Version): bool`, `gt`/`gte`/`lt`/`lte`/`eq`/`neq(a: string, b: string): bool`
- **Extract**: `major(s: string): u32?`, `minor(s: string): u32?`, `patch(s: string): u32?`
- **Ranges**: `satisfies(version: string, range: string): bool`, `matchesVersionReq(version_str: string, req: string): bool`, `isCaretCompatible(ver: string, target: string): bool`
- **Selection**: `maxSatisfying(versions: &[]string, range: string): i32`, `minSatisfying(versions: &[]string, range: string): i32` (returns index or `-1`)
- **Mutate / Format**: `incMajor`/`incMinor`/`incPatch(v: SemVer): SemVer`, `bumpMajor`/`bumpMinor`/`bumpPatch(v: Version): Version`, `formatSemver(v: SemVer, out: []u8): usize`, `formatVersion(v: Version, out: []u8): usize`, `clean(version: string, out: []u8): usize`, `inc(version: string, release: string, out: []u8): usize`

## Minimal Example
```cp
import { Version, VersionReq, satisfies, inc } from "semver";
const v = Version.parse("1.4.2");
const ok = VersionReq.parse(">=1.2.0, <2.0.0").matches(v); // true
const s = satisfies("1.5.0", "^1.2.0 || ~2.0.0");          // true
let buf: [64]u8 = undefined;
const n = inc("1.2.3", "minor", buf[0..]);                 // buf[0..n] == "1.3.0"
```

## Gotchas
1. Always check `.valid` on `SemVer` / `Version` / `VersionReq` instead of expecting an error union.
2. `pre` and `build` inline buffers are fixed at 32 bytes each; use `preStr()` / `buildStr()` to slice.
3. `clean()`, `inc()`, `formatSemver()`, `formatVersion()` write into caller-provided `out: []u8` and return `0` on failure or buffer overflow (recommend `>= 64` bytes).
4. `maxSatisfying` / `minSatisfying` return `i32` array index (`-1` if none match), not a string.
5. `major()` / `minor()` / `patch()` return optional `u32?` (`null` on invalid input).
