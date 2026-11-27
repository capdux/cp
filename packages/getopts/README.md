[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# getopts

Zero-allocation POSIX/GNU command-line option parser (short flags, clustered flags, --opt=val, -- separator, required options) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:getopts` ([https://github.com/rust-lang/getopts](https://github.com/rust-lang/getopts), `MIT OR Apache-2.0`)

| Metadata | Value |
| --- | --- |
| Package | `getopts` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add getopts
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `HasArg` | `No, Yes, Maybe` |
| `enum` | `Occur` | `Optional, Req, Multi` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OptDef` | `class` | Fields: `short_name: string, long_name: string, desc: string, has_arg: HasArg, occur: Occur` · Methods: `empty` |
| `OptVal` | `class` | Fields: `opt_idx: usize, has_val: bool, val: string` · Methods: `empty` |
| `Matches` | `class` | Fields: `ok: bool, err_kind: i32, err_opt: string, defs: [32]OptDef, def_count: usize, vals: [64]OptVal, +3 more` · Methods: `optPresent, optCount, optStr, optStrs, freeCount, freeAt` |
| `Options` | `class` | Fields: `defs: [32]OptDef, def_count: usize` · Methods: `new, opt, optflag, optflagmulti, optopt, optmulti, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `goptStrEq` | `(a: string, b: string): bool` | Invokes `goptStrEq` and returns `bool`. |
| `goptFindEq` | `(s: string): usize?` | Invokes `goptFindEq` and returns `usize?`. |
| `goptFindByName` | `(defs: &[]OptDef, count: usize, name: string): usize?` | Invokes `goptFindByName` and returns `usize?`. |
| `goptFindShort` | `(defs: &[]OptDef, count: usize, ch: string): usize?` | Invokes `goptFindShort` and returns `usize?`. |
| `goptFindLong` | `(defs: &[]OptDef, count: usize, name: string): usize?` | Invokes `goptFindLong` and returns `usize?`. |
| `goptCountForIdx` | `(vals: &[]OptVal, val_count: usize, idx: usize): usize` | Invokes `goptCountForIdx` and returns `usize`. |

## Minimal Example

```cp
import { goptStrEq } from "getopts";

pub fn main() {
    let result = goptStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
