[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/cors

Official CORS middleware plugin for cap:web (preflight OPTIONS, origin allowlist, credentials, exposed headers)

| Metadata | Value |
| --- | --- |
| Package | `@web/cors` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/cors
```

```cp
import {
    CorsConfig,
    corsHasCtlOrCrlf,
    corsBytesEql,
    isOriginAllowed,
    applyCorsHeaders,
    corsOnRequest,
    createCorsPlugin,
    registerCors,
} from "@web/cors";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CorsConfig` | `class` | Fields: `allow_origin: string, allow_methods: string, allow_headers: string, expose_headers: string, allow_credentials: bool, max_age: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `corsHasCtlOrCrlf` | `(s: string): bool` | Invokes `corsHasCtlOrCrlf` and returns `bool`. |
| `corsBytesEql` | `(a: string, b: string): bool` | Invokes `corsBytesEql` and returns `bool`. |
| `isOriginAllowed` | `(allow_origin: string, origin: string): bool` | Checks predicate `isOriginAllowed` and returns `bool`. |
| `applyCorsHeaders` | `(ctx: &mut Context, origin: string)` | Executes `applyCorsHeaders` in-place. |
| `corsOnRequest` | `(ctx: &mut Context): bool` | Invokes `corsOnRequest` and returns `bool`. |
| `createCorsPlugin` | `(cfg: CorsConfig): Plugin` | Constructs/initializes an instance via `createCorsPlugin`. |
| `registerCors` | `(app: &mut App, cfg: CorsConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { isOriginAllowed } from "@web/cors";

pub fn main() {
    let result = isOriginAllowed("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
