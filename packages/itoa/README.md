[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# itoa

Fast zero-heap two-digits-per-step integer-to-ASCII formatter (clean-room CP implementation of crates.io itoa)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:itoa` ([https://github.com/dtolnay/itoa](https://github.com/dtolnay/itoa))

| Metadata | Value |
| --- | --- |
| Package | `itoa` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add itoa
```

```cp
import {
    formatU64,
    formatI64,
    formatU32,
    formatI32,
    formatUsize,
    formatHexU64,
    formatRadixU64,
} from "itoa";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `formatU64` | `(val: u64, out: []u8): usize` | Serializes/formats data via `formatU64` returning `usize`. |
| `formatI64` | `(val: i64, out: []u8): usize` | Serializes/formats data via `formatI64` returning `usize`. |
| `formatU32` | `(val: u32, out: []u8): usize` | Serializes/formats data via `formatU32` returning `usize`. |
| `formatI32` | `(val: i32, out: []u8): usize` | Serializes/formats data via `formatI32` returning `usize`. |
| `formatUsize` | `(val: usize, out: []u8): usize` | Serializes/formats data via `formatUsize` returning `usize`. |
| `formatHexU64` | `(val: u64, uppercase: bool, out: []u8): usize` | Serializes/formats data via `formatHexU64` returning `usize`. |
| `formatRadixU64` | `(val: u64, radix: u8, out: []u8): usize` | Serializes/formats data via `formatRadixU64` returning `usize`. |

## Minimal Example

```cp
import { formatU64 } from "itoa";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatU64(0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
