# prettier@1.0.0 (Agent Quickref)

> Zero-heap JSON and CSS pretty-printer formatter (clean-room CP implementation inspired by npm prettier) | License: MIT
> Import: `import { ... } from "prettier";` | Full types: `src/index.d.cp`

## Functions
- `export fn pfIsSpace(c: u8): bool;`
- `export fn pfPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pfPushIndent(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize;`
- `export fn pfPeekNextNonSpace(s: string, start: usize): u8;`
- `export fn formatJson(input: string, opts: FormatOptions, out: []u8): usize;`
- `export fn formatCss(input: string, opts: FormatOptions, out: []u8): usize;`
