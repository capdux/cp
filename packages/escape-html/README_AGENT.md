# escape-html@1.0.0 (Agent Quickref)

> CP package escape-html | License: MIT
> Import: `import { ... } from "escape-html";` | Full types: `src/index.d.cp`

## Functions
- `export fn writeStr(out: []u8, pos: usize, s: string): usize;`
- `export fn matchAt(input: string, pos: usize, pat: string): bool;`
- `export fn needsHtmlEscape(input: string): bool;`
- `export fn escapedHtmlLen(input: string): usize;`
- `export fn escapeHtml(input: string, out: []u8): usize;`
- `export fn unescapeHtml(input: string, out: []u8): usize;`
