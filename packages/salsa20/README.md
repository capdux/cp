[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# salsa20

Zero-heap Salsa20/20 stream cipher (256-bit key, 64-bit nonce) verified against official Bernstein/eSTREAM vectors

| Metadata | Value |
| --- | --- |
| Package | `salsa20` |
| Version | `0.10.2` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add salsa20
```

```cp
import {
    s20Rotl,
    s20LoadLe32,
    s20StoreLe32,
    salsa20Block,
    salsa20Apply,
} from "salsa20";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `s20Rotl` | `(v: u32, shift: u5): u32` | Invokes `s20Rotl` and returns `u32`. |
| `s20LoadLe32` | `(src: &[]u8, off: usize): u32` | Invokes `s20LoadLe32` and returns `u32`. |
| `s20StoreLe32` | `(v: u32, dst: []u8, off: usize)` | Executes `s20StoreLe32` in-place. |
| `salsa20Block` | `(key: &[]u8, nonce: &[]u8, counter: u64, out: []u8)` | Executes `salsa20Block` in-place. |
| `salsa20Apply` | `(key: &[]u8, nonce: &[]u8, initial_counter: u64, input: &[]u8, out: []u8): usize` | Invokes `salsa20Apply` and returns `usize`. |

## Minimal Example

```cp
import { s20Rotl } from "salsa20";

pub fn main() {
    let result = s20Rotl(0, /* shift: u5 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
