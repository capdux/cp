[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# curve25519-dalek

Zero-allocation constant-time RFC 7748 X25519 elliptic curve Diffie-Hellman and GF(2^255-19) field arithmetic (curve25519-dalek parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:curve25519-dalek` ([https://github.com/dalek-cryptography/curve25519-dalek](https://github.com/dalek-cryptography/curve25519-dalek))

| Metadata | Value |
| --- | --- |
| Package | `curve25519-dalek` |
| Version | `1.0.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add curve25519-dalek
```

```cp
import {
    FieldElement,
    c25Carry,
    c25Sel,
    clampScalar,
    x25519,
    x25519Basepoint,
} from "curve25519-dalek";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FieldElement` | `class` | Fields: `limbs: [16]i64` · Methods: `fromBytes, toBytes, add, sub, mul, square, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `c25Carry` | `(o: []i64)` | Executes `c25Carry` in-place. |
| `c25Sel` | `(p: []i64, q: []i64, b: i64)` | Executes `c25Sel` in-place. |
| `clampScalar` | `(k: [32]u8): [32]u8` | Invokes `clampScalar` and returns `[32]u8`. |
| `x25519` | `(scalar: [32]u8, u_coord: [32]u8): [32]u8` | Invokes `x25519` and returns `[32]u8`. |
| `x25519Basepoint` | `(scalar: [32]u8): [32]u8` | Invokes `x25519Basepoint` and returns `[32]u8`. |

## Minimal Example

```cp
import { clampScalar } from "curve25519-dalek";

pub fn main() {
    let result = clampScalar(/* k: [32]u8 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
