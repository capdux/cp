# semver — AI Agent Quick Ref

Unified zero-heap SemVer 2.0.0 parser, comparator, npm range evaluator, and Rust Cargo VersionReq matcher for CP (superset of npm semver & crates.io semver) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SemVer,
    SvrPartial,
    SvrCompResult,
    VersionReq,
    svrIsIdentChar,
    svrIsWildcardStr,
    svrParseU32Strict,
    svrIsAllDigits,
} from "semver";
```

## Core Types & Constants

```cp
class SemVer { major: u32, minor: u32, patch: u32, pre: [32]u8, /* +4 fields */ } // methods: invalid(...) +6
class SvrPartial { major: u32, minor: u32, patch: u32, has_major: bool, /* +5 fields */ }
class SvrCompResult { matched: bool, allow_pre: bool }
class VersionReq { raw: string, valid: bool } // methods: parse(...) +2
```

## API Signatures

```cp
fn svrIsIdentChar(c: u8): bool;
fn svrIsWildcardStr(s: string): bool;
fn svrParseU32Strict(s: string): u32?;
fn svrIsAllDigits(s: &[]u8): bool;
fn svrValidatePre(s: string): bool;
fn svrValidateBuild(s: string): bool;
fn svrWriteByte(out: []u8, pos: usize, b: u8): usize;
fn parseSemver(s: string): SemVer;
fn svrComparePreIdent(a: &[]u8, b: &[]u8): i32;
fn compareSemver(a: SemVer, b: SemVer): i32;
fn svrParsePartial(raw: string): SvrPartial;
fn svrPartialToSemver(p: SvrPartial): SemVer;
fn svrSameTuplePre(v: SemVer, p: SvrPartial): bool;
fn svrMatchSingleComparator(v: SemVer, tok: string): SvrCompResult;
fn svrIsBareOperator(tok: string): bool;
fn svrMatchClause(v: SemVer, clause: string): bool;
fn svrSatisfiesVer(v: SemVer, range: string): bool;
fn svrValidateReqPart(part: string): bool;
fn svrMatchesVersionReq(v: SemVer, req: string): bool;
fn parseVersion(s: string): Version;
fn compareVersions(a: Version, b: Version): i32;
fn isPrerelease(v: Version): bool;
fn matchesVersionReq(version_str: string, req: string): bool;
fn isCaretCompatible(ver: string, target: string): bool;
fn formatVersion(v: Version, out: []u8): usize;
fn bumpMajor(v: Version): Version;
fn bumpMinor(v: Version): Version;
fn bumpPatch(v: Version): Version;
fn satisfies(version: string, range: string): bool;
fn valid(s: string): bool;
fn compare(a: string, b: string): i32;
fn gt(a: string, b: string): bool;
fn gte(a: string, b: string): bool;
// ... +15 more functions — see src/index.d.cp for full signatures
```

## Minimal Usage

```cp
import { parseSemver } from "semver";

pub fn main() {
    let result = parseSemver("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"semver"`; install via `cap add semver`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
