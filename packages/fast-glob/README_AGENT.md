# fast-glob — AI Agent Quick Ref

Zero-heap pure CP fast-glob pattern set, dynamic pattern detector, and path escaper (clean-room MIT implementation of npm fast-glob v3) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FastGlobOptions,
    GlobPatternSet,
    fglToLower,
    fglIsGlobMeta,
    fglNormalizeRelPath,
    fglPathDepth,
    fglHasDotSegment,
    fglLowerInto,
} from "fast-glob";
```

## Core Types & Constants

```cp
class FastGlobOptions { dot: bool, deep: usize, only_files: bool, only_directories: bool, /* +1 fields */ } // methods: defaults(...)
class GlobPatternSet { opts: FastGlobOptions, includes: [16]string, include_count: usize, ignores: [16]string, /* +1 fields */ } // methods: new(...) +3
```

## API Signatures

```cp
fn fglToLower(b: u8): u8;
fn fglIsGlobMeta(b: u8): bool;
fn fglNormalizeRelPath(raw: string): string;
fn fglPathDepth(path: string): usize;
fn fglHasDotSegment(path: string): bool;
fn fglLowerInto(src: string, dst: []u8): usize;
fn fglMatchOne(pattern: string, path: string, case_sensitive: bool): bool;
fn isDynamicPattern(pattern: string): bool;
fn escapePath(path: string, out: []u8): usize;
fn convertPathToPattern(path: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isDynamicPattern } from "fast-glob";

pub fn main() {
    let result = isDynamicPattern("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fast-glob"`; install via `cap add fast-glob`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
