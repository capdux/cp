[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# untrusted

Zero-heap big-endian/little-endian binary codecs & bounds-checked untrusted ByteReader/ByteWriter (inspired by Rust byteorder + untrusted)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:untrusted` ([https://github.com/briansmith/untrusted](https://github.com/briansmith/untrusted)) / `crates.io:byteorder` ([https://github.com/BurntSushi/byteorder](https://github.com/BurntSushi/byteorder))（原库名 `untrusted` → CP 包名 `untrusted`）

| Metadata | Value |
| --- | --- |
| Package | `untrusted` |
| Version | `1.0.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add untrusted
```

```cp
import {
    ByteReader,
    ByteWriter,
    readU16Be,
    readU16Le,
    readU32Be,
    readU32Le,
    readU64Be,
    readU64Le,
} from "untrusted";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ByteReader` | `class` | Fields: `data: string, pos: usize` · Methods: `init, remaining, atEnd, peekU8, readU8, readU16Be, +8 more` |
| `ByteWriter` | `class` | Fields: `out: []u8, pos: usize, overflow: bool` · Methods: `init, asSlice, writeU8, writeU16Be, writeU16Le, writeU32Be, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `readU16Be` | `(buf: string, off: usize): u16?` | Invokes `readU16Be` and returns `u16?`. |
| `readU16Le` | `(buf: string, off: usize): u16?` | Invokes `readU16Le` and returns `u16?`. |
| `readU32Be` | `(buf: string, off: usize): u32?` | Invokes `readU32Be` and returns `u32?`. |
| `readU32Le` | `(buf: string, off: usize): u32?` | Invokes `readU32Le` and returns `u32?`. |
| `readU64Be` | `(buf: string, off: usize): u64?` | Invokes `readU64Be` and returns `u64?`. |
| `readU64Le` | `(buf: string, off: usize): u64?` | Invokes `readU64Le` and returns `u64?`. |
| `writeU16Be` | `(out: []u8, off: usize, v: u16): bool` | Invokes `writeU16Be` and returns `bool`. |
| `writeU16Le` | `(out: []u8, off: usize, v: u16): bool` | Invokes `writeU16Le` and returns `bool`. |
| `writeU32Be` | `(out: []u8, off: usize, v: u32): bool` | Invokes `writeU32Be` and returns `bool`. |
| `writeU32Le` | `(out: []u8, off: usize, v: u32): bool` | Invokes `writeU32Le` and returns `bool`. |
| `writeU64Be` | `(out: []u8, off: usize, v: u64): bool` | Invokes `writeU64Be` and returns `bool`. |
| `writeU64Le` | `(out: []u8, off: usize, v: u64): bool` | Invokes `writeU64Le` and returns `bool`. |

## Minimal Example

```cp
import { readU16Be } from "untrusted";

pub fn main() {
    let result = readU16Be("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
