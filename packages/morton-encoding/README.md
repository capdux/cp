[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# morton-encoding

Zero-allocation 2D/3D Z-Order (Morton) bit-interleaving codec and 2D Hilbert space-filling curve mapper for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:morton-encoding` ([https://github.com/morton-encoding/morton-encoding](https://github.com/morton-encoding/morton-encoding))（原库名 `morton-encoding` → CP 包名 `morton-encoding`）

| Metadata | Value |
| --- | --- |
| Package | `morton-encoding` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add morton-encoding
```

```cp
import {
    Point2D,
    Point3D,
    splitBy1,
    compactBy1,
    encode2D,
    decode2D,
    splitBy2,
    compactBy2,
} from "morton-encoding";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Point2D` | `class` | Fields: `x: u32, y: u32` |
| `Point3D` | `class` | Fields: `x: u32, y: u32, z: u32` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `splitBy1` | `(v: u32): u64` | Invokes `splitBy1` and returns `u64`. |
| `compactBy1` | `(v: u64): u32` | Invokes `compactBy1` and returns `u32`. |
| `encode2D` | `(x: u32, y: u32): u64` | Serializes/formats data via `encode2D` returning `u64`. |
| `decode2D` | `(code: u64): Point2D` | Parses input via `decode2D` and returns `Point2D`. |
| `splitBy2` | `(v: u32): u64` | Invokes `splitBy2` and returns `u64`. |
| `compactBy2` | `(v: u64): u32` | Invokes `compactBy2` and returns `u32`. |
| `encode3D` | `(x: u32, y: u32, z: u32): u64` | Serializes/formats data via `encode3D` returning `u64`. |
| `decode3D` | `(code: u64): Point3D` | Parses input via `decode3D` and returns `Point3D`. |
| `clampOrder` | `(order: u32): u32` | Invokes `clampOrder` and returns `u32`. |
| `hilbertRot` | `(n: u32, x_in: u32, y_in: u32, rx: u32, ry: u32): Point2D` | Invokes `hilbertRot` and returns `Point2D`. |
| `hilbert2DEncode` | `(order: u32, x_in: u32, y_in: u32): u64` | Invokes `hilbert2DEncode` and returns `u64`. |
| `hilbert2DDecode` | `(order: u32, d_in: u64): Point2D` | Invokes `hilbert2DDecode` and returns `Point2D`. |

## Minimal Example

```cp
import { encode2D } from "morton-encoding";

pub fn main() {
    let result = encode2D(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
