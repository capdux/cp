[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# number_prefix

Zero-heap SI decimal (k/M/G/T/P/E) and IEC binary (Ki/Mi/Gi/Ti/Pi/Ei) numeric prefix formatter

| Metadata | Value |
| --- | --- |
| Package | `number_prefix` |
| Version | `0.4.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add number_prefix
```

```cp
import {
    PrefixKind,
    PrefixedNumber,
    decimalPrefix,
    binaryPrefix,
    npfWriteStr,
    npfWriteU64,
    formatPrefixed,
} from "number_prefix";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `PrefixKind` | `Standalone, Kilo, Mega, Giga, Tera, Peta, Exa, Kibi ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PrefixedNumber` | `class` | Fields: `value: f64, prefix: PrefixKind, symbol: string, is_standalone: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decimalPrefix` | `(amount: f64): PrefixedNumber` | Invokes `decimalPrefix` and returns `PrefixedNumber`. |
| `binaryPrefix` | `(amount: f64): PrefixedNumber` | Invokes `binaryPrefix` and returns `PrefixedNumber`. |
| `npfWriteStr` | `(s: string, out: []u8, pos: usize): usize` | Invokes `npfWriteStr` and returns `usize`. |
| `npfWriteU64` | `(val: u64, out: []u8, pos: usize): usize` | Invokes `npfWriteU64` and returns `usize`. |
| `formatPrefixed` | `(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize` | Serializes/formats data via `formatPrefixed` returning `usize`. |

## Minimal Example

```cp
import { formatPrefixed } from "number_prefix";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatPrefixed(/* pn: PrefixedNumber */, 0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
