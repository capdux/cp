[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# http

Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types (crates.io http v1 parity) composing unicase for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http` ([https://github.com/hyperium/http](https://github.com/hyperium/http))（原库名 `http` → CP 包名 `http`）

| Metadata | Value |
| --- | --- |
| Package | `http` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add http
```

```cp
import {
    StatusCode,
    htpStrEq,
    htpIsTchar,
    parseMethod,
    methodAsStr,
    isSafeMethod,
    isIdempotentMethod,
    parseHttpVersion,
} from "http";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StatusCode` | `class` | Fields: `code: u16` · Methods: `fromU16, asU16, canonicalReason, isInformational, isSuccess, isRedirection, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `htpStrEq` | `(a: string, b: string): bool` | Invokes `htpStrEq` and returns `bool`. |
| `htpIsTchar` | `(b: u8): bool` | Invokes `htpIsTchar` and returns `bool`. |
| `parseMethod` | `(s: string): u8` | Parses input via `parseMethod` and returns `u8`. |
| `methodAsStr` | `(m: u8): string` | Invokes `methodAsStr` and returns `string`. |
| `isSafeMethod` | `(m: u8): bool` | Checks predicate `isSafeMethod` and returns `bool`. |
| `isIdempotentMethod` | `(m: u8): bool` | Checks predicate `isIdempotentMethod` and returns `bool`. |
| `parseHttpVersion` | `(s: string): u8` | Parses input via `parseHttpVersion` and returns `u8`. |
| `httpVersionAsStr` | `(v: u8): string` | Invokes `httpVersionAsStr` and returns `string`. |
| `isValidHeaderName` | `(name: string): bool` | Checks predicate `isValidHeaderName` and returns `bool`. |
| `headerNameEq` | `(a: string, b: string): bool` | Invokes `headerNameEq` and returns `bool`. |

## Minimal Example

```cp
import { parseMethod } from "http";

pub fn main() {
    let result = parseMethod("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
