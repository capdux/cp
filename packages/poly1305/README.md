[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# poly1305

Pure CP zero-allocation RFC 8439 Poly1305 one-time authenticator (Donna 26-bit limbs + constant-time verification via subtle-ct)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:poly1305` ([https://github.com/RustCrypto/universal-hashes](https://github.com/RustCrypto/universal-hashes))

| Metadata | Value |
| --- | --- |
| Package | `poly1305` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add poly1305
```

```cp
import {
    Poly1305,
    p1305ReadU32Le,
    p1305WriteU32Le,
    poly1305Mac,
    poly1305Verify,
} from "poly1305";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Poly1305` | `class` | Fields: `r0: u32, r1: u32, r2: u32, r3: u32, r4: u32, pad0: u32, +10 more` · Methods: `init, processBlock, update, finalize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `p1305ReadU32Le` | `(b: []u8, off: usize): u32` | Invokes `p1305ReadU32Le` and returns `u32`. |
| `p1305WriteU32Le` | `(out: []u8, off: usize, v: u32)` | Executes `p1305WriteU32Le` in-place. |
| `poly1305Mac` | `(key32: []u8, msg: []u8, out_tag16: []u8): bool` | Invokes `poly1305Mac` and returns `bool`. |
| `poly1305Verify` | `(key32: []u8, msg: []u8, expected_tag16: []u8): bool` | Invokes `poly1305Verify` and returns `bool`. |

## Minimal Example

```cp
import { p1305ReadU32Le } from "poly1305";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = p1305ReadU32Le(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
