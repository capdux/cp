[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# content-type

Zero-heap RFC 7231 HTTP Content-Type header parser and formatter with quoted-string unescaping (clean-room port of content-type)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:content-type` ([https://github.com/jshttp/content-type](https://github.com/jshttp/content-type))

| Metadata | Value |
| --- | --- |
| Package | `content-type` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add content-type
```

```cp
import {
    ContentTypeParam,
    ContentType,
    ctypIsOws,
    ctypIsTokenChar,
    ctypIsValidToken,
    ctypIsQuotedAllowed,
    ctypWriteByte,
    ctypWriteSlice,
} from "content-type";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ContentTypeParam` | `class` | Fields: `name: string, value: string` · Methods: `new` |
| `ContentType` | `class` | Fields: `valid: bool, media_type: string, params: [8]ContentTypeParam, param_count: usize` · Methods: `invalid, getParam, getParamUnescaped, charset, boundary` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ctypIsOws` | `(c: u8): bool` | Invokes `ctypIsOws` and returns `bool`. |
| `ctypIsTokenChar` | `(c: u8): bool` | Invokes `ctypIsTokenChar` and returns `bool`. |
| `ctypIsValidToken` | `(s: string): bool` | Invokes `ctypIsValidToken` and returns `bool`. |
| `ctypIsQuotedAllowed` | `(c: u8): bool` | Invokes `ctypIsQuotedAllowed` and returns `bool`. |
| `ctypWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `ctypWriteByte` and returns `usize`. |
| `ctypWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `ctypWriteSlice` and returns `usize`. |
| `isValidMediaType` | `(s: string): bool` | Checks predicate `isValidMediaType` and returns `bool`. |
| `unescapeQuotedString` | `(raw: string, out: []u8): usize` | Invokes `unescapeQuotedString` and returns `usize`. |
| `parseContentType` | `(header: string): ContentType` | Parses input via `parseContentType` and returns `ContentType`. |
| `formatContentType` | `(media_type: string, params: &[]ContentTypeParam, out: []u8): usize` | Serializes/formats data via `formatContentType` returning `usize`. |

## Minimal Example

```cp
import { isValidMediaType } from "content-type";

pub fn main() {
    let result = isValidMediaType("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
