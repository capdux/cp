# mustache@1.0.0 — AI Agent Quick Ref

> Zero-heap Mustache v1.4 template engine with XSS-safe HTML escaping (`escape-html`) | Source: `npm:mustache` | License: MIT
> Import: `import { MustacheContext, renderMustache } from "mustache";` | Types: `src/index.d.cp`

## Core Types & Constants

```cp
export class MustacheEntry {
    key: [48]u8, key_len: usize, val_kind: u8,
    str_val: [128]u8, str_len: usize, bool_val: bool, list_count: usize,
    fn empty(): MustacheEntry;
}

export class MustacheContext {
    entries: [32]MustacheEntry, count: usize,
    fn new(): MustacheContext;
    fn entryCount(): usize;
    fn setStr(_: auto, key: string, val: string): bool;
    fn setBool(_: auto, key: string, val: bool): bool;
    fn setListCount(_: auto, key: string, list_count: usize): bool;
}
// Constants: MST_VAL_STR=1, MST_VAL_BOOL=2, MST_VAL_LIST=3, MST_MAX_ENTRIES=32
```

## Core Functions

- `renderMustache(template: string, ctx: MustacheContext, out: []u8): usize` — Render template into `out`, returns bytes written.
- `mstLookup(ctx: MustacheContext, key: string): i32` — Lookup key index (`-1` if missing).
- `mstEmitValue(ctx: MustacheContext, key: string, escaped: bool, out: []u8, pos: &mut usize)`
- `mstRenderSlice(tpl: string, ctx: MustacheContext, out: []u8, pos: &mut usize, depth: usize)`

## Syntax Supported

- `{{ key }}` — HTML-escaped via `escape-html`
- `{{{ key }}}` / `{{& key }}` — Raw unescaped output
- `{{# key }}...{{/ key }}` — Truthy section or repeat `list_count` times
- `{{^ key }}...{{/ key }}` — Inverted/falsy section
- `{{! comment }}` — Stripped comment

## Minimal Usage

```cp
import { MustacheContext, renderMustache } from "mustache";

let ctx = MustacheContext.new();
ctx.setStr("name", "<b>CP</b>");
let out: [256]u8 = undefined;
const n = renderMustache("Hi {{name}}!", ctx, out[0..256]);
```

## Pitfalls & Rules

1. **Fixed limits**: Max 32 entries per `MustacheContext`; keys truncated to 48B, string values truncated to 128B.
2. **Return value**: `renderMustache` returns `usize` (bytes written), NOT a `string` — slice `out[0..n]` yourself.
3. **Small buffers**: Output is safely truncated at `out.len` without error or panic; size `out` generously for HTML-escaped expansions.
4. **Mutable receiver**: Declare `let ctx = MustacheContext.new();` (not `const`) when calling `setStr`/`setBool`/`setListCount`.
