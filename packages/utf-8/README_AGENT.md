# utf-8@1.0.0 (Agent Quickref)

> Zero-copy incremental streaming UTF-8 decoder and RFC 3629 validator with Unicode Maximal Subpart lossy recovery in pure CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "utf-8";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Utf8DecodeStatus`

## Functions
- `export fn u8dExpectedLen(b0: u8): usize;`
- `export fn u8dIsValidSecond(b0: u8, b1: u8): bool;`
- `export fn u8dIsValidCont(b0: u8, idx: usize, b: u8): bool;`
- `export fn u8dWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;`
- `export fn u8dWriteReplacement(out: []u8, pos: usize): usize;`
- `export fn decodeUtf8Chunk(input: &[]u8): Utf8ChunkResult;`
- `export fn isValidUtf8(input: &[]u8): bool;`
