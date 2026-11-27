[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lz-string

Zero-heap LZW byte/string compression with Base64 and URI-safe encoding for CP (clean-room MIT implementation of npm lz-string).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lz-string` ([https://github.com/pieroxy/lz-string](https://github.com/pieroxy/lz-string))（原库名 `lz-string` → CP 包名 `lz-string`）

| Metadata | Value |
| --- | --- |
| Package | `lz-string` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lz-string
```

```cp
import {
    lzsHash,
    lzsClearTable,
    lzsFind,
    lzsInsert,
    compressBytes,
    decompressBytes,
    lzsEncode64,
    lzsDecodeChar,
} from "lz-string";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lzsHash` | `(key: u32): usize` | Invokes `lzsHash` and returns `usize`. |
| `lzsClearTable` | `(ht_key: []u32)` | Executes `lzsClearTable` in-place. |
| `lzsFind` | `(ht_key: &[]u32, ht_val: &[]u16, key: u32): i32` | Invokes `lzsFind` and returns `i32`. |
| `lzsInsert` | `(ht_key: []u32, ht_val: []u16, key: u32, code: u16)` | Executes `lzsInsert` in-place. |
| `compressBytes` | `(input: &[]u8, out: []u8): usize` | Invokes `compressBytes` and returns `usize`. |
| `decompressBytes` | `(compressed: &[]u8, out: []u8): i64` | Invokes `decompressBytes` and returns `i64`. |
| `lzsEncode64` | `(raw: &[]u8, alphabet: string, pad: bool, out: []u8): usize` | Invokes `lzsEncode64` and returns `usize`. |
| `lzsDecodeChar` | `(ch: u8, uri_mode: bool): i32` | Invokes `lzsDecodeChar` and returns `i32`. |
| `lzsDecode64` | `(text: string, uri_mode: bool, out: []u8): i64` | Invokes `lzsDecode64` and returns `i64`. |
| `compressToBase64` | `(input: string, out: []u8): usize` | Invokes `compressToBase64` and returns `usize`. |
| `decompressFromBase64` | `(b64: string, out: []u8): i64` | Invokes `decompressFromBase64` and returns `i64`. |
| `compressToEncodedURIComponent` | `(input: string, out: []u8): usize` | Invokes `compressToEncodedURIComponent` and returns `usize`. |
| `decompressFromEncodedURIComponent` | `(uri: string, out: []u8): i64` | Invokes `decompressFromEncodedURIComponent` and returns `i64`. |

## Minimal Example

```cp
import { lzsDecode64 } from "lz-string";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lzsDecode64("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
