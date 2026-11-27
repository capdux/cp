# adler2@1.0.0 (Agent Quickref)

> Zero-heap RFC 1950 Adler-32 checksum with streaming, rolling window update, and combine in pure CP (modeled after Rust adler2) | License: MIT
> Import: `import { ... } from "adler2";` | Full types: `src/index.d.cp`

## Functions
- `export fn adler32(data: &[]u8): u32;`
- `export fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32;`
