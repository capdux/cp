# stylis@4.3.6 (Agent Quickref)

> Zero-heap CSS minifier and nested selector preprocessor in pure CP | License: MIT
> Import: `import { ... } from "stylis";` | Full types: `src/index.d.cp`

## Functions
- `export fn stylIsWs(c: u8): bool;`
- `export fn stylPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn stylIsStripAfter(c: u8): bool;`
- `export fn stylIsStripBefore(c: u8): bool;`
- `export fn minifyCss(css: string, out: []u8): usize;`
- `export fn stylExpandSingle(parent: string, child: string, out: []u8, start_pos: usize): usize;`
- `export fn expandNestedSelector(parent: string, child: string, out: []u8): usize;`
- `export fn stylSkipCommentOrQuote(s: string, idx: usize): usize;`
- `export fn compileNestedCss(parent_sel: string, body_css: string, out: []u8): usize;`
