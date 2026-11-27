[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tower-http

Zero-heap CORS origin matcher, wildcard subdomain check, and preflight method/header validator for CP (compatible with npm cors & Rust tower-http::cors)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tower-http` ([https://github.com/tower-rs/tower-http](https://github.com/tower-rs/tower-http))（原库名 `tower-http` → CP 包名 `tower-http`）

| Metadata | Value |
| --- | --- |
| Package | `tower-http` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tower-http
```

```cp
import {
    findSchemeSep,
    isValidOrigin,
    matchOrigin,
    matchOriginList,
    isMethodAllowed,
    areHeadersAllowed,
    isSimpleMethod,
} from "tower-http";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `findSchemeSep` | `(s: string): i64` | Queries/retrieves data via `findSchemeSep` returning `i64`. |
| `isValidOrigin` | `(origin: string): bool` | Checks predicate `isValidOrigin` and returns `bool`. |
| `matchOrigin` | `(origin: string, pattern: string, allow_credentials: bool): bool` | Invokes `matchOrigin` and returns `bool`. |
| `matchOriginList` | `(origin: string, patterns: &[]string, allow_credentials: bool): bool` | Invokes `matchOriginList` and returns `bool`. |
| `isMethodAllowed` | `(method: string, allowed_methods: string): bool` | Checks predicate `isMethodAllowed` and returns `bool`. |
| `areHeadersAllowed` | `(req_headers: string, allowed_headers: string): bool` | Invokes `areHeadersAllowed` and returns `bool`. |
| `isSimpleMethod` | `(method: string): bool` | Checks predicate `isSimpleMethod` and returns `bool`. |

## Minimal Example

```cp
import { isValidOrigin } from "tower-http";

pub fn main() {
    let result = isValidOrigin("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
