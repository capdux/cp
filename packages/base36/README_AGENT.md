# base36@1.0.0 (Agent Quickref)

> Zero-heap Base36 and arbitrary radix 2..36 unsigned 64-bit integer encoder, overflow-safe decoder, and validator. | License: MIT
> Import: `import { ... } from "base36";` | Full types: `src/index.d.cp`

## Functions
- `export fn digitChar(v: u8, uppercase: bool): u8;`
- `export fn digitVal(c: u8): i32;`
- `export fn encodeRadix(val: u64, radix: u8, uppercase: bool, out: []u8): usize;`
- `export fn decodeRadix(input: string, radix: u8): ParseU64Result;`
- `export fn encodeBase36(val: u64, out: []u8): usize;`
- `export fn decodeBase36(input: string): ParseU64Result;`
- `export fn isValidBase36(input: string): bool;`
