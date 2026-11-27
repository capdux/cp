# ejs@1.0.0 (Agent Quickref)

> Zero-heap EJS template engine supporting <%= escaped %>, <%- raw %>, <%# comments %>, <% if/else %>, and <% repeat %> tags (clean-room port of npm ejs) | License: MIT
> Import: `import { ... } from "ejs";` | Full types: `src/index.d.cp`
> Deps: `escape-html`

## Functions
- `export fn ejsWriteStr(s: string, out: []u8, pos: usize): usize;`
- `export fn ejsLookupStr(ctx: EjsContext, key: string): string;`
- `export fn ejsLookupBool(ctx: EjsContext, key: string): bool;`
- `export fn ejsLookupCount(ctx: EjsContext, key: string): usize;`
- `export fn ejsFindTagOpen(tpl: string, start_pos: usize): usize;`
- `export fn ejsFindTagClose(tpl: string, start_pos: usize): usize;`
- `export fn ejsFindBlockBounds(tpl: string, body_start: usize, out_pos: []usize): bool;`
- `export fn ejsRenderSlice(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize;`
- `export fn renderEjs(tpl: string, ctx: EjsContext, out: []u8): usize;`
