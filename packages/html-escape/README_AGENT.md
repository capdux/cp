# html-escape@1.0.0 (Agent Quickref)

> Zero-heap HTML entity escaper and unescaper (named entities + decimal/hex numeric character references) for CP (compatible with npm escape-html, he & Rust html-escape) | License: MIT
> Import: `import { ... } from "html-escape";` | Full types: `src/index.d.cp`

## Functions
- `export fn appendStr(dst: []u8, pos: usize, src: string): usize;`
- `export fn needsEscape(input: string): bool;`
- `export fn escapeHtml(input: string, out: []u8): usize;`
- `export fn encodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn decodeNamedEntity(name: string): u32;`
- `export fn decodeNumericEntity(body: string): u32;`
- `export fn unescapeHtml(input: string, out: []u8): usize;`
