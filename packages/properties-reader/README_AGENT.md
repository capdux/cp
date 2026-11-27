# properties-reader@1.0.0 (Agent Quickref)

> Zero-heap Java / Spring Boot / i18n .properties parser and formatter with =/:/space separators, backslash line continuations, comments, and \uXXXX Unicode escapes. | License: MIT
> Import: `import { ... } from "properties-reader";` | Full types: `src/index.d.cp`

## Functions
- `export fn isLineWs(c: u8): bool;`
- `export fn decodeEscapeInto(src: &[]u8, esc_pos: usize, dst: []u8, dst_pos: usize): EscDecodeResult;`
- `export fn pushByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn pushEscaped(dst: []u8, pos: usize, ch: u8): i64;`
- `export fn formatEntry(key: &[]u8, value: &[]u8, dst: []u8): i64;`
