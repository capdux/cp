[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tar

Zero-heap POSIX USTAR 512-byte tar archive header codec and streaming cursor (crates.io tar equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tar` ([https://github.com/alexcrichton/tar-rs](https://github.com/alexcrichton/tar-rs))（原库名 `tar` → CP 包名 `tar`）

| Metadata | Value |
| --- | --- |
| Package | `tar` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tar
```

```cp
import {
    TarHeader,
    TarEntrySpan,
    TarArchiveCursor,
    tarEmptyHeader,
    tarWriteOctal,
    tarParseOctal,
    tarComputeChecksum,
    tarIsUstarMagic,
} from "tar";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TarHeader` | `class` | Fields: `name: string, mode: u32, uid: u32, gid: u32, size: u64, mtime: u64, +4 more` |
| `TarEntrySpan` | `class` | Fields: `header: TarHeader, data_offset: usize, data_len: usize` |
| `TarArchiveCursor` | `class` | Fields: `offset: usize, finished: bool` · Methods: `init, reset, nextEntry` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tarEmptyHeader` | `(): TarHeader` | Invokes `tarEmptyHeader` and returns `TarHeader`. |
| `tarWriteOctal` | `(out: []u8, offset: usize, digits: usize, val: u64): bool` | Invokes `tarWriteOctal` and returns `bool`. |
| `tarParseOctal` | `(buf: []u8, offset: usize, width: usize, require_digit: bool): u64?` | Invokes `tarParseOctal` and returns `u64?`. |
| `tarComputeChecksum` | `(block512: []u8): u32` | Invokes `tarComputeChecksum` and returns `u32`. |
| `tarIsUstarMagic` | `(block512: []u8): bool` | Invokes `tarIsUstarMagic` and returns `bool`. |
| `isZeroBlock` | `(block512: []u8): bool` | Checks predicate `isZeroBlock` and returns `bool`. |
| `tarPaddedSize` | `(raw_size: u64): u64` | Invokes `tarPaddedSize` and returns `u64`. |
| `encodeTarHeader` | `(name: string, size: u64, mode: u32, mtime: u64, typeflag: u8, out512: []u8): bool` | Serializes/formats data via `encodeTarHeader` returning `bool`. |
| `parseTarHeader` | `(block512: []u8): TarHeader` | Parses input via `parseTarHeader` and returns `TarHeader`. |

## Minimal Example

```cp
import { encodeTarHeader } from "tar";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeTarHeader("example", 0, 0, 0, 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
