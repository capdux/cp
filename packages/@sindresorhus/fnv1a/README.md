[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @sindresorhus/fnv1a

Zero-heap FNV-1 and FNV-1a 32-bit and 64-bit streaming non-cryptographic hash functions for CP (compatible with Rust fnv & npm @sindresorhus/fnv1a)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@sindresorhus/fnv1a` ([https://github.com/sindresorhus/fnv1a](https://github.com/sindresorhus/fnv1a)) / `crates.io:const-fnv1a-hash` ([https://github.com/jonasbb/const-fnv1a-hash](https://github.com/jonasbb/const-fnv1a-hash))（原库名 `@sindresorhus/fnv1a` → CP 包名 `@sindresorhus/fnv1a`）

| Metadata | Value |
| --- | --- |
| Package | `@sindresorhus/fnv1a` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @sindresorhus/fnv1a
```

```cp
import {
    fnv64OffsetBasis,
    fnv64Prime,
    mulWrap32,
    mulFnv64Prime,
    fnv1a32Update,
    fnv1a32,
    fnv132,
    fnv1a64Update,
} from "@sindresorhus/fnv1a";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fnv64OffsetBasis` | `(): u64` | Invokes `fnv64OffsetBasis` and returns `u64`. |
| `fnv64Prime` | `(): u64` | Invokes `fnv64Prime` and returns `u64`. |
| `mulWrap32` | `(a: u32, b: u32): u32` | Invokes `mulWrap32` and returns `u32`. |
| `mulFnv64Prime` | `(h: u64): u64` | Invokes `mulFnv64Prime` and returns `u64`. |
| `fnv1a32Update` | `(state: u32, data: &[]u8): u32` | Invokes `fnv1a32Update` and returns `u32`. |
| `fnv1a32` | `(data: &[]u8): u32` | Invokes `fnv1a32` and returns `u32`. |
| `fnv132` | `(data: &[]u8): u32` | Invokes `fnv132` and returns `u32`. |
| `fnv1a64Update` | `(state: u64, data: &[]u8): u64` | Invokes `fnv1a64Update` and returns `u64`. |
| `fnv1a64` | `(data: &[]u8): u64` | Invokes `fnv1a64` and returns `u64`. |
| `fnv164` | `(data: &[]u8): u64` | Invokes `fnv164` and returns `u64`. |
| `formatHex64` | `(h: u64, out: []u8): usize` | Serializes/formats data via `formatHex64` returning `usize`. |

## Minimal Example

```cp
import { formatHex64 } from "@sindresorhus/fnv1a";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatHex64(0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
