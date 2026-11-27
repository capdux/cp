# bit_field@1.0.0 (Agent Quickref)

> Zero-allocation bit and bit-range manipulation utilities for u64, u32, and byte slices in pure CP | License: MIT
> Import: `import { ... } from "bit_field";` | Full types: `src/index.d.cp`

## Functions
- `export fn bitLengthU8(): usize;`
- `export fn bitLengthU16(): usize;`
- `export fn bitLengthU32(): usize;`
- `export fn bitLengthU64(): usize;`
- `export fn bitLengthUsize(): usize;`
- `export fn bitLengthSlice(bytes: &[]u8): usize;`
- `export fn bfldMaskU8(width: u3): u8;`
- `export fn bfldMaskU16(width: u4): u16;`
- `export fn bfldMaskU32(width: u5): u32;`
- `export fn bfldMaskU64(width: u6): u64;`
- `export fn bfldFullMaskU64(width: u8): u64;`
- `export fn getBitU8(val: u8, bit: u3): bool;`
- `export fn setBitU8(val: u8, bit: u3, on: bool): u8;`
- `export fn getBitsU8(val: u8, start: u3, end: u3): u8;`
- `export fn setBitsU8(val: u8, start: u3, end: u3, bits: u8): u8;`
- `export fn getBitU16(val: u16, bit: u4): bool;`
- `export fn setBitU16(val: u16, bit: u4, on: bool): u16;`
- `export fn getBitsU16(val: u16, start: u4, end: u4): u16;`
- `export fn setBitsU16(val: u16, start: u4, end: u4, bits: u16): u16;`
- `export fn getBitU64(val: u64, bit: u6): bool;`
- `export fn setBitU64(val: u64, bit: u6, on: bool): u64;`
- `export fn getBitsU64(val: u64, start: u6, end: u6): u64;`
- `export fn setBitsU64(val: u64, start: u6, end: u6, bits: u64): u64;`
- `export fn getBitsRangeU64(val: u64, start: u8, end: u8): u64;`
- *(+15 more in `src/index.d.cp`)*
