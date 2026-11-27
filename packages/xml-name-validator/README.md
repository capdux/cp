[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# xml-name-validator

Zero-allocation W3C XML 1.0 (5th Ed.) & Namespaces in XML 1.0 Char, Name, NCName, and QName validator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:xml-name-validator` ([https://github.com/jsdom/xml-name-validator](https://github.com/jsdom/xml-name-validator), `Apache-2.0`)，同步覆盖 `npm:xmlchars` 字符类判定规则。

| Metadata | Value |
| --- | --- |
| Package | `xml-name-validator` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add xml-name-validator
```

```cp
import {
    XnvDecoded,
    xnvDecodeUtf8,
    xnvValidateName,
    isXmlChar,
    isNameStartChar,
    isNameChar,
    isValidXmlName,
    isValidNCName,
} from "xml-name-validator";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `XnvDecoded` | `class` | Fields: `cp: u32, len: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `xnvDecodeUtf8` | `(s: string, pos: usize): XnvDecoded` | Invokes `xnvDecodeUtf8` and returns `XnvDecoded`. |
| `xnvValidateName` | `(s: string, allow_colon: bool): bool` | Invokes `xnvValidateName` and returns `bool`. |
| `isXmlChar` | `(cp: u32): bool` | Checks predicate `isXmlChar` and returns `bool`. |
| `isNameStartChar` | `(cp: u32): bool` | Checks predicate `isNameStartChar` and returns `bool`. |
| `isNameChar` | `(cp: u32): bool` | Checks predicate `isNameChar` and returns `bool`. |
| `isValidXmlName` | `(s: string): bool` | Checks predicate `isValidXmlName` and returns `bool`. |
| `isValidNCName` | `(s: string): bool` | Checks predicate `isValidNCName` and returns `bool`. |
| `isValidQName` | `(s: string): bool` | Checks predicate `isValidQName` and returns `bool`. |

## Minimal Example

```cp
import { isValidXmlName } from "xml-name-validator";

pub fn main() {
    let result = isValidXmlName("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
