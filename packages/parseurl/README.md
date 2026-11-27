[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parseurl

Zero-allocation high-speed HTTP request URL pathname/query zero-copy splitter and path safety checker for CP (clean-room parity with Express parseurl)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parseurl` ([https://github.com/pillarjs/parseurl](https://github.com/pillarjs/parseurl))

| Metadata | Value |
| --- | --- |
| Package | `parseurl` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parseurl
```

```cp
import {
    ParsedRequestUrl,
    purlIsAlpha,
    purlIsSchemeChar,
    purlHexVal,
    purlFindOriginEnd,
    parseRequestUrl,
    decodePathname,
    isSafeRequestPath,
} from "parseurl";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedRequestUrl` | `class` | Fields: `href: string, path: string, pathname: string, search: string, query: string, hash: string, +1 more` · Methods: `empty` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `purlIsAlpha` | `(b: u8): bool` | Invokes `purlIsAlpha` and returns `bool`. |
| `purlIsSchemeChar` | `(b: u8): bool` | Invokes `purlIsSchemeChar` and returns `bool`. |
| `purlHexVal` | `(ch: u8): u8` | Invokes `purlHexVal` and returns `u8`. |
| `purlFindOriginEnd` | `(raw_url: string): usize` | Invokes `purlFindOriginEnd` and returns `usize`. |
| `parseRequestUrl` | `(raw_url: string): ParsedRequestUrl` | Parses input via `parseRequestUrl` and returns `ParsedRequestUrl`. |
| `decodePathname` | `(pathname: string, out: []u8): usize` | Parses input via `decodePathname` and returns `usize`. |
| `isSafeRequestPath` | `(pathname: string): bool` | Checks predicate `isSafeRequestPath` and returns `bool`. |
| `getRequestQueryParam` | `(raw_url: string, key: string, out: []u8): usize?` | Queries/retrieves data via `getRequestQueryParam` returning `usize?`. |
| `decodeQueryValue` | `(raw: string, out: []u8): usize` | Parses input via `decodeQueryValue` and returns `usize`. |

## Minimal Example

```cp
import { parseRequestUrl } from "parseurl";

pub fn main() {
    let result = parseRequestUrl("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
