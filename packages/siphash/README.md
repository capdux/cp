[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# siphash

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed 64-bit HashDoS-resistant PRF for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:siphash` ([https://github.com/jedisct1/siphash-js](https://github.com/jedisct1/siphash-js))（原库名 `siphash` → CP 包名 `siphash`）

| Metadata | Value |
| --- | --- |
| Package | `siphash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add siphash
```

```cp
import {
    SipState,
    shU64,
    shAdd64,
    shRotl64,
    shReadLe64,
    shRound,
    shRun,
    siphash24,
} from "siphash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SipState` | `class` | Fields: `v0: u64, v1: u64, v2: u64, v3: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `shU64` | `(hi: u32, lo: u32): u64` | Invokes `shU64` and returns `u64`. |
| `shAdd64` | `(a: u64, b: u64): u64` | Invokes `shAdd64` and returns `u64`. |
| `shRotl64` | `(x: u64, bits: u32): u64` | Invokes `shRotl64` and returns `u64`. |
| `shReadLe64` | `(data: &[]u8, offset: usize): u64` | Invokes `shReadLe64` and returns `u64`. |
| `shRound` | `(st: &mut SipState)` | Executes `shRound` in-place. |
| `shRun` | `(k0: u64, k1: u64, data: &[]u8, c_rounds: usize, d_rounds: usize): u64` | Invokes `shRun` and returns `u64`. |
| `siphash24` | `(k0: u64, k1: u64, data: &[]u8): u64` | Invokes `siphash24` and returns `u64`. |
| `siphash13` | `(k0: u64, k1: u64, data: &[]u8): u64` | Invokes `siphash13` and returns `u64`. |
| `siphash24Key16` | `(key: [16]u8, data: &[]u8): u64` | Invokes `siphash24Key16` and returns `u64`. |
| `siphash13Key16` | `(key: [16]u8, data: &[]u8): u64` | Invokes `siphash13Key16` and returns `u64`. |

## Minimal Example

```cp
import { shU64 } from "siphash";

pub fn main() {
    let result = shU64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
