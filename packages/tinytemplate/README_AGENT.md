# tinytemplate — AI Agent Quick Ref

- **Package**: `tinytemplate` (`1.0.0`) | **Source**: crates.io `tinytemplate` v1 (zero-heap Mustache/Handlebars + filters)
- **Import**: `import { TemplateContext, TemplateError, render, escapeHtml } from "tinytemplate";`
- **Limits**: `MAX_BLOCK_DEPTH=16`, `MAX_VARS=64`, `MAX_LISTS=16`, `MAX_LIST_ITEMS=128`, `INT_POOL_CAPACITY=512`.

## Core Signatures

```typescript
export error TemplateError {
    BufferTooSmall, UnclosedTag, UnclosedBlock, MismatchedBlock, DepthExceeded, ContextFull, UnknownFilter
}

export class TemplateContext {
    fn init(): TemplateContext;
    fn reset(_: auto);
    fn set(_: auto, key: &[]u8, val: &[]u8): void ! TemplateError;
    fn setBool(_: auto, key: &[]u8, b: bool): void ! TemplateError;
    fn setInt(_: auto, key: &[]u8, n: i64): void ! TemplateError;
    fn get(key: &[]u8): (&[]u8)?;
    fn has(key: &[]u8): bool;
    fn varCount(): usize;
    fn pushListItem(_: auto, list_key: &[]u8, item: &[]u8): void ! TemplateError;
    fn listLen(list_key: &[]u8): usize;
    fn listItem(list_key: &[]u8, idx: usize): (&[]u8)?;
    fn isTruthy(key: &[]u8): bool;
}

export fn render(tpl: &[]u8, ctx: &TemplateContext, out: []u8): &[]u8 ! TemplateError;
export fn escapeHtml(input: &[]u8, out: []u8): &[]u8 ! TemplateError;
```

## Supported Syntax & Filters

- Interpolation: `{{key}}` (HTML-escaped), `{{{key}}}` (raw), `{{! comment }}`
- Pipeline filters: `trim`, `upper`, `lower`, `len` (chainable via `|`, e.g. `{{ name | trim | upper }}`)
- Blocks: `{{#if k}}...{{else}}...{{/if}}`, `{{#unless k}}...{{/unless}}`, `{{#each list}}...{{/each}}` (`{{this}}`, `{{@index}}`)

## Minimal Pattern

```typescript
import { TemplateContext, render } from "tinytemplate";

let ctx = TemplateContext.init();
try ctx.set("name", "world");
try ctx.pushListItem("items", "a");
let out: [512]u8 = undefined;
const html = try render("Hi {{name | upper}}: {{#each items}}{{@index}}={{this}}{{/each}}", &ctx, out[0..]);
```

## Pitfalls

1. **Error union return**: `render`, `set`, `setBool`, `setInt`, `pushListItem`, and `escapeHtml` return `! TemplateError`; always use `try` or `catch`.
2. **Pass `&ctx` to `render`**: Second parameter is `&TemplateContext` (read-only borrow).
3. **Unknown filter fails**: Only `trim`, `upper`, `lower`, `len` exist; any other filter returns `TemplateError.UnknownFilter`.
4. **Filter buffer cap**: Intermediate filter buffers are 2048 bytes (`BufferTooSmall` if exceeded).
