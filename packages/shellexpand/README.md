[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shellexpand

Zero-allocation shell-like tilde (~) and environment variable ($VAR, ${VAR}, ${VAR:-default}) expansion (clean-room MIT implementation of crates.io shellexpand)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:shellexpand` ([https://gitlab.com/ijackson/rust-shellexpand](https://gitlab.com/ijackson/rust-shellexpand)) | 许可：`MIT OR Apache-2.0`

| Metadata | Value |
| --- | --- |
| Package | `shellexpand` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shellexpand
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EnvEntry` | `class` | Fields: `key: string, value: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sexpIsVarStart` | `(b: u8): bool` | Invokes `sexpIsVarStart` and returns `bool`. |
| `sexpIsVarChar` | `(b: u8): bool` | Invokes `sexpIsVarChar` and returns `bool`. |
| `sexpStrEq` | `(a: string, b: string): bool` | Invokes `sexpStrEq` and returns `bool`. |
| `sexpLookup` | `(vars: &[]EnvEntry, key: string): string?` | Invokes `sexpLookup` and returns `string?`. |
| `sexpAppend` | `(out: []u8, pos: usize, s: string): usize` | Invokes `sexpAppend` and returns `usize`. |
| `tildeWithHome` | `(input: string, home_dir: string, out: []u8): usize` | Invokes `tildeWithHome` and returns `usize`. |
| `envExpand` | `(input: string, vars: &[]EnvEntry, keep_unknown: bool, out: []u8): usize` | Invokes `envExpand` and returns `usize`. |
| `fullExpand` | `(input: string, home_dir: string, vars: &[]EnvEntry, out: []u8): usize` | Invokes `fullExpand` and returns `usize`. |

## Minimal Example

```cp
import { sexpStrEq } from "shellexpand";

pub fn main() {
    let result = sexpStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
