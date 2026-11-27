[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# micromark-util-sanitize-uri

Zero-allocation safe URI sanitizer and RFC 3986 percent-encoder blocking unsafe schemes

| Metadata | Value |
| --- | --- |
| Package | `micromark-util-sanitize-uri` |
| Version | `2.0.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add micromark-util-sanitize-uri
```

```cp
import { surIsSafeUriByte, isSafeUri, sanitizeUri } from "micromark-util-sanitize-uri";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `surIsSafeUriByte` | `(b: u8): bool` | Invokes `surIsSafeUriByte` and returns `bool`. |
| `isSafeUri` | `(uri: string): bool` | Checks predicate `isSafeUri` and returns `bool`. |
| `sanitizeUri` | `(uri: string, out: []u8): usize` | Invokes `sanitizeUri` and returns `usize`. |

## Minimal Example

```cp
import { isSafeUri } from "micromark-util-sanitize-uri";

pub fn main() {
    let result = isSafeUri("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
