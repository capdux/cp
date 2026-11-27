[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# handlebars

Pure CP, zero-allocation **Handlebars** template engine composing `escape-html` for default XSS-safe HTML escaping, inline helpers (`upper`, `lower`, `default`), comments, and nested `{{#if}}` / `{{#unless}}` / `{{#repeat}}` blocks with recursion depth guards.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [npm](https://www.npmjs.com/package/handlebars) |
| **Upstream Package** | `handlebars` (v4 parity) |
| **Upstream Repo** | https://github.com/handlebars-lang/handlebars.js |
| **CP Package** | `handlebars` (`1.0.0`) |
| **License** | MIT |

Keep the exact same package name in CP — no renaming required:

```typescript
// TypeScript (npm)
import Handlebars from "handlebars";

// CP (cap)
import { HbsContext, renderHandlebars } from "handlebars";
```

## Installation

```bash
cap add handlebars
```

## Key Features & Security Highlights

- **XSS-Safe by Default**: `{{var}}` automatically HTML-escapes `&`, `<`, `>`, `"`, `'` via `escape-html`; use triple staches `{{{raw}}}` only when intentionally emitting trusted HTML.
- **Zero Heap Allocation**: `HbsContext` stores up to 32 typed bindings (`string`, `bool`, `usize`) on the stack, and `renderHandlebars` writes directly into a caller-owned `[]u8` buffer.
- **Built-in Inline Helpers**: Supports `{{upper key}}`, `{{lower key}}`, and `{{default key "fallback"}}`, plus Handlebars comments `{{! ... }}`.
- **Nested Blocks with Depth Guard**: Supports `{{#if key}}...{{else}}...{{/if}}`, `{{#unless key}}...{{else}}...{{/unless}}`, and `{{#repeat count_key}}...{{/repeat}}` with a built-in recursion limit (16 levels) to prevent stack exhaustion.

## Quick Start

```typescript
import { HbsContext, renderHandlebars } from "handlebars";

fn main(): void {
    let ctx = HbsContext.new();
    ctx.setStr("user", "<script>alert(1)</script>");
    ctx.setStr("role", "admin");
    ctx.setBool("active", true);
    ctx.setCount("stars", 3);

    const tpl =
        "<h1>Hello {{user}} ({{upper role}})</h1>" +
        "{{#if active}}<span>Status: {{default status \"online\"}}</span>{{else}}<span>offline</span>{{/if}}" +
        "{{#repeat stars}}*{{/repeat}}";

    let out: [512]u8 = undefined;
    const len = renderHandlebars(tpl, ctx, out[0..]);
    const html = out[0..len];
    _ = html;
}
```

### Raw Unescaped Output & `{{#unless}}`

```typescript
import { HbsContext, renderHandlebars } from "handlebars";

fn renderBanner(is_muted: bool): void {
    let ctx = HbsContext.new();
    ctx.setStr("badge", "<b>PRO</b>");
    ctx.setBool("muted", is_muted);

    let out: [256]u8 = undefined;
    const n = renderHandlebars("{{#unless muted}}{{{badge}}}{{/unless}}", ctx, out[0..]);
    _ = out[0..n];
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `HbsContext.new` | `() -> HbsContext` | Create an empty stack-allocated context (capacity: 32 entries) |
| `HbsContext.setStr` | `(k: string, v: string)` | Bind or update a string variable |
| `HbsContext.setBool` | `(k: string, v: bool)` | Bind or update a boolean variable |
| `HbsContext.setCount` | `(k: string, n: usize)` | Bind or update an unsigned count variable (for `{{#repeat}}` & output) |
| `HbsContext.size` | `() -> usize` | Return number of bound entries |
| `renderHandlebars` | `(tpl: string, ctx: HbsContext, out: []u8) -> usize` | Render Handlebars template into `out`, returning bytes written |
| `hbsFindBlockBounds` | `(tpl, body_start, end, block_tag) -> HbsBlockSpan` | Locate matching `{{else}}` and `{{/tag}}` bounds |
| `hbsEvalInline` | `(expr_raw, ctx, escape, out, pos) -> usize` | Evaluate a single inline expression or helper |

## License

MIT
