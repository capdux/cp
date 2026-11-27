[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/compress

Official HTTP response compression plugin for cap:web with pure-CP zero-heap RFC 1952 Gzip encoder

| Metadata | Value |
| --- | --- |
| Package | `@web/compress` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/compress
```

```cp
import {
    EncodingKind,
    CompressConfig,
    cmpAsciiLower,
    cmpEqlIgnoreCase,
    cmpTrimSpaces,
    cmpParseQValue,
    negotiateEncoding,
    cmpCrc32,
} from "@web/compress";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `EncodingKind` | `None, Gzip, Deflate` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CompressConfig` | `class` | Fields: `min_bytes: usize, prefer_gzip: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmpAsciiLower` | `(c: u8): u8` | Invokes `cmpAsciiLower` and returns `u8`. |
| `cmpEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `cmpEqlIgnoreCase` and returns `bool`. |
| `cmpTrimSpaces` | `(s: string): string` | Invokes `cmpTrimSpaces` and returns `string`. |
| `cmpParseQValue` | `(raw: string): i32` | Invokes `cmpParseQValue` and returns `i32`. |
| `negotiateEncoding` | `(accept_encoding: string, prefer_gzip: bool): EncodingKind` | Invokes `negotiateEncoding` and returns `EncodingKind`. |
| `cmpCrc32` | `(data: string): u32` | Invokes `cmpCrc32` and returns `u32`. |
| `cmpWriteU16Le` | `(out_buf: []u8, pos: usize, val: u16)` | Executes `cmpWriteU16Le` in-place. |
| `cmpWriteU32Le` | `(out_buf: []u8, pos: usize, val: u32)` | Executes `cmpWriteU32Le` in-place. |
| `encodeGzipStored` | `(src: string, out_buf: []u8): usize?` | Serializes/formats data via `encodeGzipStored` returning `usize?`. |
| `compressResponseHook` | `(ctx: &mut Context, resp: &mut Response)` | Executes `compressResponseHook` in-place. |
| `createCompressPlugin` | `(cfg: CompressConfig): Plugin` | Constructs/initializes an instance via `createCompressPlugin`. |
| `registerCompress` | `(app: &mut App, cfg: CompressConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { encodeGzipStored } from "@web/compress";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeGzipStored("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
