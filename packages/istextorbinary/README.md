[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# istextorbinary

Zero-allocation WHATWG MIME Sniffing Standard binary/text detector and HTML/XML/JSON/SVG content sniffer for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:istextorbinary` ([https://github.com/bevry/istextorbinary](https://github.com/bevry/istextorbinary)) / `crates.io:content_inspector` ([https://github.com/sharkdp/content_inspector](https://github.com/sharkdp/content_inspector))（原库名 `istextorbinary` → CP 包名 `istextorbinary`）

| Metadata | Value |
| --- | --- |
| Package | `istextorbinary` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add istextorbinary
```

```cp
import {
    msToLower,
    msIsSpace,
    msIsTagTerminator,
    msStartsWithIgnoreCase,
    msMatchTag,
    isBinary,
    isText,
    sniffContentType,
} from "istextorbinary";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `msToLower` | `(c: u8): u8` | Invokes `msToLower` and returns `u8`. |
| `msIsSpace` | `(c: u8): bool` | Invokes `msIsSpace` and returns `bool`. |
| `msIsTagTerminator` | `(c: u8): bool` | Invokes `msIsTagTerminator` and returns `bool`. |
| `msStartsWithIgnoreCase` | `(s: &[]u8, prefix: &[]u8): bool` | Invokes `msStartsWithIgnoreCase` and returns `bool`. |
| `msMatchTag` | `(s: &[]u8, tagPrefix: &[]u8): bool` | Invokes `msMatchTag` and returns `bool`. |
| `isBinary` | `(data: &[]u8): bool` | Checks predicate `isBinary` and returns `bool`. |
| `isText` | `(data: &[]u8): bool` | Checks predicate `isText` and returns `bool`. |
| `sniffContentType` | `(data: &[]u8): u8` | Invokes `sniffContentType` and returns `u8`. |
| `contentTypeToMime` | `(ct: u8): string` | Invokes `contentTypeToMime` and returns `string`. |
| `sniffMime` | `(data: &[]u8): string` | Invokes `sniffMime` and returns `string`. |

## Minimal Example

```cp
import { isBinary } from "istextorbinary";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isBinary(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
