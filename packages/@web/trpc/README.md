[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/trpc

Official type-safe tRPC-style query & mutation RPC router plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/trpc` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/trpc
```

```cp
import {
    TrpcProcKind,
    TrpcConfig,
    TrpcProcEntry,
    trpcBytesEql,
    trpcWriteRaw,
    trpcRegisterProc,
    trpcQuery,
    trpcMutation,
} from "@web/trpc";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `TrpcProcKind` | `Query, Mutation` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TrpcConfig` | `class` | Fields: `base_path: string` |
| `TrpcProcEntry` | `class` | Fields: `name: string, kind: TrpcProcKind, handler: TrpcHandlerFn?` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `trpcBytesEql` | `(a: string, b: string): bool` | Invokes `trpcBytesEql` and returns `bool`. |
| `trpcWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `trpcWriteRaw` and returns `bool`. |
| `trpcRegisterProc` | `(name: string, kind: TrpcProcKind, handler: TrpcHandlerFn): bool` | Invokes `trpcRegisterProc` and returns `bool`. |
| `trpcQuery` | `(name: string, handler: TrpcHandlerFn): bool` | Invokes `trpcQuery` and returns `bool`. |
| `trpcMutation` | `(name: string, handler: TrpcHandlerFn): bool` | Invokes `trpcMutation` and returns `bool`. |
| `trpcReset` | `()` | Executes `trpcReset` in-place. |
| `trpcFindProcIndex` | `(name: string): usize?` | Invokes `trpcFindProcIndex` and returns `usize?`. |
| `trpcWrapResult` | `(raw_json: string): Response` | Invokes `trpcWrapResult` and returns `Response`. |
| `trpcGetHandler` | `(ctx: &mut Context): Response` | Invokes `trpcGetHandler` and returns `Response`. |
| `trpcPostHandler` | `(ctx: &mut Context): Response` | Invokes `trpcPostHandler` and returns `Response`. |
| `trpcBuildRoutePattern` | `(base_path: string): string` | Invokes `trpcBuildRoutePattern` and returns `string`. |
| `registerTrpc` | `(app: &mut App, cfg: TrpcConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { trpcBytesEql } from "@web/trpc";

pub fn main() {
    let result = trpcBytesEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
