# unicase@1.0.0 (Agent Quickref)

> Zero-heap Unicode & ASCII case-insensitive comparison, ordering, matching, and hashing (clean-room port of Rust unicase) | License: MIT
> Import: `import { ... } from "unicase";` | Full types: `src/index.d.cp`
> Deps: `@sindresorhus/fnv1a`

## Functions
- `export fn ucDecodeUtf8(s: string, pos: usize, step_out: []usize): u32;`
- `export fn ucEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn foldAsciiByte(b: u8): u8;`
- `export fn foldCodepoint(cp: u32, folded_out: []u32): usize;`
- `export fn caseFold(src: string, out: []u8): usize;`
- `export fn eqAscii(a: string, b: string): bool;`
- `export fn cmpAscii(a: string, b: string): i32;`
- `export fn cmpUnicode(a: string, b: string): i32;`
- `export fn eqUnicode(a: string, b: string): bool;`
- `export fn hashAscii(s: string): u64;`
- `export fn hashUnicode(s: string): u64;`
- `export fn startsWithIgnoreCase(haystack: string, prefix: string): bool;`
- `export fn endsWithIgnoreCase(haystack: string, suffix: string): bool;`
- `export fn containsIgnoreCase(haystack: string, needle: string): bool;`
