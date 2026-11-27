# getopts — AI Agent Quick Ref

Zero-allocation POSIX/GNU command-line option parser (short flags, clustered flags, --opt=val, -- separator, required options) in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    HasArg,
    Occur,
    OptDef,
    OptVal,
    Matches,
    Options,
    goptStrEq,
    goptFindEq,
} from "getopts";
```

## Core Types & Constants

```cp
enum HasArg { No, Yes, Maybe }
enum Occur { Optional, Req, Multi }
class OptDef { short_name: string, long_name: string, desc: string, has_arg: HasArg, /* +1 fields */ } // methods: empty(...)
class OptVal { opt_idx: usize, has_val: bool, val: string } // methods: empty(...)
class Matches { ok: bool, err_kind: i32, err_opt: string, defs: [32]OptDef, /* +5 fields */ } // methods: optPresent(...) +5
class Options { defs: [32]OptDef, def_count: usize } // methods: new(...) +7
```

## API Signatures

```cp
fn goptStrEq(a: string, b: string): bool;
fn goptFindEq(s: string): usize?;
fn goptFindByName(defs: &[]OptDef, count: usize, name: string): usize?;
fn goptFindShort(defs: &[]OptDef, count: usize, ch: string): usize?;
fn goptFindLong(defs: &[]OptDef, count: usize, name: string): usize?;
fn goptCountForIdx(vals: &[]OptVal, val_count: usize, idx: usize): usize;
```

## Minimal Usage

```cp
import { goptStrEq } from "getopts";

pub fn main() {
    let result = goptStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"getopts"`; install via `cap add getopts`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
