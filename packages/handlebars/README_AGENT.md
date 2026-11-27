# handlebars — AI Agent Quick Ref

- **Package**: `handlebars` (`1.0.0`) | **Source**: npm `handlebars` v4 (zero-heap, XSS-safe via `escape-html`)
- **Import**: `import { HbsContext, renderHandlebars } from "handlebars";`
- **Allocation**: Zero-heap; fixed 32-entry context, max nesting depth 16, writes into caller `[]u8`.

## Core Signatures

```typescript
export class HbsContext {
    entries: [32]HbsEntry,
    len: usize,
    fn new(): HbsContext;
    fn setStr(_: auto, k: string, v: string);
    fn setBool(_: auto, k: string, v: bool);
    fn setCount(_: auto, k: string, n: usize);
    fn size(): usize;
}

export fn renderHandlebars(tpl: string, ctx: HbsContext, out: []u8): usize;
```

## Supported Template Syntax

- `{{key}}` — HTML-escaped interpolation; `{{{key}}}` — raw unescaped interpolation
- `{{! comment }}` — omitted from output
- `{{upper key}}`, `{{lower key}}`, `{{default key "fallback"}}` — inline helpers
- `{{#if key}}...{{else}}...{{/if}}`, `{{#unless key}}...{{else}}...{{/unless}}`, `{{#repeat count_key}}...{{/repeat}}`

## Minimal Pattern

```typescript
import { HbsContext, renderHandlebars } from "handlebars";

let ctx = HbsContext.new();
ctx.setStr("name", "Alice");
ctx.setBool("vip", true);
let out: [256]u8 = undefined;
const n = renderHandlebars("Hi {{upper name}}{{#if vip}}!{{/if}}", ctx, out[0..]);
```

## Pitfalls

1. **Max 32 context entries**: Additional new keys beyond 32 are silently ignored (updating existing keys works).
2. **Buffer truncation**: `renderHandlebars` stops writing when `out` is full and returns `<= out.len` (no error thrown).
3. **Supported helpers only**: Only `upper`, `lower`, `default` inline helpers and `if`, `unless`, `repeat` block tags are built in.
4. **No `{{#each}}` array iteration**: Use `{{#repeat count_key}}` here, or use `tinytemplate` if you need `{{#each list}}` iteration.
