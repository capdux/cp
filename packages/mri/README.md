[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mri

Zero-allocation POSIX/GNU CLI argument parser (--flag, --key=val, --key val, -abc short clusters, -- terminator, stdin '-') in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mri` ([https://github.com/lukeed/mri](https://github.com/lukeed/mri)) / `crates.io:lexopt` ([https://github.com/blyxxyz/lexopt](https://github.com/blyxxyz/lexopt))（原库名 `mri` → CP 包名 `mri`）

| Metadata | Value |
| --- | --- |
| Package | `mri` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mri
```

```cp
import {
    ParsedArg,
    strEq,
    parseArgs,
    hasFlag,
    getOption,
    countPositionals,
} from "mri";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedArg` | `class` | Fields: `kind: u8, name: string, value: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `strEq` | `(a: string, b: string): bool` | Invokes `strEq` and returns `bool`. |
| `parseArgs` | `(argv: &[]string, out: []ParsedArg): usize` | Parses input via `parseArgs` and returns `usize`. |
| `hasFlag` | `(argv: &[]string, long_name: string, short_char: u8): bool` | Checks predicate `hasFlag` and returns `bool`. |
| `getOption` | `(argv: &[]string, long_name: string, short_char: u8): string` | Queries/retrieves data via `getOption` returning `string`. |
| `countPositionals` | `(argv: &[]string): usize` | Invokes `countPositionals` and returns `usize`. |

## Minimal Example

```cp
import { parseArgs } from "mri";

pub fn main() {
    let result = parseArgs(/* argv: &[]string */, /* out: []ParsedArg */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
