[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# yargs-parser

Zero-allocation CLI argument parser (npm yargs-parser parity) with long/short flags, --no- negation, camelCase/kebab-case key lookup, numeric/boolean coercion, and -- rest passthrough for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yargs-parser` ([https://github.com/yargs/yargs-parser](https://github.com/yargs/yargs-parser))

| Metadata | Value |
| --- | --- |
| Package | `yargs-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add yargs-parser
```

```cp
import {
    ParsedItem,
    ParsedEntry,
    ParsedArgs,
    ypIsNegativeNumber,
    ypFormatInt,
    ypIsFlagToken,
    ypEmptyItem,
    ypEmptyEntry,
} from "yargs-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedItem` | `class` | Fields: `data: [128]u8, len: usize` |
| `ParsedEntry` | `class` | Fields: `key: [64]u8, key_len: usize, camel_key: [64]u8, camel_len: usize, kebab_key: [64]u8, kebab_len: usize, +5 more` |
| `ParsedArgs` | `class` | Fields: `entries: [32]ParsedEntry, entry_count: usize, positionals: [32]ParsedItem, pos_count: usize, rest: [16]ParsedItem, rest_count: usize` · Methods: `findIndex, has, getBool, getInt, getString, entryCount, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ypIsNegativeNumber` | `(s: string): bool` | Invokes `ypIsNegativeNumber` and returns `bool`. |
| `ypFormatInt` | `(valIn: i64, out: []u8): i64` | Invokes `ypFormatInt` and returns `i64`. |
| `ypIsFlagToken` | `(s: string): bool` | Invokes `ypIsFlagToken` and returns `bool`. |
| `ypEmptyItem` | `(): ParsedItem` | Invokes `ypEmptyItem` and returns `ParsedItem`. |
| `ypEmptyEntry` | `(): ParsedEntry` | Invokes `ypEmptyEntry` and returns `ParsedEntry`. |
| `ypEntryMatchesKey` | `(entry: ParsedEntry, query: string): bool` | Invokes `ypEntryMatchesKey` and returns `bool`. |
| `ypPushPositional` | `(args: &mut ParsedArgs, s: string)` | Executes `ypPushPositional` in-place. |
| `ypPushRest` | `(args: &mut ParsedArgs, s: string)` | Executes `ypPushRest` in-place. |
| `ypSetEntry` | `(args: &mut ParsedArgs, rawKey: string, kind: u8, boolVal: bool, intVal: i64, strVal: string)` | Executes `ypSetEntry` in-place. |
| `ypSetCoerced` | `(args: &mut ParsedArgs, rawKey: string, rawVal: string)` | Executes `ypSetCoerced` in-place. |
| `parseArgs` | `(argv: &[]string): ParsedArgs` | Parses input via `parseArgs` and returns `ParsedArgs`. |

## Minimal Example

```cp
import { parseArgs } from "yargs-parser";

pub fn main() {
    let result = parseArgs(/* argv: &[]string */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
