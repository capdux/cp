# bytecount@1.0.0 (Agent Quickref)

> Zero-heap fast byte frequency counting, SWAR 4-byte parallel counter, and UTF-8 char/line counters (crates.io bytecount equivalent, reuses @cap/memchr) | License: MIT
> Import: `import { ... } from "bytecount";` | Full types: `src/index.d.cp`
> Deps: `memchr`

## Functions
- `export fn count(haystack: []u8, needle: u8): usize;`
- `export fn countFast(haystack: []u8, needle: u8): usize;`
- `export fn numChars(utf8_bytes: []u8): usize;`
- `export fn countLines(buf: []u8): usize;`
- `export fn countAsciiWhitespace(buf: []u8): usize;`
- `export fn countInRange(buf: []u8, lo: u8, hi: u8): usize;`
