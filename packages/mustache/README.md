[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# mustache

Zero-heap Mustache v1.4 logic-less template engine for CP with XSS-safe HTML escaping via `escape-html` — clean-room MIT parity with npm `mustache` v4.

Renders `{{ var }}` (HTML-escaped), `{{{ var }}}` / `{{& var }}` (raw), `{{# section }}...{{/ section }}` (truthy & list loops), `{{^ section }}...{{/ section }}` (inverted/falsy), and `{{! comments }}` directly into caller-provided stack buffers with zero heap allocation.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `mustache` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/janl/mustache.js` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: mustache)
import Mustache from "mustache";
const html = Mustache.render("Hello, {{name}}!", { name: "<b>CP</b>" });
```

```cp
// CP (cap: mustache) — zero-heap rendering into caller stack buffer
import { MustacheContext, renderMustache } from "mustache";

let ctx = MustacheContext.new();
ctx.setStr("name", "<b>CP</b>");
let out: [256]u8 = undefined;
const n = renderMustache("Hello, {{name}}!", ctx, out[0..256]);
// out[0..n] == "Hello, &lt;b&gt;CP&lt;/b&gt;!"
```

## Installation

```bash
cap add mustache
```

## Usage

### Basic Usage: Escaped vs Raw Interpolation, Sections & Comments

```cp
import { MustacheContext, renderMustache } from "mustache";

fn demoRender() {
    let ctx = MustacheContext.new();
    ctx.setStr("user", "<script>alert(1)</script>");
    ctx.setStr("badge", "<strong>PRO</strong>");
    ctx.setBool("is_admin", true);
    ctx.setBool("is_guest", false);

    const tpl = "{{! header }}Hi {{user}} {{{badge}}}{{#is_admin}} [ADMIN]{{/is_admin}}{{^is_guest}} (signed in){{/is_guest}}";
    let out: [512]u8 = undefined;
    const len = renderMustache(tpl, ctx, out[0..512]);
    // out[0..len] safely escapes {{user}} while keeping {{{badge}}} raw
    _ = len;
}
```

### Edge Cases: List Repetition, Small Buffer Truncation & Pool Capacity

```cp
import { MustacheContext, renderMustache } from "mustache";

fn demoEdgeCases() {
    let ctx = MustacheContext.new();
    // Repeat a section N times using setListCount
    ctx.setListCount("rows", 3);
    ctx.setListCount("empty_rows", 0);

    let out: [128]u8 = undefined;
    const n1 = renderMustache("{{#rows}}<li>item</li>{{/rows}}{{^empty_rows}}none{{/empty_rows}}", ctx, out[0..128]);
    // => "<li>item</li><li>item</li><li>item</li>none"

    // Small output buffer safely truncates without panic or overflow
    let tiny: [8]u8 = undefined;
    const n2 = renderMustache("Hello {{missing_key}} world!", ctx, tiny[0..8]);
    // n2 <= 8; missing keys render as empty string

    // Context capacity is fixed at 32 entries (setStr/setBool/setListCount return false when full)
    const count = ctx.entryCount(); // 2
    _ = n1;
    _ = n2;
    _ = count;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `MustacheContext.new` | `fn(): MustacheContext` | Creates an empty stack-allocated context (fixed pool of 32 entries). |
| `MustacheContext.setStr` | `fn(*_, key: string, val: string): bool` | Binds/updates a string value (key max 48B, val max 128B); returns `false` if pool is full. |
| `MustacheContext.setBool` | `fn(*_, key: string, val: bool): bool` | Binds/updates a boolean flag for `{{#...}}` / `{{^...}}` sections. |
| `MustacheContext.setListCount` | `fn(*_, key: string, list_count: usize): bool` | Binds/updates a repeat count (`0` is falsy for `{{^...}}`, `>0` repeats `{{#...}}`). |
| `MustacheContext.entryCount` | `fn(): usize` | Returns current number of bound entries (`0..32`). |
| `renderMustache` | `fn(template: string, ctx: MustacheContext, out: []u8): usize` | Renders `template` into `out` and returns total bytes written. |
| `mstLookup` | `fn(ctx: MustacheContext, key: string): i32` | Looks up entry index by key (`-1` if not found). |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`strEq`, `trim`) and `cap:fmt` (`writeStr`) for fast tag matching and output copying.
- **Zero Heap Allocation & Fixed Stack Pool**: `MustacheContext` stores up to 32 `MustacheEntry` items directly on the stack (`entries: [32]MustacheEntry`).
- **Zero-Clear Stack Initialization (`undefined`)**: Entry buffers (`key`, `str_val`, `entries`) use `undefined` to avoid redundant zeroing of unused slots.
- **XSS-Safe by Default**: Reuses `escape-html` (`escapeHtml`) for `{{ key }}` interpolation; all writes are bounds-checked against `out.len`.

## License

MIT — Clean-room CP implementation inspired by npm `mustache`.
