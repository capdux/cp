# sqids@1.0.0 (Agent Quickref)

> Zero-allocation YouTube-style reversible short ID generator (Sqids specification) with deterministic shuffle, min-length padding, and profanity blocklist for CP | License: MIT
> Import: `import { ... } from "sqids";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SqidsError`

## Functions
- `export fn hasDigit(s: string): bool;`
- `export fn isBlockedId(id: &[]u8): bool;`
- `export fn shuffleAlphabet(buf: []u8);`
- `export fn reverseBytes(buf: []u8);`
- `export fn appendToId(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?;`
- `export fn chunkToNumber(chunk: &[]u8, alphabet: &[]u8): u64?;`
