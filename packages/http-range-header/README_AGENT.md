# http-range-header@1.0.0 (Agent Quickref)

> CP package http-range-header | License: MIT
> Import: `import { ... } from "http-range-header";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum RangePosition`

## Functions
- `export fn hrhIsSpace(b: u8): bool;`
- `export fn hrhParseU64(s: string, start: usize, end: usize): u64?;`
- `export fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize;`
- `export fn validateRanges(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64;`
- `export fn formatContentRange(start: u64, end: u64, total: u64, out: []u8): usize;`
- `export fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize;`
