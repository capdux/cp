[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bendy

BitTorrent BEP-3 deterministic binary Bencode streaming encoder, zero-copy decoder, validator, and dict lookup.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bendy` ([https://github.com/P3KI/bendy](https://github.com/P3KI/bendy))（原库名 `bendy` → CP 包名 `bendy`）

| Metadata | Value |
| --- | --- |
| Package | `bendy` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bendy
```

```cp
import {
    BencodeIntResult,
    BencodeBytesResult,
    BencodeWriter,
    peekKind,
    decodeInt,
    decodeBytes,
    compareBytes,
    parseValueEnd,
} from "bendy";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BencodeIntResult` | `class` | Fields: `value: i64, next_pos: usize, ok: bool` |
| `BencodeBytesResult` | `class` | Fields: `start: usize, len: usize, next_pos: usize, ok: bool` |
| `BencodeWriter` | `class` | Fields: `buf: []u8, pos: usize, overflow: bool` · Methods: `init, writeByte, writeSlice, writeInt, writeBytes, beginList, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `peekKind` | `(src: &[]u8, pos: usize): u8` | Invokes `peekKind` and returns `u8`. |
| `decodeInt` | `(src: &[]u8, pos: usize): BencodeIntResult` | Parses input via `decodeInt` and returns `BencodeIntResult`. |
| `decodeBytes` | `(src: &[]u8, pos: usize): BencodeBytesResult` | Parses input via `decodeBytes` and returns `BencodeBytesResult`. |
| `compareBytes` | `(a: &[]u8, b: &[]u8): i32` | Invokes `compareBytes` and returns `i32`. |
| `parseValueEnd` | `(src: &[]u8, pos: usize, depth: usize): i64` | Parses input via `parseValueEnd` and returns `i64`. |
| `validate` | `(src: &[]u8): bool` | Validates input with `validate` returning `bool`. |
| `dictLookup` | `(src: &[]u8, key: &[]u8): BencodeBytesResult` | Invokes `dictLookup` and returns `BencodeBytesResult`. |

## Minimal Example

```cp
import { decodeInt } from "bendy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeInt(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
