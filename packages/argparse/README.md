[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# argparse

Zero-allocation Python-style CLI argument parser supporting short/long flags, counts, bundling, and required checks for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:argparse` ([https://github.com/nodeca/argparse](https://github.com/nodeca/argparse))

| Metadata | Value |
| --- | --- |
| Package | `argparse` |
| Version | `1.0.0` |
| License | `Python-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add argparse
```

```cp
import {
    ArgAction,
    ArgDef,
    ArgSlot,
    ParseResult,
    ArgParser,
    argpStrEq,
    argpStripDashes,
    argpFindEq,
} from "argparse";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ArgAction` | `Store, StoreTrue, StoreFalse, Count` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ArgDef` | `class` | Fields: `short_flag: u8, long_name: string, dest: string, action: ArgAction, required: bool, default_val: string, +1 more` |
| `ArgSlot` | `class` | Fields: `dest: string, action: ArgAction, has_str: bool, str_val: string, bool_val: bool, count_val: u32, +1 more` |
| `ParseResult` | `class` | Fields: `ok: bool, err_code: i32, slot_count: usize, slots: [32]ArgSlot, pos_count: usize, positionals: [32]string` · Methods: `getString, getBool, getCount, positionalCount, positionalAt` |
| `ArgParser` | `class` | Fields: `def_count: usize, defs: [32]ArgDef` · Methods: `init, addArgument, parseArgs` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `argpStrEq` | `(a: string, b: string): bool` | Invokes `argpStrEq` and returns `bool`. |
| `argpStripDashes` | `(s: string): string` | Invokes `argpStripDashes` and returns `string`. |
| `argpFindEq` | `(s: string): usize?` | Invokes `argpFindEq` and returns `usize?`. |
| `argpResolveDest` | `(def: ArgDef): string` | Invokes `argpResolveDest` and returns `string`. |
| `argpFindLong` | `(defs: &[]ArgDef, name: string): usize?` | Invokes `argpFindLong` and returns `usize?`. |
| `argpFindShort` | `(defs: &[]ArgDef, ch: u8): usize?` | Invokes `argpFindShort` and returns `usize?`. |

## Minimal Example

```cp
import { argpStrEq } from "argparse";

pub fn main() {
    let result = argpStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
