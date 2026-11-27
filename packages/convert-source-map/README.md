[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# convert-source-map

Zero-allocation inline sourceMappingURL comment extractor, Base64 decoder/encoder, and comment stripper (clean-room MIT implementation of npm convert-source-map, #96 / 307M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:convert-source-map` ([https://github.com/thlorenz/convert-source-map](https://github.com/thlorenz/convert-source-map)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `convert-source-map` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add convert-source-map
```

```cp
import {
    SourceMapCommentKind,
    SourceMapComment,
    csmStartsWith,
    csmPush,
    csmB64Val,
    csmB64Encode,
    csmB64Decode,
    findSourceMapComment,
} from "convert-source-map";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SourceMapCommentKind` | `None, InlineBase64, ExternalFile` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SourceMapComment` | `class` | Fields: `kind: SourceMapCommentKind, comment_start: usize, comment_end: usize, payload: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `csmStartsWith` | `(s: string, pos: usize, lit: string): bool` | Invokes `csmStartsWith` and returns `bool`. |
| `csmPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `csmPush` and returns `usize`. |
| `csmB64Val` | `(b: u8): i32` | Invokes `csmB64Val` and returns `i32`. |
| `csmB64Encode` | `(data: &[]u8, out: []u8, start_pos: usize): usize` | Invokes `csmB64Encode` and returns `usize`. |
| `csmB64Decode` | `(b64: string, out: []u8): i64` | Invokes `csmB64Decode` and returns `i64`. |
| `findSourceMapComment` | `(src: string): SourceMapComment` | Queries/retrieves data via `findSourceMapComment` returning `SourceMapComment`. |
| `fromSource` | `(src: string, out: []u8): i64` | Invokes `fromSource` and returns `i64`. |
| `toComment` | `(json_map: string, multiline: bool, out: []u8): usize` | Invokes `toComment` and returns `usize`. |
| `removeComments` | `(src: string, out: []u8): usize` | Invokes `removeComments` and returns `usize`. |

## Minimal Example

```cp
import { csmStartsWith } from "convert-source-map";

pub fn main() {
    let result = csmStartsWith("example", 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
