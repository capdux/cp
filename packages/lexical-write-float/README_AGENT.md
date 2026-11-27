# lexical-write-float@1.0.0 (Agent Quickref)

> Zero-allocation pure CP integer and floating-point formatter (rust-lexical lexical-write-float / lexical-write-integer parity) | License: MIT
> Import: `import { ... } from "lexical-write-float";` | Full types: `src/index.d.cp`

## Functions
- `export fn lexwCopyStr(out: []u8, pos: usize, src: string): usize;`
- `export fn lexwWritePaddedU64(out: []u8, pos: usize, val: u64, width: usize): usize;`
- `export fn lexwWriteSpecialFloat(val: f64, out: []u8): usize;`
- `export fn writeUintRadix(val: u64, radix: u32, uppercase: bool, out: []u8): usize;`
- `export fn writeIntRadix(val: i64, radix: u32, uppercase: bool, out: []u8): usize;`
- `export fn writeFloatFixed(val: f64, decimals: u8, out: []u8): usize;`
- `export fn writeFloatSci(val: f64, precision: u8, out: []u8): usize;`
