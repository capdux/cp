# crockford-base32@1.0.0 (Agent Quickref)

> Zero-allocation Douglas Crockford Base32 encoder/decoder for 64-bit integers (with optional mod-37 check symbol), arbitrary byte streams, and grouped formatting in CP. | License: MIT
> Import: `import { ... } from "crockford-base32";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodeU64(val: u64, with_check: bool, out: []u8): usize;`
- `export fn decodeU64(input: string, with_check: bool, out_val: &mut u64): bool;`
- `export fn encodeBytes(data: &[]u8, out: []u8): usize;`
- `export fn decodeBytes(input: string, out: []u8): i64;`
- `export fn formatGrouped(raw: string, group_size: usize, out: []u8): usize;`
