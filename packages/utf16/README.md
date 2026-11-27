[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# utf16

Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec with full surrogate pair support, lone-surrogate rejection, BOM detection, and UTF-16 unit counting.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:utf16` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:widestring` ([https://github.com/starkat99/widestring-rs](https://github.com/starkat99/widestring-rs))（原库名 `utf16` → CP 包名 `utf16`）

| Metadata | Value |
| --- | --- |
| Package | `utf16` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add utf16
```

```cp
import {
    detectBom,
    decodeUtf8Next,
    encodeUtf8Scalar,
    countUtf16Units,
    utf8ToUtf16,
    utf16ToUtf8,
    utf8ToUtf16LeBytes,
    utf16LeBytesToUtf8,
} from "utf16";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `detectBom` | `(data: &[]u8): u8` | Invokes `detectBom` and returns `u8`. |
| `decodeUtf8Next` | `(s: string, pos: usize, out_cp: &mut u32): i64` | Parses input via `decodeUtf8Next` and returns `i64`. |
| `encodeUtf8Scalar` | `(cp: u32, out: []u8, pos: usize): i64` | Serializes/formats data via `encodeUtf8Scalar` returning `i64`. |
| `countUtf16Units` | `(utf8: string): i64` | Invokes `countUtf16Units` and returns `i64`. |
| `utf8ToUtf16` | `(utf8: string, out: []u16): i64` | Invokes `utf8ToUtf16` and returns `i64`. |
| `utf16ToUtf8` | `(utf16: &[]u16, out: []u8): i64` | Invokes `utf16ToUtf8` and returns `i64`. |
| `utf8ToUtf16LeBytes` | `(utf8: string, out: []u8): i64` | Invokes `utf8ToUtf16LeBytes` and returns `i64`. |
| `utf16LeBytesToUtf8` | `(bytes: &[]u8, out: []u8): i64` | Invokes `utf16LeBytesToUtf8` and returns `i64`. |

## Minimal Example

```cp
import { decodeUtf8Next } from "utf16";

pub fn main() {
    let result = decodeUtf8Next("example", 0, /* out_cp: &mut u32 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
