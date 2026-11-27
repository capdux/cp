# pkg-types — AI Agent Quick Ref

Zero-heap package.json and cap.json manifest parser and package name validator (`v1.3.1`, `MIT`)

## Import

```cp
import {
    PkgManifest,
    PkgNameParts,
    pkgtIsSpace,
    pkgtSkipSpace,
    pkgtStrEq,
    pkgtScanStringEnd,
    pkgtSkipValue,
    pkgtEmptyManifest,
} from "pkg-types";
```

## Core Types & Constants

```cp
class PkgManifest { name: string, version: string, license: string, main: string, /* +4 fields */ }
class PkgNameParts { scope: string, pkg_name: string, has_scope: bool }
```

## API Signatures

```cp
fn pkgtIsSpace(c: u8): bool;
fn pkgtSkipSpace(s: string, start: usize): usize;
fn pkgtStrEq(a: string, b: string): bool;
fn pkgtScanStringEnd(s: string, open_quote: usize): usize;
fn pkgtSkipValue(s: string, start: usize): usize;
fn pkgtEmptyManifest(): PkgManifest;
fn parsePkgManifest(json_text: string): PkgManifest;
fn pkgtIsValidSegmentChar(c: u8): bool;
fn pkgtIsValidSegment(seg: string): bool;
fn isValidPkgName(name: string): bool;
fn splitPkgScope(name: string): PkgNameParts;
```

## Minimal Usage

```cp
import { parsePkgManifest } from "pkg-types";

pub fn main() {
    let result = parsePkgManifest("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pkg-types"`; install via `cap add pkg-types`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
