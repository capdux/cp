# @paralleldrive/cuid2@1.0.0 (Agent Quickref)

> Zero-heap collision-resistant, URL-safe, lowercase CUID2 ID generator and validator (clean-room port of npm @paralleldrive/cuid2) | License: MIT
> Import: `import { ... } from "@paralleldrive/cuid2";` | Full types: `src/index.d.cp`
> Deps: `@sindresorhus/fnv1a`

## Functions
- `export fn c2Mix(x: u64): u64;`
- `export fn c2Base36Char(v: u64): u8;`
- `export fn c2LetterChar(v: u64): u8;`
- `export fn isCuid2(id: string): bool;`
- `export fn isCuid2WithBounds(id: string, min_len: usize, max_len: usize): bool;`
