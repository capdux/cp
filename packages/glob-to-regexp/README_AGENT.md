# glob-to-regexp — AI Agent Quick Ref

Zero-allocation POSIX/glob wildcard to ECMAScript/PCRE regular expression converter with extended & globstar modes in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GlobOptions,
    pushByte,
    pushSlice,
    isEscapeMeta,
    globToRegex,
    globToRegexDefault,
} from "glob-to-regexp";
```

## Core Types & Constants

```cp
class GlobOptions { extended: bool, globstar: bool, anchored: bool }
```

## API Signatures

```cp
fn pushByte(dst: []u8, pos: usize, b: u8): i64;
fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
fn isEscapeMeta(b: u8): bool;
fn globToRegex(glob: &[]u8, opts: GlobOptions, dst: []u8): i64;
fn globToRegexDefault(glob: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isEscapeMeta } from "glob-to-regexp";

pub fn main() {
    let result = isEscapeMeta(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"glob-to-regexp"`; install via `cap add glob-to-regexp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
