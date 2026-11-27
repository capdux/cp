# itoa@1.0.0 (Agent Quickref)

> Fast zero-heap two-digits-per-step integer-to-ASCII formatter (clean-room CP implementation of crates.io itoa) | License: MIT
> Import: `import { ... } from "itoa";` | Full types: `src/index.d.cp`

## Functions
- `export fn formatU64(val: u64, out: []u8): usize;`
- `export fn formatI64(val: i64, out: []u8): usize;`
- `export fn formatU32(val: u32, out: []u8): usize;`
- `export fn formatI32(val: i32, out: []u8): usize;`
- `export fn formatUsize(val: usize, out: []u8): usize;`
- `export fn formatHexU64(val: u64, uppercase: bool, out: []u8): usize;`
- `export fn formatRadixU64(val: u64, radix: u8, out: []u8): usize;`
