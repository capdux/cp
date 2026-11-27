[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# accept-header

Zero-heap RFC 7231 HTTP Accept header parser and media-type negotiator with quality-factor & specificity ranking (clean-room port of Rust accept-header)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:accept-header` ([https://github.com/djc/accept-header](https://github.com/djc/accept-header))

| Metadata | Value |
| --- | --- |
| Package | `accept-header` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add accept-header
```

```cp
import {
    AcceptEntry,
    AcceptHeader,
    acphFindSlash,
    acphIsStar,
    acphMediaSpecificity,
    acphMatchesRange,
    acphEmptyEntry,
    acphEmptySpec,
} from "accept-header";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AcceptEntry` | `class` | Fields: `media_range: string, quality_milli: u16, specificity: u8` |
| `AcceptHeader` | `class` | Fields: `entries: [16]AcceptEntry, count: usize` · Methods: `parse, len, accepts, bestMatch` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `acphFindSlash` | `(s: string): usize` | Invokes `acphFindSlash` and returns `usize`. |
| `acphIsStar` | `(s: string): bool` | Invokes `acphIsStar` and returns `bool`. |
| `acphMediaSpecificity` | `(range: string): u8` | Invokes `acphMediaSpecificity` and returns `u8`. |
| `acphMatchesRange` | `(range: string, mime: string): bool` | Invokes `acphMatchesRange` and returns `bool`. |
| `acphEmptyEntry` | `(): AcceptEntry` | Invokes `acphEmptyEntry` and returns `AcceptEntry`. |
| `acphEmptySpec` | `(): QualitySpec` | Invokes `acphEmptySpec` and returns `QualitySpec`. |
| `negotiateMediaType` | `(header: string, available: []string): string?` | Invokes `negotiateMediaType` and returns `string?`. |

## Minimal Example

```cp
import { acphFindSlash } from "accept-header";

pub fn main() {
    let result = acphFindSlash("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
