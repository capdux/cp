[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/basic-auth

Official HTTP Basic Auth (RFC 7617) and X-API-Key constant-time authentication plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/basic-auth` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/basic-auth
```

```cp
import {
    BasicAuthConfig,
    baConstTimeEql,
    baDecodeVal,
    basicAuthDecodeBase64,
    baStartsWithIgnoreCase,
    basicAuthHook,
    createBasicAuthPlugin,
    registerBasicAuth,
} from "@web/basic-auth";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BasicAuthConfig` | `class` | Fields: `username: string, password: string, api_key: string, api_key_header: string, realm: string, optional_auth: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `baConstTimeEql` | `(a: string, b: string): bool` | Invokes `baConstTimeEql` and returns `bool`. |
| `baDecodeVal` | `(c: u8): u8?` | Invokes `baDecodeVal` and returns `u8?`. |
| `basicAuthDecodeBase64` | `(input: string, out_buf: []u8): usize?` | Invokes `basicAuthDecodeBase64` and returns `usize?`. |
| `baStartsWithIgnoreCase` | `(s: string, prefix: string): bool` | Invokes `baStartsWithIgnoreCase` and returns `bool`. |
| `basicAuthHook` | `(ctx: &mut Context): bool` | Invokes `basicAuthHook` and returns `bool`. |
| `createBasicAuthPlugin` | `(cfg: BasicAuthConfig): Plugin` | Constructs/initializes an instance via `createBasicAuthPlugin`. |
| `registerBasicAuth` | `(app: &mut App, cfg: BasicAuthConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { baConstTimeEql } from "@web/basic-auth";

pub fn main() {
    let result = baConstTimeEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
