# bigdecimal@1.0.0 (Agent Quickref)

> Zero-heap exact base-10 financial arithmetic library with 18-digit precision, rounding, parsing, and formatting (clean-room port of Rust bigdecimal) | License: MIT
> Import: `import { ... } from "bigdecimal";` | Full types: `src/index.d.cp`

## Functions
- `export fn bdecIsSpace(c: u8): bool;`
- `export fn bdecIsDigit(c: u8): bool;`
- `export fn bdecAbsU64(v: i64): u64;`
- `export fn bdecSignedFromU64(mag: u64, neg: bool): i64;`
- `export fn bdecPow10U64(exp: u32): u64;`
- `export fn bdecDigitCount(u: u64): i32;`
- `export fn bdecWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn parseDecimal(s: string): BigDecimal?;`
- `export fn formatDecimal(val: BigDecimal, out: []u8): usize;`
