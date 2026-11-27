[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# humansize

Zero-heap human-readable byte size formatter (Decimal, Binary, Windows) in pure CP (modeled after Rust humansize v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:humansize` ([https://github.com/LeopoldArkham/humansize](https://github.com/LeopoldArkham/humansize))（原库名 `humansize` → CP 包名 `humansize`）

| Metadata | Value |
| --- | --- |
| Package | `humansize` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add humansize
```

```cp
import {
    BaseUnit,
    FormatSizeOptions,
    hszWriteSlice,
    hszWriteU64,
    hszWriteFixedFrac,
    hszPow10,
    hszUnitStr,
    formatSize,
} from "humansize";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `BaseUnit` | `Decimal, Binary, Windows` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FormatSizeOptions` | `class` | Fields: `base: BaseUnit, decimal_places: u8, space_after_value: bool, long_units: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hszWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `hszWriteSlice` and returns `usize`. |
| `hszWriteU64` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `hszWriteU64` and returns `usize`. |
| `hszWriteFixedFrac` | `(out: []u8, pos: usize, frac: u64, places: u8): usize` | Invokes `hszWriteFixedFrac` and returns `usize`. |
| `hszPow10` | `(places: u8): u64` | Invokes `hszPow10` and returns `u64`. |
| `hszUnitStr` | `(base: BaseUnit, idx: usize, long_units: bool, is_one: bool): string` | Invokes `hszUnitStr` and returns `string`. |
| `formatSize` | `(bytes: u64, opts: FormatSizeOptions, out: []u8): usize` | Serializes/formats data via `formatSize` returning `usize`. |
| `formatDecimal` | `(bytes: u64, out: []u8): usize` | Serializes/formats data via `formatDecimal` returning `usize`. |
| `formatBinary` | `(bytes: u64, out: []u8): usize` | Serializes/formats data via `formatBinary` returning `usize`. |

## Minimal Example

```cp
import { hszWriteSlice } from "humansize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hszWriteSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
