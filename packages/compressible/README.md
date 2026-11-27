[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# compressible

Zero-allocation HTTP Content-Type compressibility checker

| Metadata | Value |
| --- | --- |
| Package | `compressible` |
| Version | `2.0.18` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add compressible
```

```cp
import {
    cmprIsSpace,
    cmprToLower,
    cmprEqlIgnoreCase,
    cmprHasPrefixIgnoreCase,
    cmprHasSuffixIgnoreCase,
    cmprFindSlash,
    stripContentTypeParams,
    isCompressible,
} from "compressible";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmprIsSpace` | `(b: u8): bool` | Invokes `cmprIsSpace` and returns `bool`. |
| `cmprToLower` | `(b: u8): u8` | Invokes `cmprToLower` and returns `u8`. |
| `cmprEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `cmprEqlIgnoreCase` and returns `bool`. |
| `cmprHasPrefixIgnoreCase` | `(s: string, prefix: string): bool` | Invokes `cmprHasPrefixIgnoreCase` and returns `bool`. |
| `cmprHasSuffixIgnoreCase` | `(s: string, suffix: string): bool` | Invokes `cmprHasSuffixIgnoreCase` and returns `bool`. |
| `cmprFindSlash` | `(s: string): usize` | Invokes `cmprFindSlash` and returns `usize`. |
| `stripContentTypeParams` | `(ct: string): string` | Invokes `stripContentTypeParams` and returns `string`. |
| `isCompressible` | `(content_type: string): bool` | Checks predicate `isCompressible` and returns `bool`. |

## Minimal Example

```cp
import { isCompressible } from "compressible";

pub fn main() {
    let result = isCompressible("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
