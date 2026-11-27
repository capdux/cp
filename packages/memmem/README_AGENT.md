# memmem@1.0.0 (Agent Quickref)

> Zero-allocation byte-substring search engine with precompiled BMH + KMP Finder, rfind, and non-overlapping count (Rust memchr::memmem parity) for CP | License: MIT
> Import: `import { ... } from "memmem";` | Full types: `src/index.d.cp`

## Functions
- `export fn find(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn rfind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn contains(haystack: &[]u8, needle: &[]u8): bool;`
- `export fn count(haystack: &[]u8, needle: &[]u8): usize;`
