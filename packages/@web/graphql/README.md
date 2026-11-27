[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/graphql

Official GraphQL endpoint, depth-limit guard & GraphiQL Playground plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/graphql` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/graphql
```

```cp
import {
    GraphQLConfig,
    GqlResolverEntry,
    gqlBytesEql,
    gqlWriteByte,
    gqlWriteRaw,
    gqlHexLower,
    gqlWriteJsonEscaped,
    gqlIsWs,
} from "@web/graphql";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GraphQLConfig` | `class` | Fields: `endpoint_path: string, enable_playground: bool, max_depth: usize` |
| `GqlResolverEntry` | `class` | Fields: `name: string, resolver: GraphQLResolverFn?` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `gqlBytesEql` | `(a: string, b: string): bool` | Invokes `gqlBytesEql` and returns `bool`. |
| `gqlWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `gqlWriteByte` and returns `bool`. |
| `gqlWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `gqlWriteRaw` and returns `bool`. |
| `gqlHexLower` | `(n: u8): u8` | Invokes `gqlHexLower` and returns `u8`. |
| `gqlWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `gqlWriteJsonEscaped` and returns `bool`. |
| `gqlIsWs` | `(c: u8): bool` | Invokes `gqlIsWs` and returns `bool`. |
| `gqlIsIdentStart` | `(c: u8): bool` | Invokes `gqlIsIdentStart` and returns `bool`. |
| `gqlIsIdentCont` | `(c: u8): bool` | Invokes `gqlIsIdentCont` and returns `bool`. |
| `gqlSkipWsAndComments` | `(s: string, start: usize): usize` | Invokes `gqlSkipWsAndComments` and returns `usize`. |
| `gqlCheckQueryDepth` | `(query: string, max_depth: usize): bool` | Invokes `gqlCheckQueryDepth` and returns `bool`. |
| `gqlRegisterResolver` | `(field_name: string, resolver: GraphQLResolverFn): bool` | Invokes `gqlRegisterResolver` and returns `bool`. |
| `gqlResetResolvers` | `()` | Executes `gqlResetResolvers` in-place. |
| `gqlFindResolver` | `(field_name: string): GraphQLResolverFn?` | Invokes `gqlFindResolver` and returns `GraphQLResolverFn?`. |
| `gqlSkipJsonWs` | `(s: string, start: usize): usize` | Invokes `gqlSkipJsonWs` and returns `usize`. |
| `gqlExtractJsonQuery` | `(body: string, out_buf: []u8): usize?` | Invokes `gqlExtractJsonQuery` and returns `usize?`. |
| `gqlLooksLikeJsonValue` | `(val: string): bool` | Invokes `gqlLooksLikeJsonValue` and returns `bool`. |
| `gqlSkipBalanced` | `(s: string, start: usize, open_ch: u8, close_ch: u8): usize` | Invokes `gqlSkipBalanced` and returns `usize`. |
| `gqlRenderPlaygroundHtml` | `(endpoint: string, out_buf: []u8): usize?` | Invokes `gqlRenderPlaygroundHtml` and returns `usize?`. |
| `gqlGetHandler` | `(ctx: &mut Context): Response` | Invokes `gqlGetHandler` and returns `Response`. |
| `gqlPostHandler` | `(ctx: &mut Context): Response` | Invokes `gqlPostHandler` and returns `Response`. |
| `registerGraphQL` | `(app: &mut App, cfg: GraphQLConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { gqlBytesEql } from "@web/graphql";

pub fn main() {
    let result = gqlBytesEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
