# bytemuck@1.0.0 (Agent Quickref)

> Zero-heap safe POD byte reinterpretation, LE/BE integer/float byte casting, and alignment/size guard checks (crates.io bytemuck equivalent) | License: MIT
> Import: `import { ... } from "bytemuck";` | Full types: `src/index.d.cp`

## Functions
- `export fn checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8;`
- `export fn isAligned(addr: usize, align_req: usize): bool;`
- `export fn fillZeroed(buf: []u8);`
- `export fn zeroedU8(): u8;`
- `export fn zeroedU16(): u16;`
- `export fn zeroedU32(): u32;`
- `export fn zeroedU64(): u64;`
- `export fn zeroedI32(): i32;`
- `export fn zeroedI64(): i64;`
- `export fn zeroedF32(): f32;`
- `export fn zeroedF64(): f64;`
- `export fn writeU16Le(v: u16, out: []u8): bool;`
- `export fn readU16Le(buf: []u8): u16?;`
- `export fn writeU16Be(v: u16, out: []u8): bool;`
- `export fn readU16Be(buf: []u8): u16?;`
- `export fn writeU32Le(v: u32, out: []u8): bool;`
- `export fn readU32Le(buf: []u8): u32?;`
- `export fn writeU32Be(v: u32, out: []u8): bool;`
- `export fn readU32Be(buf: []u8): u32?;`
- `export fn writeU64Le(v: u64, out: []u8): bool;`
- `export fn readU64Le(buf: []u8): u64?;`
- `export fn writeU64Be(v: u64, out: []u8): bool;`
- `export fn readU64Be(buf: []u8): u64?;`
- `export fn writeF32Le(v: f32, out: []u8): bool;`
- *(+35 more in `src/index.d.cp`)*
