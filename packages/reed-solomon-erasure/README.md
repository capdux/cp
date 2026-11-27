[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# reed-solomon-erasure

Zero-allocation GF(2^8) Cauchy Reed-Solomon erasure coding and FEC reconstruction engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:reed-solomon-erasure` ([https://github.com/darrenldl/reed-solomon-erasure](https://github.com/darrenldl/reed-solomon-erasure))（原库名 `reed-solomon-erasure` → CP 包名 `reed-solomon-erasure`）

| Metadata | Value |
| --- | --- |
| Package | `reed-solomon-erasure` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add reed-solomon-erasure
```

```cp
import { RsCodec, gfMul, gfPow, gfInv } from "reed-solomon-erasure";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RsCodec` | `class` | Fields: `data_count: usize, parity_count: usize, parity_matrix: [128]u8` · Methods: `init, dataShards, parityShards, totalShards, encodeByte, verifyByte, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `gfMul` | `(a: u8, b: u8): u8` | Invokes `gfMul` and returns `u8`. |
| `gfPow` | `(a: u8, n: u32): u8` | Invokes `gfPow` and returns `u8`. |
| `gfInv` | `(a: u8): u8` | Invokes `gfInv` and returns `u8`. |

## Minimal Example

```cp
import { gfMul } from "reed-solomon-erasure";

pub fn main() {
    let result = gfMul(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
