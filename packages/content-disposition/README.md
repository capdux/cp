[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# content-disposition

Zero-allocation RFC 6266 / RFC 5987 HTTP Content-Disposition header parser, formatter, filename* decoder, and filename sanitizer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:content-disposition` ([https://github.com/jshttp/content-disposition](https://github.com/jshttp/content-disposition))

| Metadata | Value |
| --- | --- |
| Package | `content-disposition` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add content-disposition
```

```cp
import {
    ContentDisposition,
    cdspIsOws,
    cdspIsTokenChar,
    cdspSkipOws,
    cdspHasNonAscii,
    cdspWriteByte,
    cdspWriteStr,
    cdspWriteAsciiFallback,
} from "content-disposition";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ContentDisposition` | `class` | Fields: `valid: bool, disp_type: string, filename: string, filename_star: string` · Methods: `invalid, isAttachment, isInline, hasFilename` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cdspIsOws` | `(b: u8): bool` | Invokes `cdspIsOws` and returns `bool`. |
| `cdspIsTokenChar` | `(b: u8): bool` | Invokes `cdspIsTokenChar` and returns `bool`. |
| `cdspSkipOws` | `(s: string, start: usize): usize` | Invokes `cdspSkipOws` and returns `usize`. |
| `cdspHasNonAscii` | `(s: string): bool` | Invokes `cdspHasNonAscii` and returns `bool`. |
| `cdspWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `cdspWriteByte` and returns `usize`. |
| `cdspWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `cdspWriteStr` and returns `usize`. |
| `cdspWriteAsciiFallback` | `(out: []u8, pos: usize, s: string): usize` | Invokes `cdspWriteAsciiFallback` and returns `usize`. |
| `cdspRfc5987Set` | `(): AsciiSet` | Invokes `cdspRfc5987Set` and returns `AsciiSet`. |
| `cdspIsControl` | `(b: u8): bool` | Invokes `cdspIsControl` and returns `bool`. |
| `parseContentDisposition` | `(header: string): ContentDisposition` | Parses input via `parseContentDisposition` and returns `ContentDisposition`. |
| `sanitizeFilename` | `(raw: string, out: []u8): usize` | Invokes `sanitizeFilename` and returns `usize`. |
| `decodeFilenameStar` | `(raw_star: string, out: []u8): usize` | Parses input via `decodeFilenameStar` and returns `usize`. |
| `formatContentDisposition` | `(disp_type: string, filename: string, out: []u8): usize` | Serializes/formats data via `formatContentDisposition` returning `usize`. |

## Minimal Example

```cp
import { parseContentDisposition } from "content-disposition";

pub fn main() {
    let result = parseContentDisposition("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
