[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# universal-hash

Zero-allocation NIST SP 800-38D GF(2^128) universal hash (GHASH for AES-GCM) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:universal-hash` ([https://github.com/RustCrypto/universal-hashes](https://github.com/RustCrypto/universal-hashes))（原库名 `universal-hash` → CP 包名 `universal-hash`）

| Metadata | Value |
| --- | --- |
| Package | `universal-hash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add universal-hash
```

```cp
import {
    Block128,
    Ghash,
    bitToMask64,
    readBeU64,
    writeBeU64,
    gf128Mul,
} from "universal-hash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Block128` | `class` | Fields: `hi: u64, lo: u64` · Methods: `zero, one, fromBytes, toBytes, xor, eq` |
| `Ghash` | `class` | Fields: `h: Block128, y: Block128` · Methods: `new, reset, updateBlock, updatePadded, updateLengths, finalize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bitToMask64` | `(bit: u64): u64` | Invokes `bitToMask64` and returns `u64`. |
| `readBeU64` | `(buf: &[]u8, off: usize): u64` | Invokes `readBeU64` and returns `u64`. |
| `writeBeU64` | `(val: u64, out: []u8, off: usize)` | Executes `writeBeU64` in-place. |
| `gf128Mul` | `(x: Block128, y: Block128): Block128` | Invokes `gf128Mul` and returns `Block128`. |

## Minimal Example

```cp
import { bitToMask64 } from "universal-hash";

pub fn main() {
    let result = bitToMask64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
