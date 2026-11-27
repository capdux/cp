# version-compare — AI Agent Quick Ref

Zero-heap flexible version string comparison (multi-segment, v-prefix, prerelease ordering, operator matching) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    VersionCmp,
    VcSeg,
    VcParsed,
    vcIsSep,
    vcTagRank,
    vcCmpLexLower,
    vcParse,
    compareVersions,
} from "version-compare";
```

## Core Types & Constants

```cp
enum VersionCmp { Lt, Eq, Gt, Invalid }
class VcSeg { isNum: bool, numVal: u64, start: usize, len: usize }
class VcParsed { segs: [32]VcSeg, count: usize, valid: bool }
```

## API Signatures

```cp
fn vcIsSep(b: u8): bool;
fn vcTagRank(s: string, start: usize, len: usize): i32;
fn vcCmpLexLower(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32;
fn vcParse(raw: string): VcParsed;
fn compareVersions(a: string, b: string): VersionCmp;
fn isLess(a: string, b: string): bool;
fn isEqual(a: string, b: string): bool;
fn isGreater(a: string, b: string): bool;
fn satisfies(ver: string, op: string, target: string): bool;
```

## Minimal Usage

```cp
import { isLess } from "version-compare";

pub fn main() {
    let result = isLess("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"version-compare"`; install via `cap add version-compare`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
