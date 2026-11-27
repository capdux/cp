[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# uuencode

Zero-heap POSIX/Unix uuencode and uudecode codec supporting 45-byte lines, backtick/space zero encoding, and begin/end frames.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:uuencode` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:uuencode` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `uuencode` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add uuencode
```

```cp
import {
    uuEncChar,
    uuDecVal,
    encodeLine,
    decodeLine,
    encode,
    decode,
} from "uuencode";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uuEncChar` | `(v: u8): u8` | Invokes `uuEncChar` and returns `u8`. |
| `uuDecVal` | `(c: u8): i32` | Invokes `uuDecVal` and returns `i32`. |
| `encodeLine` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `encodeLine` returning `usize`. |
| `decodeLine` | `(line: string, out: []u8): i64` | Parses input via `decodeLine` and returns `i64`. |
| `encode` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(input: string, out: []u8): i64` | Parses input via `decode` and returns `i64`. |

## Minimal Example

```cp
import { decodeLine } from "uuencode";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeLine("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
