[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# set-cookie-parser

Zero-allocation RFC 6265 Set-Cookie HTTP header parser and splitter

| Metadata | Value |
| --- | --- |
| Package | `set-cookie-parser` |
| Version | `2.7.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add set-cookie-parser
```

```cp
import {
    ParsedCookie,
    scpStripQuotes,
    parseSetCookie,
    splitCookiesCount,
    splitCookiesGet,
} from "set-cookie-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedCookie` | `class` | Fields: `name: string, value: string, path: string, domain: string, expires: string, same_site: string, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `scpStripQuotes` | `(s: string): string` | Invokes `scpStripQuotes` and returns `string`. |
| `parseSetCookie` | `(header: string): ParsedCookie` | Parses input via `parseSetCookie` and returns `ParsedCookie`. |
| `splitCookiesCount` | `(combined: string): usize` | Invokes `splitCookiesCount` and returns `usize`. |
| `splitCookiesGet` | `(combined: string, target_idx: usize): string?` | Invokes `splitCookiesGet` and returns `string?`. |

## Minimal Example

```cp
import { parseSetCookie } from "set-cookie-parser";

pub fn main() {
    let result = parseSetCookie("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
