# shellexpand — AI Agent Quick Ref

Zero-allocation shell-like tilde (~) and environment variable ($VAR, ${VAR}, ${VAR:-default}) expansion (clean-room MIT implementation of crates.io shellexpand) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EnvEntry,
    sexpIsVarStart,
    sexpIsVarChar,
    sexpStrEq,
    sexpLookup,
    sexpAppend,
    tildeWithHome,
    envExpand,
} from "shellexpand";
```

## Core Types & Constants

```cp
class EnvEntry { key: string, value: string }
```

## API Signatures

```cp
fn sexpIsVarStart(b: u8): bool;
fn sexpIsVarChar(b: u8): bool;
fn sexpStrEq(a: string, b: string): bool;
fn sexpLookup(vars: &[]EnvEntry, key: string): string?;
fn sexpAppend(out: []u8, pos: usize, s: string): usize;
fn tildeWithHome(input: string, home_dir: string, out: []u8): usize;
fn envExpand(input: string, vars: &[]EnvEntry, keep_unknown: bool, out: []u8): usize;
fn fullExpand(input: string, home_dir: string, vars: &[]EnvEntry, out: []u8): usize;
```

## Minimal Usage

```cp
import { sexpStrEq } from "shellexpand";

pub fn main() {
    let result = sexpStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shellexpand"`; install via `cap add shellexpand`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
