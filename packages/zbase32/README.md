[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zbase32

Zero-allocation Zooko human-oriented z-base-32 encoder, decoder, and validator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:zbase32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:zbase32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `zbase32` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add zbase32
```

```cp
import {
    decodeChar,
    encodedLen,
    encode,
    decode,
    isValidZBase32,
} from "zbase32";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeChar` | `(b: u8): i32` | Parses input via `decodeChar` and returns `i32`. |
| `encodedLen` | `(byte_len: usize): usize` | Serializes/formats data via `encodedLen` returning `usize`. |
| `encode` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(input: string, out: []u8): i64` | Parses input via `decode` and returns `i64`. |
| `isValidZBase32` | `(input: string): bool` | Checks predicate `isValidZBase32` and returns `bool`. |

## Minimal Example

```cp
import { decode } from "zbase32";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
