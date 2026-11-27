[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/sensible

Official RFC 9457 Problem Details & standard HTTP error helpers plugin for cap:web (inspired by @fastify/sensible)

| Metadata | Value |
| --- | --- |
| Package | `@web/sensible` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/sensible
```

```cp
import {
    SensibleConfig,
    sensWriteRaw,
    sensWriteEscaped,
    sensWriteU16,
    problemResponse,
    httpBadRequest,
    httpUnauthorized,
    httpForbidden,
} from "@web/sensible";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SensibleConfig` | `class` | Fields: `expose_problem_type_base: string, default_error_header: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sensWriteRaw` | `(dst: []u8, pos: &mut usize, s: string): bool` | Invokes `sensWriteRaw` and returns `bool`. |
| `sensWriteEscaped` | `(dst: []u8, pos: &mut usize, s: string): bool` | Invokes `sensWriteEscaped` and returns `bool`. |
| `sensWriteU16` | `(dst: []u8, pos: &mut usize, v: u16): bool` | Invokes `sensWriteU16` and returns `bool`. |
| `problemResponse` | `(status_code: u16, title: string, detail: string): Response` | Invokes `problemResponse` and returns `Response`. |
| `httpBadRequest` | `(detail: string): Response` | Invokes `httpBadRequest` and returns `Response`. |
| `httpUnauthorized` | `(detail: string): Response` | Invokes `httpUnauthorized` and returns `Response`. |
| `httpForbidden` | `(detail: string): Response` | Invokes `httpForbidden` and returns `Response`. |
| `httpNotFound` | `(detail: string): Response` | Invokes `httpNotFound` and returns `Response`. |
| `httpConflict` | `(detail: string): Response` | Invokes `httpConflict` and returns `Response`. |
| `httpUnprocessable` | `(detail: string): Response` | Invokes `httpUnprocessable` and returns `Response`. |
| `httpTooManyRequests` | `(detail: string): Response` | Invokes `httpTooManyRequests` and returns `Response`. |
| `httpInternalError` | `(detail: string): Response` | Invokes `httpInternalError` and returns `Response`. |
| `sensibleAssert` | `(ctx: &mut Context, cond: bool, status_code: u16, title: string, detail: string): bool` | Invokes `sensibleAssert` and returns `bool`. |
| `sensibleAfterHook` | `(ctx: &mut Context, resp: &mut Response)` | Executes `sensibleAfterHook` in-place. |
| `createSensiblePlugin` | `(cfg: SensibleConfig): Plugin` | Constructs/initializes an instance via `createSensiblePlugin`. |
| `registerSensible` | `(app: &mut App, cfg: SensibleConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { sensWriteRaw } from "@web/sensible";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sensWriteRaw(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
