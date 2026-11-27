[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mediatype

Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser, structured suffix extractor, and parameter matcher for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`mediatype`](https://github.com/picoHz/mediatype)（`MIT`）（原库名 `mediatype`，CP 包名 `mediatype`）。

| Metadata | Value |
| --- | --- |
| Package | `mediatype` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mediatype
```

```cp
import {
    MediaType,
    MediaParam,
    ParsedMediaType,
    mtToLower,
    mtIsOws,
    mtTrimOws,
    mtEqIgnoreCase,
    mtIsTchar,
} from "mediatype";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MediaType` | `class` | Fields: `top: string, subtype: string, suffix: string, params: string` |
| `MediaParam` | `class` | Fields: `name: string, value: string` |
| `ParsedMediaType` | `class` | Fields: `valid: bool, top_level: string, sub_level: string, suffix: string, params: [8]MediaParam, param_count: usize` · Methods: `invalid, parse, getParam, hasSuffix, formatEssence, formatFull, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mtToLower` | `(c: u8): u8` | Invokes `mtToLower` and returns `u8`. |
| `mtIsOws` | `(c: u8): bool` | Invokes `mtIsOws` and returns `bool`. |
| `mtTrimOws` | `(s: &[]u8): &[]u8` | Invokes `mtTrimOws` and returns `&[]u8`. |
| `mtEqIgnoreCase` | `(a: &[]u8, b: &[]u8): bool` | Invokes `mtEqIgnoreCase` and returns `bool`. |
| `mtIsTchar` | `(c: u8): bool` | Invokes `mtIsTchar` and returns `bool`. |
| `isValidToken` | `(s: string): bool` | Checks predicate `isValidToken` and returns `bool`. |
| `parseMediaType` | `(input: string, out: &mut MediaType): bool` | Parses input via `parseMediaType` and returns `bool`. |
| `getParam` | `(mt: MediaType, name: string, out: []u8): usize` | Queries/retrieves data via `getParam` returning `usize`. |
| `formatMediaType` | `(mt: MediaType, out: []u8): usize` | Serializes/formats data via `formatMediaType` returning `usize`. |
| `isJsonLike` | `(mt: MediaType): bool` | Checks predicate `isJsonLike` and returns `bool`. |
| `isTextLike` | `(mt: MediaType): bool` | Checks predicate `isTextLike` and returns `bool`. |
| `essenceEq` | `(a: string, b: string): bool` | Invokes `essenceEq` and returns `bool`. |

## Minimal Example

```cp
import { isValidToken } from "mediatype";

pub fn main() {
    let result = isValidToken("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
