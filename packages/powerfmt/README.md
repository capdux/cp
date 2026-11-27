[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# powerfmt

Zero-heap high-performance integer formatting (alignment, padding, radix 2/8/10/16, digit grouping) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:powerfmt` ([https://github.com/jhpratt/powerfmt](https://github.com/jhpratt/powerfmt)) / `npm:format-number` ([https://github.com/comfuture/format-number](https://github.com/comfuture/format-number))

| Metadata | Value |
| --- | --- |
| Package | `powerfmt` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add powerfmt
```

```cp
import {
    IntFmtSpec,
    digitByte,
    writeByte,
    writeRepeat,
    formatMagnitude,
    formatU64,
    formatI64,
    formatDecPadded,
} from "powerfmt";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `IntFmtSpec` | `class` | Fields: `min_width: usize, pad_byte: u8, alignment: u8, radix: u8, uppercase: bool, force_sign: bool, +2 more` · Methods: `defaultSpec` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `digitByte` | `(d: u64, uppercase: bool): u8` | Invokes `digitByte` and returns `u8`. |
| `writeByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `writeByte` and returns `usize`. |
| `writeRepeat` | `(out: []u8, pos: usize, b: u8, count: usize): usize` | Invokes `writeRepeat` and returns `usize`. |
| `formatMagnitude` | `(mag: u64, sign_byte: u8, spec: IntFmtSpec, out: []u8): usize` | Serializes/formats data via `formatMagnitude` returning `usize`. |
| `formatU64` | `(val: u64, spec: IntFmtSpec, out: []u8): usize` | Serializes/formats data via `formatU64` returning `usize`. |
| `formatI64` | `(val: i64, spec: IntFmtSpec, out: []u8): usize` | Serializes/formats data via `formatI64` returning `usize`. |
| `formatDecPadded` | `(val: i64, width: usize, out: []u8): usize` | Serializes/formats data via `formatDecPadded` returning `usize`. |
| `formatHex` | `(val: u64, uppercase: bool, out: []u8): usize` | Serializes/formats data via `formatHex` returning `usize`. |
| `formatWithCommas` | `(val: i64, out: []u8): usize` | Serializes/formats data via `formatWithCommas` returning `usize`. |

## Minimal Example

```cp
import { formatU64 } from "powerfmt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatU64(0, /* spec: IntFmtSpec */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
