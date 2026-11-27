[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sha1

Zero-allocation FIPS 180-4 SHA-1 cryptographic hash streaming implementation (Rust sha1 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sha1` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `sha1` → CP 包名 `sha1`）

| Metadata | Value |
| --- | --- |
| Package | `sha1` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sha1
```

```cp
import {
    Sha1Hasher,
    sha1Add32,
    sha1Add5,
    sha1Rotl1,
    sha1Rotl5,
    sha1Rotl30,
    sha1ReadBe32,
    sha1WriteBe32,
} from "sha1";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Sha1Hasher` | `class` | Fields: `h0: u32, h1: u32, h2: u32, h3: u32, h4: u32, buf: [64]u8, +2 more` · Methods: `new, reset, compressBuf, update, updateStr, finalize, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sha1Add32` | `(a: u32, b: u32): u32` | Invokes `sha1Add32` and returns `u32`. |
| `sha1Add5` | `(a: u32, b: u32, c: u32, d: u32, e: u32): u32` | Invokes `sha1Add5` and returns `u32`. |
| `sha1Rotl1` | `(x: u32): u32` | Invokes `sha1Rotl1` and returns `u32`. |
| `sha1Rotl5` | `(x: u32): u32` | Invokes `sha1Rotl5` and returns `u32`. |
| `sha1Rotl30` | `(x: u32): u32` | Invokes `sha1Rotl30` and returns `u32`. |
| `sha1ReadBe32` | `(b: []u8, off: usize): u32` | Invokes `sha1ReadBe32` and returns `u32`. |
| `sha1WriteBe32` | `(out: []u8, off: usize, v: u32)` | Executes `sha1WriteBe32` in-place. |
| `sha1HexDigit` | `(nibble: u8): u8` | Invokes `sha1HexDigit` and returns `u8`. |
| `sha1Bytes` | `(data: []u8, out20: []u8)` | Executes `sha1Bytes` in-place. |
| `sha1Hex` | `(s: string, out40: []u8): usize` | Invokes `sha1Hex` and returns `usize`. |

## Minimal Example

```cp
import { sha1Hex } from "sha1";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha1Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
