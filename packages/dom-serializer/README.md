[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dom-serializer

Zero-heap HTML/XML DOM node serializer (covers domelementtype and w3c-xmlserializer)

| Metadata | Value |
| --- | --- |
| Package | `dom-serializer` |
| Version | `2.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add dom-serializer
```

```cp
import {
    DomNodeKind,
    DomAttr,
    SerializeOptions,
    dsrlWriteByte,
    dsrlWriteAttrValue,
    isVoidElement,
    serializeText,
    serializeOpenTag,
} from "dom-serializer";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `DomNodeKind` | `Element, Text, Comment, Cdata, Doctype` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DomAttr` | `class` | Fields: `name: string, value: string` |
| `SerializeOptions` | `class` | Fields: `xml_mode: bool, self_closing_tags: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `dsrlWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `dsrlWriteByte` and returns `usize`. |
| `dsrlWriteAttrValue` | `(out: []u8, pos: usize, val: string): usize` | Invokes `dsrlWriteAttrValue` and returns `usize`. |
| `isVoidElement` | `(tag: string): bool` | Checks predicate `isVoidElement` and returns `bool`. |
| `serializeText` | `(text: string, out: []u8): usize` | Invokes `serializeText` and returns `usize`. |
| `serializeOpenTag` | `(tag: string, attrs: &[]DomAttr, self_close: bool, out: []u8): usize` | Invokes `serializeOpenTag` and returns `usize`. |
| `serializeElement` | `(tag: string, attrs: &[]DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize` | Invokes `serializeElement` and returns `usize`. |
| `serializeComment` | `(text: string, out: []u8): usize` | Invokes `serializeComment` and returns `usize`. |
| `serializeCdata` | `(text: string, out: []u8): usize` | Invokes `serializeCdata` and returns `usize`. |

## Minimal Example

```cp
import { isVoidElement } from "dom-serializer";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
