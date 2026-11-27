[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/swagger

Official OpenAPI 3.1.0 & interactive Scalar/Swagger UI documentation plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/swagger` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/swagger
```

```cp
import {
    SwaggerConfig,
    swAsciiLower,
    swAsciiEqlIgnoreCase,
    swWriteByte,
    swWriteRaw,
    swWriteHtmlEscaped,
    renderDocsHtml,
    refreshSpec,
} from "@web/swagger";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SwaggerConfig` | `class` | Fields: `title: string, version: string, spec_path: string, docs_path: string, ui_theme: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `swAsciiLower` | `(c: u8): u8` | Invokes `swAsciiLower` and returns `u8`. |
| `swAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `swAsciiEqlIgnoreCase` and returns `bool`. |
| `swWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `swWriteByte` and returns `bool`. |
| `swWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `swWriteRaw` and returns `bool`. |
| `swWriteHtmlEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `swWriteHtmlEscaped` and returns `bool`. |
| `renderDocsHtml` | `(cfg: SwaggerConfig, out_buf: []u8): usize?` | Invokes `renderDocsHtml` and returns `usize?`. |
| `refreshSpec` | `(app: &mut App): usize?` | Invokes `refreshSpec` and returns `usize?`. |
| `cachedSpec` | `(): string` | Invokes `cachedSpec` and returns `string`. |
| `cachedDocsHtml` | `(): string` | Invokes `cachedDocsHtml` and returns `string`. |
| `swOpenApiSpecHandler` | `(ctx: &mut Context): Response` | Invokes `swOpenApiSpecHandler` and returns `Response`. |
| `swDocsUiHandler` | `(ctx: &mut Context): Response` | Invokes `swDocsUiHandler` and returns `Response`. |
| `registerSwagger` | `(app: &mut App, cfg: SwaggerConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { swAsciiEqlIgnoreCase } from "@web/swagger";

pub fn main() {
    let result = swAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
