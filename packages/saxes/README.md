[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# saxes

Zero-allocation streaming SAX XML 1.0 event parser with tag stack validation and entity decoding (npm saxes parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:saxes` ([https://github.com/lddubeau/saxes](https://github.com/lddubeau/saxes))（原库名 `saxes` → CP 包名 `saxes`）

| Metadata | Value |
| --- | --- |
| Package | `saxes` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add saxes
```

```cp
import {
    XmlEventKind,
    XmlAttr,
    XmlEvent,
    SaxesParser,
    saxEmptyAttrs,
    saxMakeEvent,
    saxIsWs,
    saxIsNameStart,
} from "saxes";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `XmlEventKind` | `OpenTag, CloseTag, SelfClosingTag, Text, Cdata, Comment, ProcessingInstruction, Error` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `XmlAttr` | `class` | Fields: `name: string, value: string` |
| `XmlEvent` | `class` | Fields: `kind: XmlEventKind, name: string, text: string, attrs: [8]XmlAttr, attr_count: usize` |
| `SaxesParser` | `class` | Fields: `stack: [32]string, depth: usize, has_err: bool, root_count: usize` · Methods: `new, parseAll, isWellFormed, decodeEntities` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `saxEmptyAttrs` | `(): [8]XmlAttr` | Invokes `saxEmptyAttrs` and returns `[8]XmlAttr`. |
| `saxMakeEvent` | `(kind: XmlEventKind, name: string, text: string): XmlEvent` | Invokes `saxMakeEvent` and returns `XmlEvent`. |
| `saxIsWs` | `(ch: u8): bool` | Invokes `saxIsWs` and returns `bool`. |
| `saxIsNameStart` | `(ch: u8): bool` | Invokes `saxIsNameStart` and returns `bool`. |
| `saxIsNameChar` | `(ch: u8): bool` | Invokes `saxIsNameChar` and returns `bool`. |
| `saxStrEq` | `(a: string, b: string): bool` | Invokes `saxStrEq` and returns `bool`. |
| `saxStartsWithAt` | `(s: string, pos: usize, prefix: string): bool` | Invokes `saxStartsWithAt` and returns `bool`. |
| `saxFindSubstrFrom` | `(s: string, start: usize, needle: string): i64` | Invokes `saxFindSubstrFrom` and returns `i64`. |
| `saxAllWs` | `(s: string): bool` | Invokes `saxAllWs` and returns `bool`. |
| `saxEmit` | `(out: []XmlEvent, count: usize, ev: XmlEvent): usize` | Invokes `saxEmit` and returns `usize`. |
| `saxEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | Invokes `saxEncodeUtf8` and returns `usize`. |
| `saxHexVal` | `(ch: u8): i32` | Invokes `saxHexVal` and returns `i32`. |
| `saxDecodeEntitiesImpl` | `(raw: string, out: []u8): usize` | Invokes `saxDecodeEntitiesImpl` and returns `usize`. |

## Minimal Example

```cp
import { saxMakeEvent } from "saxes";

pub fn main() {
    let result = saxMakeEvent(/* kind: XmlEventKind */, "example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
