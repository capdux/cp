[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# tinytemplate

Pure CP, zero-allocation **Mustache / Handlebars-compatible template engine** with default HTML XSS escaping, chained pipeline filters (`trim`, `upper`, `lower`, `len`), nested `{{#if}}` / `{{#unless}}` / `{{#each}}` blocks, `@index` / `this` loop variables, and strict compile-time capacity & depth guards.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [crates.io](https://crates.io/crates/tinytemplate) |
| **Upstream Package** | `tinytemplate` (v1 parity) |
| **Upstream Repo** | https://github.com/bheisler/TinyTemplate |
| **CP Package** | `tinytemplate` (`1.0.0`) |
| **License** | MIT |

Keep the exact same package name in CP — no renaming required:

```rust
// Rust (crates.io)
use tinytemplate::TinyTemplate;
```

```typescript
// CP (cap)
import {
    TemplateContext,
    TemplateError,
    render,
    escapeHtml,
    MAX_BLOCK_DEPTH,
    MAX_VARS,
    MAX_LISTS,
    MAX_LIST_ITEMS,
} from "tinytemplate";
```

## Installation

```bash
cap add tinytemplate
```

## Key Features & Security Highlights

- **Zero Heap Allocation**: `TemplateContext` stores up to `MAX_VARS` (64) variables, `MAX_LISTS` (16) lists, `MAX_LIST_ITEMS` (128) total list items, and a 512-byte integer formatting pool directly on the stack.
- **Default XSS Escaping**: `{{expr}}` escapes `&`, `<`, `>`, `"`, `'` automatically; `{{{expr}}}` emits raw unescaped bytes when explicitly requested.
- **Chained Pipeline Filters**: Combine built-in filters with `|`: `{{ name | trim | upper }}`, `{{ title | lower }}`, `{{ items_text | len }}`.
- **Rich Control Flow**: Nested `{{#if key}}...{{else}}...{{/if}}`, `{{#unless key}}...{{/unless}}`, and `{{#each list}}...{{/each}}` supporting `{{this}}` and 0-based `{{@index}}` up to `MAX_BLOCK_DEPTH` (16) levels.
- **Typed Error Handling**: Returns `&[]u8 ! TemplateError` (`BufferTooSmall`, `UnclosedTag`, `UnclosedBlock`, `MismatchedBlock`, `DepthExceeded`, `ContextFull`, `UnknownFilter`) instead of panicking or silently truncating.

## Quick Start

```typescript
import { TemplateContext, render } from "tinytemplate";

fn main(): !void {
    let ctx = TemplateContext.init();
    try ctx.set("user", "  <b>Ada</b>  ");
    try ctx.setBool("show_list", true);
    try ctx.setInt("year", 2026);
    try ctx.pushListItem("tags", "cp");
    try ctx.pushListItem("tags", "zero-alloc");

    const tpl =
        "<h1>{{ user | trim }} ({{year}})</h1>" +
        "{{#if show_list}}<ul>{{#each tags}}<li>{{@index}}: {{ this | upper }}</li>{{/each}}</ul>{{/if}}";

    let out: [1024]u8 = undefined;
    const rendered = try render(tpl, &ctx, out[0..]);
    _ = rendered;
}
```

### Standalone HTML Escaping

```typescript
import { escapeHtml } from "tinytemplate";

fn safeSnippet(input: &[]u8, out: []u8): &[]u8 ! void {
    const escaped = escapeHtml(input, out) catch return;
    _ = escaped;
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `TemplateError` | `BufferTooSmall \| UnclosedTag \| UnclosedBlock \| MismatchedBlock \| DepthExceeded \| ContextFull \| UnknownFilter` | Typed error union |
| `MAX_BLOCK_DEPTH` / `MAX_VARS` / `MAX_LISTS` / `MAX_LIST_ITEMS` / `INT_POOL_CAPACITY` | `16` / `64` / `16` / `128` / `512` | Fixed stack capacity bounds |
| `TemplateContext.init` | `() -> TemplateContext` | Create an empty template context |
| `TemplateContext.reset` | `()` | Clear all variables, lists, and integer pool for reuse |
| `TemplateContext.set` | `(key: &[]u8, val: &[]u8) -> void ! TemplateError` | Bind or update a string/byte variable |
| `TemplateContext.setBool` | `(key: &[]u8, b: bool) -> void ! TemplateError` | Bind `"true"` or `"false"` |
| `TemplateContext.setInt` | `(key: &[]u8, n: i64) -> void ! TemplateError` | Format and bind signed 64-bit integer in `int_pool` |
| `TemplateContext.pushListItem` | `(list_key: &[]u8, item: &[]u8) -> void ! TemplateError` | Append an item to a named list for `{{#each}}` |
| `render` | `(tpl: &[]u8, ctx: &TemplateContext, out: []u8) -> &[]u8 ! TemplateError` | Render template into `out` and return rendered slice |
| `escapeHtml` | `(input: &[]u8, out: []u8) -> &[]u8 ! TemplateError` | Escape HTML special characters into `out` |

## License

MIT
