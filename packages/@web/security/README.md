[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/security

Official Helmet security headers (HSTS/CSP/X-Frame-Options/Referrer-Policy) and Double-Submit CSRF plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/security` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/security
```

```cp
import {
    SecurityConfig,
    constantTimeEql,
    isSafeHttpMethod,
    securityOnRequest,
    createSecurityPlugin,
    registerSecurity,
} from "@web/security";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SecurityConfig` | `class` | Fields: `hsts: string, csp: string, x_frame_options: string, x_content_type_options: string, referrer_policy: string, permissions_policy: string, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `constantTimeEql` | `(a: string, b: string): bool` | Invokes `constantTimeEql` and returns `bool`. |
| `isSafeHttpMethod` | `(m: AppMethod): bool` | Checks predicate `isSafeHttpMethod` and returns `bool`. |
| `securityOnRequest` | `(ctx: &mut Context): bool` | Invokes `securityOnRequest` and returns `bool`. |
| `createSecurityPlugin` | `(cfg: SecurityConfig): Plugin` | Constructs/initializes an instance via `createSecurityPlugin`. |
| `registerSecurity` | `(app: &mut App, cfg: SecurityConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { constantTimeEql } from "@web/security";

pub fn main() {
    let result = constantTimeEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
