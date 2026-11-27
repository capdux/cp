# path-to-regexp — AI Agent Quick Ref

Zero-heap Express/Router URL path template matching, parameter extraction, and reverse route compilation (clean-room port of path-to-regexp v8) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RouteParam,
    RouteMatch,
    ptrStrEq,
    ptrSegEq,
    ptrTrimSlashes,
    ptrFindSegEnd,
    ptrSkipSlashes,
    ptrWriteByte,
} from "path-to-regexp";
```

## Core Types & Constants

```cp
class RouteParam { name: string, value: string } // methods: new(...)
class RouteMatch { matched: bool, params: [16]RouteParam, param_count: usize } // methods: noMatch(...) +1
```

## API Signatures

```cp
fn ptrStrEq(a: string, b: string): bool;
fn ptrSegEq(a: string, b: string, case_sensitive: bool): bool;
fn ptrTrimSlashes(s: string): string;
fn ptrFindSegEnd(s: string, pos: usize): usize;
fn ptrSkipSlashes(s: string, pos: usize): usize;
fn ptrWriteByte(out: []u8, pos: usize, b: u8): usize;
fn ptrWriteSlice(out: []u8, pos: usize, s: string): usize;
fn ptrFindParam(params: &[]RouteParam, name: string): string?;
fn matchPath(pattern: string, path: string, case_sensitive: bool): RouteMatch;
fn compilePath(pattern: string, params: &[]RouteParam, out: []u8): usize;
fn countRouteParams(pattern: string): usize;
```

## Minimal Usage

```cp
import { ptrStrEq } from "path-to-regexp";

pub fn main() {
    let result = ptrStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"path-to-regexp"`; install via `cap add path-to-regexp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
