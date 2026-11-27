# sqids@1.0.0 (Agent Quickref)

> Zero-allocation YouTube-style reversible short ID generator (Sqids specification) with deterministic shuffle, min-length padding, and profanity blocklist for CP | License: MIT
> Import: `import { ... } from "sqids";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SqidsError`

## Functions
- `export fn toLowerAscii(c: u8): u8;`
- `export fn hasDigit(s: string): bool;`
- `export fn containsSlice(haystack: &[]u8, needle: &[]u8): bool;`
- `export fn startsWithSlice(haystack: &[]u8, prefix: &[]u8): bool;`
- `export fn endsWithSlice(haystack: &[]u8, suffix: &[]u8): bool;`
- `export fn isBlockedId(id: &[]u8): bool;`
- `export fn shuffleAlphabet(buf: []u8);`
- `export fn reverseBytes(buf: []u8);`
- `export fn indexOfByte(haystack: &[]u8, needle: u8): usize?;`
- `export fn appendToId(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?;`
- `export fn chunkToNumber(chunk: &[]u8, alphabet: &[]u8): u64?;`
