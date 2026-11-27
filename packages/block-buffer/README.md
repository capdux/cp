[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# block-buffer

Zero-allocation fixed-size block streaming buffers (64/128-byte) and cryptographic padding primitives (Merkle-Damgard BE/LE, PKCS#7, ISO 7816, Zero) for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:block-buffer` ([https://github.com/RustCrypto/utils](https://github.com/RustCrypto/utils)) / `crates.io:crypto-common` ([https://github.com/RustCrypto/traits](https://github.com/RustCrypto/traits))

| Metadata | Value |
| --- | --- |
| Package | `block-buffer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add block-buffer
```

```cp
import { BlockBuffer64, BlockBuffer128, bbWriteBe64, bbWriteLe64 } from "block-buffer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BlockBuffer64` | `class` | Fields: `buf: [64]u8, pos: usize, total_bytes: u64` · Methods: `new, remaining, getPos, reset, feed, padMdBe64, +4 more` |
| `BlockBuffer128` | `class` | Fields: `buf: [128]u8, pos: usize, total_bytes: u64` · Methods: `new, remaining, getPos, reset, feed, padMdBe64, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bbWriteBe64` | `(out: []u8, offset: usize, val: u64)` | Executes `bbWriteBe64` in-place. |
| `bbWriteLe64` | `(out: []u8, offset: usize, val: u64)` | Executes `bbWriteLe64` in-place. |

## Minimal Example

```cp
import { bbWriteBe64 } from "block-buffer";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    bbWriteBe64(out_buf[0..256], 0, 0);
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
