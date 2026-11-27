[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zip

Zero-heap PKZIP Stored archive writer, local header & EOCD parser, and CRC-32 verifier (crates.io zip equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zip` ([https://github.com/zip-rs/zip2](https://github.com/zip-rs/zip2))（原库名 `zip` → CP 包名 `zip`）

| Metadata | Value |
| --- | --- |
| Package | `zip` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add zip
```

```cp
import {
    ZipLocalHeader,
    ZipEocd,
    zipEmptyLocalHeader,
    zipEmptyEocd,
    zipCopyStr,
    zipCopyBytes,
    writeStoredZipSingle,
    parseLocalHeader,
} from "zip";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ZipLocalHeader` | `class` | Fields: `version_needed: u16, flags: u16, method: u16, crc32: u32, compressed_size: u32, uncompressed_size: u32, +4 more` |
| `ZipEocd` | `class` | Fields: `disk_num: u16, total_entries: u16, cd_size: u32, cd_offset: u32, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `zipEmptyLocalHeader` | `(): ZipLocalHeader` | Invokes `zipEmptyLocalHeader` and returns `ZipLocalHeader`. |
| `zipEmptyEocd` | `(): ZipEocd` | Invokes `zipEmptyEocd` and returns `ZipEocd`. |
| `zipCopyStr` | `(dst: []u8, offset: usize, s: string)` | Executes `zipCopyStr` in-place. |
| `zipCopyBytes` | `(dst: []u8, offset: usize, src: []u8)` | Executes `zipCopyBytes` in-place. |
| `writeStoredZipSingle` | `(filename: string, payload: []u8, out: []u8): usize?` | Invokes `writeStoredZipSingle` and returns `usize?`. |
| `parseLocalHeader` | `(buf: []u8, offset: usize): ZipLocalHeader` | Parses input via `parseLocalHeader` and returns `ZipLocalHeader`. |
| `findAndParseEocd` | `(buf: []u8): ZipEocd` | Queries/retrieves data via `findAndParseEocd` returning `ZipEocd`. |
| `verifyStoredEntry` | `(buf: []u8, hdr: ZipLocalHeader): bool` | Validates input with `verifyStoredEntry` returning `bool`. |

## Minimal Example

```cp
import { writeStoredZipSingle } from "zip";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = writeStoredZipSingle("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
