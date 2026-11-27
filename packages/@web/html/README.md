[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/html

Official HTML SSR, zero-heap XSS escaping, and HTMX helpers plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/html` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/html
```

```cp
import {
    HtmlConfig,
    HtmlBuilder,
    htmAsciiLower,
    htmAsciiEqlIgnoreCase,
    htmWriteByte,
    htmWriteRaw,
    htmEscapeInto,
    htmlEscape,
} from "@web/html";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HtmlConfig` | `class` | Fields: `default_charset: string` |
| `HtmlBuilder` | `class` | Fields: `buf: []u8, pos: usize, overflow: bool` · Methods: `init, raw, text, tag, tagRaw, slice, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `htmAsciiLower` | `(c: u8): u8` | Invokes `htmAsciiLower` and returns `u8`. |
| `htmAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `htmAsciiEqlIgnoreCase` and returns `bool`. |
| `htmWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `htmWriteByte` and returns `bool`. |
| `htmWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `htmWriteRaw` and returns `bool`. |
| `htmEscapeInto` | `(out_buf: []u8, pos: &mut usize, input: string): bool` | Invokes `htmEscapeInto` and returns `bool`. |
| `htmlEscape` | `(input: string, out_buf: []u8): usize?` | Invokes `htmlEscape` and returns `usize?`. |
| `htmlIsHtmx` | `(ctx: &Context): bool` | Invokes `htmlIsHtmx` and returns `bool`. |
| `htmlHxRedirect` | `(url: string): Response` | Invokes `htmlHxRedirect` and returns `Response`. |
| `htmlHxTrigger` | `(body_html: string, event_name: string): Response` | Invokes `htmlHxTrigger` and returns `Response`. |
| `htmAfterHook` | `(ctx: &mut Context, resp: &mut Response)` | Executes `htmAfterHook` in-place. |
| `registerHtml` | `(app: &mut App, cfg: HtmlConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { htmAsciiEqlIgnoreCase } from "@web/html";

pub fn main() {
    let result = htmAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
